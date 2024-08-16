/**
 * source: https://github.com/IAIK/SLUBStick/blob/main/exploits/include/slab_settings.h
 */

#pragma once
#include <sys/resource.h>
#include <sys/time.h>
#include <stddef.h>
#include <errno.h>
#include "util.h"

/**
 * found best values for large:
 *  $ for i in {1..15}; do echo $i; cat tmp | grep "slab $i " | grep -v not | wc -l; done
 */


/*
 * ALLOCS_SIZE:          Fetch form /sys/kernel/slab/kmalloc-SIZE/slab_size
 * SLABS_PER_CHUNK_SIZE: ?
 * SLAB_RECLAIMED_AS_PAGE_TABLE_SIZE: ?
 * OBJS_PER_SLABS_SIZE:  Fetch from /sys/kernel/slab/kmalloc-SIZE/objs_per_slab
 * YIELD_SIZE:           ?
 * CPU_PARTIAL_SIZE:     Fetch this from /sys/kernel/slab/kmalloc-SIZE/cpu_partial
 */

// SLAB_SIZE == 4096
#define ALLOCS_4k 512
#define SLAB_PER_CHUNK_4k 15
#define OBJS_PER_SLAB_4k 8
#define SLAB_RECLAIMED_AS_PAGE_TABLE_4k 9
#define YIELD_4k 10000
#define CPU_PARTIAL_4k 6
// SLAB_SIZE == 2048
#define ALLOCS_2k 512
#define SLAB_PER_CHUNK_2k 15
#define OBJS_PER_SLAB_2k 16
#define SLAB_RECLAIMED_AS_PAGE_TABLE_2k 12
#define YIELD_2k 10000
#define CPU_PARTIAL_2k 24
// SLAB_SIZE == 1024
#define ALLOCS_1k 512
#define SLAB_PER_CHUNK_1k 15
#define OBJS_PER_SLAB_1k 16
#define SLAB_RECLAIMED_AS_PAGE_TABLE_1k 7
#define YIELD_1k 10000
#define CPU_PARTIAL_1k 24
// SLAB_SIZE == 512
#define ALLOCS_512 512
#define SLAB_PER_CHUNK_512 15
#define OBJS_PER_SLAB_512 16
#define SLAB_RECLAIMED_AS_PAGE_TABLE_512 4
#define YIELD_512 10000
#define CPU_PARTIAL_512 52
// SLAB_SIZE == 256
#define ALLOCS_256 512
#define SLAB_PER_CHUNK_256 7
#define OBJS_PER_SLAB_256 16
#define SLAB_RECLAIMED_AS_PAGE_TABLE_256 0
#define YIELD_256 100000
#define CPU_PARTIAL_256 52
// SLAB_SIZE == 192
#define ALLOCS_192 2048
#define SLAB_PER_CHUNK_192 12
#define OBJS_PER_SLAB_192 21
#define SLAB_RECLAIMED_AS_PAGE_TABLE_192 5
#define YIELD_192 100000
#define CPU_PARTIAL_192 120
// SLAB_SIZE == 128
#define ALLOCS_128 4096
#define SLAB_PER_CHUNK_128 8
#define OBJS_PER_SLAB_128 32
#define SLAB_RECLAIMED_AS_PAGE_TABLE_128 1
#define YIELD_128 100000
#define CPU_PARTIAL_128 120
// SLAB_SIZE == 96
#define ALLOCS_96 4096
#define SLAB_PER_CHUNK_96 12
#define OBJS_PER_SLAB_96 42
#define SLAB_RECLAIMED_AS_PAGE_TABLE_96 10
#define YIELD_96 20000
#define CPU_PARTIAL_96 120
// SLAB_SIZE == 64
#define ALLOCS_64 8192
#define SLAB_PER_CHUNK_64 8
#define OBJS_PER_SLAB_64 64
#define SLAB_RECLAIMED_AS_PAGE_TABLE_64 5
#define YIELD_64 20000
#define CPU_PARTIAL_64 120
// SLAB_SIZE == 32
#define ALLOCS_32 8192
#define SLAB_PER_CHUNK_32 6
#define OBJS_PER_SLAB_32 128
#define SLAB_RECLAIMED_AS_PAGE_TABLE_32 3
#define YIELD_32 20000
#define CPU_PARTIAL_32 120
// SLAB_SIZE == 16
#define ALLOCS_16 16384
#define SLAB_PER_CHUNK_16 6
#define OBJS_PER_SLAB_16 256
#define SLAB_RECLAIMED_AS_PAGE_TABLE_16 4
#define YIELD_16 10000
#define CPU_PARTIAL_16 120
// SLAB_SIZE == 8
#define ALLOCS_8 32768
#define SLAB_PER_CHUNK_8 6
#define OBJS_PER_SLAB_8 512
#define SLAB_RECLAIMED_AS_PAGE_TABLE_8 4
#define YIELD_8 10000
#define CPU_PARTIAL_8 120

struct slab_info {
    size_t size;
    size_t allocs;
    size_t pre_allocs;
    size_t slab_per_chunk;
    size_t objs_per_slab;
    size_t reclaimed_page_table;
    size_t yield;
    size_t cpu_partial;
};
static struct slab_info slab_infos[] = {
    {
        .size = 4096,
        .allocs = ALLOCS_4k,
        .slab_per_chunk = SLAB_PER_CHUNK_4k,
        .objs_per_slab = OBJS_PER_SLAB_4k,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_4k,
        .yield = YIELD_4k,
        .cpu_partial = CPU_PARTIAL_4k,
    },
    {
        .size = 2048,
        .allocs = ALLOCS_2k,
        .slab_per_chunk = SLAB_PER_CHUNK_2k,
        .objs_per_slab = OBJS_PER_SLAB_2k,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_2k,
        .yield = YIELD_2k,
        .cpu_partial = CPU_PARTIAL_2k,
    },
    {
        .size = 1024,
        .allocs = ALLOCS_1k,
        .slab_per_chunk = SLAB_PER_CHUNK_1k,
        .objs_per_slab = OBJS_PER_SLAB_1k,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_1k,
        .yield = YIELD_1k,
        .cpu_partial = CPU_PARTIAL_1k,
    },
    {
        .size = 512,
        .allocs = ALLOCS_512,
        .slab_per_chunk = SLAB_PER_CHUNK_512,
        .objs_per_slab = OBJS_PER_SLAB_512,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_512,
        .yield = YIELD_512,
        .cpu_partial = CPU_PARTIAL_512,
    },
    {
        .size = 256,
        .allocs = ALLOCS_256,
        .slab_per_chunk = SLAB_PER_CHUNK_256,
        .objs_per_slab = OBJS_PER_SLAB_256,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_256,
        .yield = YIELD_256,
        .cpu_partial = CPU_PARTIAL_256,
    },
    {
        .size = 192,
        .allocs = ALLOCS_192,
        .slab_per_chunk = SLAB_PER_CHUNK_192,
        .objs_per_slab = OBJS_PER_SLAB_192,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_192,
        .yield = YIELD_192,
        .cpu_partial = CPU_PARTIAL_192,
    },
    {
        .size = 128,
        .allocs = ALLOCS_128,
        .slab_per_chunk = SLAB_PER_CHUNK_128,
        .objs_per_slab = OBJS_PER_SLAB_128,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_128,
        .yield = YIELD_128,
        .cpu_partial = CPU_PARTIAL_128,
    },
    {
        .size = 96,
        .allocs = ALLOCS_96,
        .slab_per_chunk = SLAB_PER_CHUNK_96,
        .objs_per_slab = OBJS_PER_SLAB_96,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_96,
        .yield = YIELD_96,
        .cpu_partial = CPU_PARTIAL_96,
    },
    {
        .size = 64,
        .allocs = ALLOCS_64,
        .slab_per_chunk = SLAB_PER_CHUNK_64,
        .objs_per_slab = OBJS_PER_SLAB_64,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_64,
        .yield = YIELD_64,
        .cpu_partial = CPU_PARTIAL_64,
    },
    {
        .size = 32,
        .allocs = ALLOCS_32,
        .slab_per_chunk = SLAB_PER_CHUNK_32,
        .objs_per_slab = OBJS_PER_SLAB_32,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_32,
        .yield = YIELD_32,
        .cpu_partial = CPU_PARTIAL_32,
    },
    {
        .size = 16,
        .allocs = ALLOCS_16,
        .slab_per_chunk = SLAB_PER_CHUNK_16,
        .objs_per_slab = OBJS_PER_SLAB_16,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_16,
        .yield = YIELD_16,
        .cpu_partial = CPU_PARTIAL_16,
    },
    {
        .size = 8,
        .allocs = ALLOCS_8,
        .slab_per_chunk = SLAB_PER_CHUNK_8,
        .objs_per_slab = OBJS_PER_SLAB_8,
        .reclaimed_page_table = SLAB_RECLAIMED_AS_PAGE_TABLE_8,
        .yield = YIELD_8,
        .cpu_partial = CPU_PARTIAL_8,
    },
};
struct slab_info *cur = 0;
static inline void set_current_slab_info(size_t size)
{
    int ret;
    for (size_t i = 0; i < sizeof(slab_infos)/sizeof(struct slab_info); ++i)
        if (slab_infos[i].size == size)
            cur = &slab_infos[i];
    if (cur == 0)
        lerror("slab with size %ld not found", size);
    ldebug("slab with size %ld found", size);
    ldebug(" - cur->size                    %ld", cur->size);
    ldebug(" - cur->allocs                  %ld", cur->allocs);
    ldebug(" - cur->slab_per_chunk          %ld", cur->slab_per_chunk);
    ldebug(" - cur->objs_per_slab           %ld", cur->objs_per_slab);
    ldebug(" - cur->reclaimed_page_table    %ld", cur->reclaimed_page_table);
    ldebug(" - cur->yield                   %ld", cur->yield);
}
