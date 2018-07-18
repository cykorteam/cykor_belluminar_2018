## 0. XSS on the chat server

The chat server sets `$(new_item).html(...user controlled value...)`, so it has XSS. It also executes `<script>` tag, by jQuery.

## 1. HTTP request injection by server's parser bug

The backend server's HTTP recv routine is pretty naive:

```c
#define BUFSIZE 1024
nbytes = recv(cli_socket, (char *)buffer, BUFSIZE, 0);
```

and for all responses, it sends Connection: keep-alive header so the socket isn't closed.

When chrome send a request with body, it sends the data separately. So the body will be the next HTTP request to the server, allowing some forbidden methods (forbidden by CORS, because of the different port).

PoC:

```js
var go = (payload) => {
    var xhr = new XMLHttpRequest();
    var buffer = new Uint8Array(payload.length);
    payload.split('').forEach(
        (x, y) => buffer[y] = x.charCodeAt(0)
    );
    xhr.open('POST', baseURL + 'a'.repeat(0x263), true);
    xhr.send(buffer);
};

```

## 2. SQL Injection via race condition

The query-string parser on the backend uses global array to store parsed values, and it has reentrancy problem between threads (all requests are processed by threaded routines), allowing some SQL injection filter to be bypassed.

Routine for /register has a slow filtering routine (O(n\*m\*m)), and the one for /search has a fast filtering one (O(n)).

One can send `/search?search='a' * 64 + "', 1); ... query ... "` and execute some statements since there are no callback, allowing the execution of multiple statements.

## 3. SQLite 1-day

Since the DLL is given, participants can get the version, and find that [some 1days](https://www.blackhat.com/docs/us-17/wednesday/us-17-Feng-Many-Birds-One-Stone-Exploiting-A-Single-SQLite-Vulnerability-Across-Multiple-Software.pdf) works. We used the chaintin's one with some variants on exploit.

There is two vulnerability: leak via `FTS_TOKENIZER('...')` and a type confusion via `SELECT FTS_TOKENIZER('simple', x'4141414100000000'); CREATE VIRTUAL TABLE a USING FTS3(b);`. Since there is a search function which just returns a value from the db, one can insert a value to `chat_table` with their `usernum` value, and just send `postMessage` to receive the result from the db.

After leaking the sqlite3 module base via storing `fts_tokenizer('simple')`, we can either heap-spray some pointers or find a vector that stores a controlled values to the binary's data section, because when the type confusion is triggered, it crashes like:

```
call [rdi+0x8] # rdi: controlled value (ex. 0x41414141)
```

Heap-spray can be possible, but it's not reliable in my case since all string in sqlite is null-terminated, and pointers too. I used `pragma soft_heap_limit = controlled_value;`, which is stored on binary base + 0x190898.

Then EIP is controllable.

### Exploit

Since EIP is controlled, other things can be possible. Fortunately the request is stored on the stack with 1024 byte buffer, and there is a gadget that directly jumps the stack to our payload:

```
0x00000200000db6ac : add rsp, 0x1030 ; pop rdi ; ret

(obtained by ROPgadget)
```

Then the stack is on &(payload[96]). Then we can ROP on the sqlite3.dll. Our ROP chain is:

```
pop rax # writable + 0..len - 0x10
pop rcx # value
mov [rax + 0x10], rcx (n times)
...
pop rcx # writable
[LoadLibraryA stub on sqlite3.dll]
```

Then we can load a DLL from SMB / WebDAV directory on our server (attacker's server), and pop a shell.