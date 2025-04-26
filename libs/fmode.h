/* SPDX-License-Identifier: GPL-2.0 */
/* from include/linux/fs.h */

/* file is open for reading */
#define FMODE_READ (1 << 0)
/* file is open for writing */
#define FMODE_WRITE (1 << 1)
/* file is seekable */
#define FMODE_LSEEK (1 << 2)
/* file can be accessed using pread */
#define FMODE_PREAD (1 << 3)
/* file can be accessed using pwrite */
#define FMODE_PWRITE (1 << 4)
/* File is opened for execution with sys_execve / sys_uselib */
#define FMODE_EXEC (1 << 5)
/* File writes are restricted (block device specific) */
#define FMODE_WRITE_RESTRICTED (1 << 6)
/* File supports atomic writes */
#define FMODE_CAN_ATOMIC_WRITE (1 << 7)

/* FMODE_* bit 8 */

/* 32bit hashes as llseek() offset (for directories) */
#define FMODE_32BITHASH (1 << 9)
/* 64bit hashes as llseek() offset (for directories) */
#define FMODE_64BITHASH (1 << 10)

/*
 * Don't update ctime and mtime.
 *
 * Currently a special hack for the XFS open_by_handle ioctl, but we'll
 * hopefully graduate it to a proper O_CMTIME flag supported by open(2) soon.
 */
#define FMODE_NOCMTIME (1 << 11)

/* Expect random access pattern */
#define FMODE_RANDOM (1 << 12)

/* File is huge (eg. /dev/mem): treat loff_t as unsigned */
#define FMODE_UNSIGNED_OFFSET (1 << 13)

/* File is opened with O_PATH; almost nothing can be done with it */
#define FMODE_PATH (1 << 14)

/* File needs atomic accesses to f_pos */
#define FMODE_ATOMIC_POS (1 << 15)
/* Write access to underlying fs */
#define FMODE_WRITER (1 << 16)
/* Has read method(s) */
#define FMODE_CAN_READ (1 << 17)
/* Has write method(s) */
#define FMODE_CAN_WRITE (1 << 18)

#define FMODE_OPENED (1 << 19)
#define FMODE_CREATED (1 << 20)

/* File is stream-like */
#define FMODE_STREAM (1 << 21)

/* File supports DIRECT IO */
#define FMODE_CAN_ODIRECT (1 << 22)

#define FMODE_NOREUSE (1 << 23)

/* FMODE_* bit 24 */

/* File is embedded in backing_file object */
#define FMODE_BACKING (1 << 25)

/* File was opened by fanotify and shouldn't generate fanotify events */
#define FMODE_NONOTIFY (1 << 26)

/* File is capable of returning -EAGAIN if I/O will block */
#define FMODE_NOWAIT (1 << 27)

/* File represents mount that needs unmounting */
#define FMODE_NEED_UNMOUNT (1 << 28)

/* File does not contribute to nr_files count */
#define FMODE_NOACCOUNT (1 << 29)
