#ifndef IO_URING_H
#define IO_URING_H

#include <linux/io_uring.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mman.h>

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************
 * CONSTANTS                   *
 *******************************/

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

#define QUEUE_DEPTH 0x80

#define IORING_OFF_PBUF_RING 0x80000000ULL
#define IOU_PBUF_RING_MMAP 1

#define PBUF_SIZE(entries) (entries * sizeof(struct io_uring_buf))
#define PBUF_ENTRIES(x) (((x / sizeof(size_t) * 0x1000) / PBUF_SIZE(1)))
#define PBUF_ENTRIES_SIZE(x) PBUF_SIZE(PBUF_ENTRIES(x))

/*******************************
 * CUSTOM STRUCTURES           *
 *******************************/

// Extended completion queue entry with buffer info
struct io_uring_cqe2 {
    __u64 user_data;
    __s32 res;
    union {
        struct {
            __u16 _pad;
            __u16 bid;
        };
        __u32 flags;
    };
};

// Buffer registration structure
struct io_uring_buf_reg2 {
    __u64 ring_addr;
    __u32 ring_entries;
    __u16 bgid;
    __u16 flags;
    __u64 resv[3];
};


/*******************************
 * FUNCTION DECLARATIONS       *
 *******************************/

// Syscall wrappers
int io_uring_setup(unsigned entries, struct io_uring_params *p);
int io_uring_enter(int ring_fd, unsigned int to_submit,
                   unsigned int min_complete, unsigned int flags);
int io_uring_register(int fd, unsigned int opcode, const void *arg,
                      unsigned int nr_args);

// Main functions
void setup_io_uring(void);

// provide buffer functions
void *prep_setup_provide_buffer(unsigned entries);
void setup_provide_buffer(u16 bgid, void* pbuf, unsigned entries);
void setup_provide_buffer_mmap(u16 bgid, unsigned entries);
void *mmap_provide_buffer(u16 bgid, unsigned entries);
void destroy_provide_buffer(u16 bgid);
u32 provided_buffer_status(u16 bgid);

void wake_poll(void);
void io_uring_read_pbuf(uint16_t bgid, int fd);
void io_uring_write(int fd, void *buf, size_t count);
struct io_uring_cqe2 wait_for_cqe(void);

// Global variables
extern int ring_fd;
extern struct io_uring_params params;
extern size_t rings_size;
extern uint64_t user_data;
extern int ring_event;
extern void *sq_ptr;
extern struct io_uring_sqe *sqes;
extern _Atomic unsigned *sq_head;
extern _Atomic unsigned *sq_tail;
extern _Atomic unsigned *sq_mask;
extern _Atomic unsigned *sq_array;
extern _Atomic unsigned *sq_flags;
extern _Atomic unsigned *cq_head;
extern _Atomic unsigned *cq_tail;
extern _Atomic unsigned *cq_mask;
extern struct io_uring_cqe *cqes;

#ifdef __cplusplus
}
#endif

#endif /* IO_URING_H */
