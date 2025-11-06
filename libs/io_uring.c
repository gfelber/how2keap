#include <complex.h>
#include <sys/mman.h>
#define _GNU_SOURCE
#include "io_uring.h"
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>

/*******************************
 * GLOBAL VARIABLES            *
 *******************************/

int ring_fd = -1;
struct io_uring_params params = {0};
size_t rings_size = 0;
uint64_t user_data = 0x6fe1be2L << 32;
int ring_event = -1;

// submission queue pointers
void *sq_ptr = NULL;
struct io_uring_sqe *sqes = NULL;
_Atomic unsigned *sq_head = NULL;
_Atomic unsigned *sq_tail = NULL;
_Atomic unsigned *sq_mask = NULL;
_Atomic unsigned *sq_array = NULL;
_Atomic unsigned *sq_flags = NULL;

// completion queue pointers
_Atomic unsigned *cq_head = NULL;
_Atomic unsigned *cq_tail = NULL;
_Atomic unsigned *cq_mask = NULL;
struct io_uring_cqe *cqes = NULL;

/*******************************
 * SYSCALL WRAPPERS            *
 *******************************/

int io_uring_setup(unsigned entries, struct io_uring_params *p) {
  return syscall(SYS_io_uring_setup, entries, p);
}

int io_uring_enter(int ring_fd, unsigned int to_submit,
                   unsigned int min_complete, unsigned int flags) {
  return syscall(SYS_io_uring_enter, ring_fd, to_submit, min_complete, flags,
                 NULL, 0);
}

int io_uring_register(int fd, unsigned int opcode, const void *arg,
                      unsigned int nr_args) {
  return syscall(SYS_io_uring_register, fd, opcode, arg, nr_args);
}

/*******************************
 * MAIN FUNCTIONS              *
 *******************************/

void setup_io_uring(void) {
  memset(&params, 0, sizeof(params));

#ifdef POLLING
  params.flags |= IORING_SETUP_SQPOLL;
#endif
  params.features |= IORING_FEAT_SINGLE_MMAP;

  // Setup io_uring
  ring_fd = io_uring_setup(QUEUE_DEPTH, &params);
  if (ring_fd < 0) {
    return;
  }

  // sq_array is at the end of io_rings
  rings_size = params.sq_off.array + params.sq_entries * sizeof(unsigned);

  // mmap the submission and completion rings
  sq_ptr = mmap(0, rings_size, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_POPULATE, ring_fd, IORING_OFF_SQ_RING);
  if (sq_ptr == MAP_FAILED) {
    return;
  }

  void *cq_ptr = sq_ptr;

  // Get the pointers for various ring structures
  sqes = mmap(0, params.sq_entries * sizeof(struct io_uring_sqe),
              PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ring_fd,
              IORING_OFF_SQES);
  if (sqes == MAP_FAILED) {
    return;
  }

  // submission queue pointers
  sq_head = sq_ptr + params.sq_off.head;
  sq_tail = sq_ptr + params.sq_off.tail;
  sq_mask = sq_ptr + params.sq_off.ring_mask;
  sq_array = sq_ptr + params.sq_off.array;
  sq_flags = sq_ptr + params.sq_off.flags;

  // completion queue pointers
  cq_head = cq_ptr + params.cq_off.head;
  cq_tail = cq_ptr + params.cq_off.tail;
  cq_mask = cq_ptr + params.cq_off.ring_mask;
  cqes = cq_ptr + params.cq_off.cqes;

  // register eventfd
#ifdef POLLING
  ring_event = eventfd(0, EFD_CLOEXEC);
  if (ring_event >= 0) {
    io_uring_register(ring_fd, IORING_REGISTER_EVENTFD_ASYNC, &ring_event, 1);
  }
#endif
}

void wake_poll(void) {
#ifdef POLLING
  if (sq_flags && (*sq_flags & IORING_SQ_NEED_WAKEUP)) {
    io_uring_enter(ring_fd, 0, 0, IORING_ENTER_SQ_WAKEUP);
  }
#endif
}

void setup_provide_buffer_mmap(u16 bgid, unsigned entries) {
  struct io_uring_buf_reg2 pbuf_params = {.ring_addr = 0,
                                          .ring_entries = entries,
                                          .bgid = bgid,
                                          .flags = IOU_PBUF_RING_MMAP};

  int ret = SYSCHK(
      io_uring_register(ring_fd, IORING_REGISTER_PBUF_RING, &pbuf_params, 1));
}

void *mmap_provide_buffer(u16 bgid, unsigned entries) {
  struct io_uring_buf_ring *pbuf;
  size_t pbuf_size = PBUF_SIZE(entries);
  u64 off = IORING_OFF_PBUF_RING | (((u64)bgid) << IORING_OFF_PBUF_SHIFT);
  pbuf = mmap(0, pbuf_size, PROT_READ | PROT_WRITE, MAP_SHARED, ring_fd, off);
  CHK(pbuf != MAP_FAILED);

  return pbuf;
}

void *prep_setup_provide_buffer(unsigned entries) {
  struct io_uring_buf_ring *pbuf;
  size_t pbuf_size = PBUF_SIZE(entries);

  pbuf =
      mmap(0, pbuf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (pbuf == MAP_FAILED)
    lerror("mmap pbuf failed");

  for (unsigned i = 0; i < entries; ++i) {
    pbuf->bufs[i].addr = 0;
    pbuf->bufs[i].len = 0;
    pbuf->bufs[i].bid = i;
    pbuf->tail = i;
  }

  return pbuf;
}

u32 provided_buffer_status(u16 bgid) {
  struct io_uring_buf_status status = {
      .buf_group = bgid,
  };
  int rc = io_uring_register(ring_fd, IORING_REGISTER_PBUF_STATUS, &status, 1);
  if (rc < 0)
    return -errno;
  return status.head;
}

void setup_provide_buffer(u16 bgid, void *pbuf, unsigned entries) {
  struct io_uring_buf_reg2 pbuf_params = {.ring_addr = (u64)pbuf,
                                          .ring_entries = entries,
                                          .bgid = bgid,
                                          .flags = 0};

  int ret = SYSCHK(
      io_uring_register(ring_fd, IORING_REGISTER_PBUF_RING, &pbuf_params, 1));
}

void destroy_provide_buffer(u16 bgid) {

  struct io_uring_buf_reg2 pbuf_params = {
      .ring_addr = 0, .ring_entries = 0, .bgid = bgid, .flags = 0};
  int ret = SYSCHK(
      io_uring_register(ring_fd, IORING_UNREGISTER_PBUF_RING, &pbuf_params, 1));
}

void io_uring_read_pbuf(uint16_t bgid, int fd) {
  if (!sq_tail || !sq_mask || !sqes || !sq_array) {
    return;
  }

  unsigned index = *sq_tail & *sq_mask;
  struct io_uring_sqe *sqe = &sqes[index];
  memset(sqe, 0, sizeof(*sqe));

  // submission queue entry
  sqe->opcode = IORING_OP_READ;
  sqe->fd = fd;
  sqe->off = -1;
  sqe->flags = IOSQE_BUFFER_SELECT;
  sqe->buf_group = bgid;
  sqe->user_data = user_data++;

  sq_array[index] = *sq_tail & *sq_mask;
  write_barrier();
  *sq_tail += 1;

  // Submit the request
#ifndef POLLING
  io_uring_enter(ring_fd, 1, 1, IORING_ENTER_GETEVENTS);
#endif
}

void io_uring_write(int fd, void *buf, size_t count) {
  if (!sq_tail || !sq_mask || !sqes || !sq_array) {
    return;
  }

  unsigned index = *sq_tail & *sq_mask;
  struct io_uring_sqe *sqe = &sqes[index];
  memset(sqe, 0, sizeof(*sqe));

  // submission queue entry
  sqe->opcode = IORING_OP_WRITE;
  sqe->fd = fd;
  sqe->off = -1;
  sqe->addr = (uintptr_t)buf;
  sqe->len = count;
  sqe->flags = 0;
  sqe->user_data = user_data++;

  sq_array[index] = *sq_tail & *sq_mask;
  write_barrier();
  *sq_tail += 1;

  // Submit the request
#ifndef POLLING
  io_uring_enter(ring_fd, 1, 1, IORING_ENTER_GETEVENTS);
#endif
}

struct io_uring_cqe2 wait_for_cqe(void) {
  struct io_uring_cqe2 cqe = {0};

  if (!cq_head || !cq_mask || !cqes) {
    cqe.res = -EINVAL;
    return cqe;
  }

#ifdef POLLING
  if (ring_event >= 0) {
    uint64_t val;
    read(ring_event, &val, sizeof(val));
  }
#endif

  read_barrier();
  cqe = *(struct io_uring_cqe2 *)&cqes[*cq_head & *cq_mask];
  *cq_head += 1;

  return cqe;
}
