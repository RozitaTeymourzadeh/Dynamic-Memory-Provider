/**
 * @file
 *
 * Explores memory management at the C runtime level.
 *
 * Author: Rozita Teymourzadeh , Allison Wong, Mathew Malensek
 *
 * To use (one specific command):
 * LD_PRELOAD=$(pwd)/allocator.so command
 * ('command' will run with your allocator)
 *
 * To use (all following commands):
 * export LD_PRELOAD=$(pwd)/allocator.so
 * (Everything after this point will use your custom allocator -- be careful!)
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "allocator.h"
#include "logger.h"

static struct mem_block *g_head = NULL; /*!< Start (head) of our linked list */
static unsigned long g_allocations = 0; /*!< Allocation counter */
pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER; /*< Mutex for protecting the linked list */
bool scribble = false;


/**
 * void *malloc_name(size_t size, char *name)
 *
 * Malloc name to allocate dynamic memory and provide name feature
 *
 * @param size        memory size
 * @param name        pointer to memory name
 * @return void
  */
void *malloc_name(size_t size, char *name){

    LOG("Allocation Requestion: %zu bytes\n", size);
    /* set indicator for scribble flag */
    scribble = false;
    char *indicator = getenv("ALLOCATOR_SCRIBBLE");
    if (indicator != NULL) {/* set scribble flag to true if indicator is NULL*/
        if (strcmp(indicator, "1") == 0) {
            scribble = true;
        }
    }
    void *region_ptr = reuse(size);
    pthread_mutex_lock(&alloc_mutex);
    if (region_ptr == NULL) {
        LOG("Region pointer was %s", "NULL\n");
        size_t actual_size = size + sizeof(struct mem_block);
        if (actual_size % 8 != 0) {
            actual_size = actual_size + (8 - actual_size % 8);
            LOG("Aligned size: %zu\n", actual_size);
        }
        int page_size = getpagesize();
        size_t num_pages = actual_size / page_size;
        if (actual_size % page_size != 0) {
            num_pages = num_pages + 1;
        }

        /* calculate region_sz */
        size_t region_sz = num_pages * page_size;

        /* call mmap to allocate memory */
        struct mem_block *block = mmap(
            NULL,                           /* address, use NULL to let kernel decide */
            region_sz,                      /* size of memory block to allocate */
            PROT_READ | PROT_WRITE,         /* memory protection flags */
            MAP_PRIVATE | MAP_ANONYMOUS,    /* type of mapping */
            -1,                             /* file descriptor */
            0                               /* offset in memory */
        );
        /* error message if allocate memory failed */
        if (block == MAP_FAILED) {
            perror("mmap error");
            pthread_mutex_unlock(&alloc_mutex);
            return NULL;
        }
        block->alloc_id = g_allocations++;
        if(name == NULL){
            char buffer[32];
            sprintf(buffer, "%lu", block->alloc_id);
            strcpy(block->name, "ALOCATOR ");
            strcat(block->name, buffer);
        } else {
            strcpy(block->name, name);
        }

        block->size = region_sz;
        block->usage = actual_size;
        block->region_start = block;
        block->region_size = region_sz;
        block->next = NULL;
        if (g_head == NULL) {
            g_head = block;
        } else {
            struct mem_block *tail = g_head;
            while (tail->next != NULL) {
                tail = tail->next;
            }
            LOG("Updating tail: %p -> %p\n", tail, tail->next);
            tail->next = block;
        }
        if (scribble) {
            memset(block + 1, 0xAA, size);
        }
        pthread_mutex_unlock(&alloc_mutex);
        LOG("Successfully allocated memory @ %p\n", block);
        return block + 1;
    } else {
        LOG("Region pointer was %s", "not NULL\n");
        if (scribble) {
            memset(region_ptr, 0xAA, size);
        }
        struct mem_block *data_block = (struct mem_block*) region_ptr - 1;
        if(name == NULL){
            char buffer[32];
            sprintf(buffer, "%lu", data_block->alloc_id);
            strcpy(data_block->name, "ALOCATOR ");
            strcat(data_block->name, buffer);
        } else {
            strcpy(data_block->name, name);
        }
        LOG("ALLOCATION ID: %lu\n", data_block->alloc_id);
        pthread_mutex_unlock(&alloc_mutex);
        LOG("Successfully return region_ptr @ %p\n", region_ptr);
        return region_ptr;
    }
}

/**
 * void *first_fit(size_t size)
 *
 * A part of FSM system to find first fit memory
 *
 * @param size        memory size
 * @return void       void pointer
  */
void *first_fit(size_t size)
{
    /* first fit FSM implementation */
    struct mem_block *block = g_head; /* get the head of the linked list */
    size_t actual_size = size + sizeof(struct mem_block); /* calculate actual size = size + header */
    if (actual_size % 8 != 0) {
        actual_size = actual_size + (8 -actual_size % 8);
        LOG("Aligned size: %zu\n", actual_size);
    }
    if (block == NULL) {
        return NULL;
    }
    while (block != NULL) {
        if (block->size - block->usage >= actual_size) {  /* find the first space */
            if (block->usage == 0) { /* consider available space as required space */
                block->alloc_id = g_allocations++;
                block->usage = actual_size;
                return block + 1;
            } else { /* if find space has some other usage */
                struct mem_block *create_block = (void*) block + block->usage; /* pointer to the create block */
                create_block->alloc_id = g_allocations++;
                create_block->size = block->size - block->usage;
                create_block->usage = actual_size;
                create_block->region_start = block->region_start;
                create_block->region_size = block->region_size;
                create_block->next = block->next;
                block->next = create_block;
                block->size = block->usage;
                return create_block + 1;
            }
        }
        block = block->next;
    }
    return NULL;
}

/**
 * void *worst_fit(size_t size)
 *
 * A part of FSM system to find worst fit memory
 *
 * @param size        memory size
 * @return void       void pointer
  */
void *worst_fit(size_t size)
{
    /* worst fit FSM implementation */
    struct mem_block *block = g_head; /* get the head of the linked list */
    size_t actual_size = size + sizeof(struct mem_block);
    if (actual_size % 8 != 0) {
        actual_size = actual_size + (8 -actual_size % 8);
        LOG("Aligned size: %zu\n", actual_size);
    }
    int cnt = 0;
    int worst = 0;
    int i = 0;
    struct mem_block *temp_block = g_head; /* get the head of the linked list in temp to manipulate it */
    int temp_cnt = 0;
    if (block == NULL) {
        return NULL;
    }
    while (block != NULL) {
        cnt++;
        if (block->size - block->usage >= actual_size) { /* find the match */
            int remaining = block->size - block->usage - actual_size;
            if (remaining > worst) { /* calculate the waste and save it to worst */
                worst = remaining;
                i = cnt;
            }
        }
        block = block->next;
    }
    while (temp_block != NULL) {
        temp_cnt++;
        if (temp_cnt == i) {
            if (temp_block->usage == 0) { /* consider available space as required space */
                temp_block->alloc_id = g_allocations++;
                temp_block->usage = actual_size;
                return temp_block + 1;
            } else { /* if find worst space has some other usage */
                struct mem_block *create_block = (void*) temp_block + temp_block->usage;
                create_block->alloc_id = g_allocations++;
                create_block->size = temp_block->size - temp_block->usage;
                create_block->usage = actual_size;
                create_block->region_start = temp_block->region_start;
                create_block->region_size = temp_block->region_size;
                create_block->next = temp_block->next;
                temp_block->next = create_block;
                temp_block->size = temp_block->usage;
                return create_block + 1;
            }
        }
        temp_block = temp_block->next;
    }
    return NULL;
}

/**
 * void *best_fit(size_t size)
 *
 * A part of FSM system to find best fit memory
 *
 * @param size        memory size
 * @return void       void pointer
  */
void *best_fit(size_t size)
{
    /*best fit FSM implementation */
    size_t actual_size = size + sizeof(struct mem_block);
    if(actual_size %8 != 0){
        actual_size = actual_size + (8 -actual_size % 8);
        LOG("Aligned size: %zu\n", actual_size);
    }
    struct mem_block *block = g_head;
    int cnt = 0;
    int best = INT_MAX;
    int i = 0;
    struct mem_block *temp_block = g_head;/* get the head of the linked list in temp to manipulate it */
    int temp_cnt = 0;
    if (block == NULL) {
        return NULL;
    }
    while (block != NULL) {
        cnt++;
        if (block->size - block->usage >= actual_size) {
            int remaining = block->size - block->usage - actual_size;
            if (remaining < best) {/* calculate the waste and save it to best */
                LOG("Inside Loop, cnt: %d, remaining: %d, best: %d, i: %d, block->name: %s\n", cnt, remaining, best, i, block->name);
                best = remaining;
                LOG("Inside Loop, setting i to %d\n", cnt);
                i = cnt;
            }
        }
        block = block->next;
    }
    LOG("Found i: %d\n", i);
    LOG("best was: %d\n", best);
    while (temp_block != NULL) {
        temp_cnt++;
        if (temp_cnt == i) { /* find the match */
            if (temp_block->usage == 0) {
                temp_block->alloc_id = g_allocations++;
                temp_block->usage = actual_size;
                return temp_block + 1;
            } else { /* if find best space has some other usage */
                struct mem_block *create_block = (void*) temp_block + temp_block->usage;
                create_block->alloc_id = g_allocations++;
                create_block->size = temp_block->size - temp_block->usage;
                create_block->usage = actual_size;
                create_block->region_start = temp_block->region_start;
                create_block->region_size = temp_block->region_size;
                create_block->next = temp_block->next;
                temp_block->next = create_block;
                temp_block->size = temp_block->usage;
                return create_block + 1;
            }
        }
        temp_block = temp_block->next;
    }
    return NULL;
}

/**
 * void *reuse(size_t size)
 *
 * Driver for FSM system to select memory search method.
 *
 * @param size        memory size
 * @return void       void
  */
void *reuse(size_t size)
{
    pthread_mutex_lock(&alloc_mutex);
    /*using free space management (FSM) algorithms, find a block of memory that we can reuse. Return NULL if no suitable block is found.*/
    char *algo = getenv("ALLOCATOR_ALGORITHM");
    if (algo == NULL) {
        algo = "first_fit";
    }
    if (strcmp(algo, "first_fit") == 0) {
        void *ptr = first_fit(size);
        pthread_mutex_unlock(&alloc_mutex);
        return ptr;
    } else if (strcmp(algo, "best_fit") == 0) {
        void *ptr = best_fit(size);
        pthread_mutex_unlock(&alloc_mutex);
        return ptr;
    } else if (strcmp(algo, "worst_fit") == 0) {
        void *ptr = worst_fit(size);
        pthread_mutex_unlock(&alloc_mutex);
        return ptr;
    }
    pthread_mutex_unlock(&alloc_mutex);
    return NULL;
}

/**
 * void *malloc_name(size_t size)
 *
 * Malloc name to allocate dynamic memory
 *
 * @param size        memory size
 * @return void       void pointer
  */
void *malloc(size_t size)
{
    /* allocate memory. You'll first check if you can reuse an existing block. If not, map a new memory region.*/
    return malloc_name(size, NULL);
}

/**
 * void free(void *ptr)
 *
 * Free dynamic memory
 *
 * @param *ptr       void pointer
 * @return void
  */
void free(void *ptr)
{
    /*TODO: free memory. If the containing region is empty (i.e., there are no more blocks in use), then it should be unmapped.*/
    pthread_mutex_lock(&alloc_mutex);
    LOG("Free request @ %p\n", ptr);
    bool free_region = true;
    if (ptr == NULL) {
        pthread_mutex_unlock(&alloc_mutex);
        return;
    }
    /* Free block */
    struct mem_block *block = (struct mem_block*) ptr - 1;
    block->usage = 0;
    struct mem_block *next_region = NULL;
    struct mem_block *tail = block->region_start;
    /* check to see if region is empty triger free_region flag */
    while (tail != NULL)
    {
        if (tail->alloc_id != block->alloc_id)
        {
            if (tail->region_start != block->region_start)
            {
                next_region = tail->region_start;
                break;
            }
            if (tail->usage != 0)
            {
                free_region = false;
                pthread_mutex_unlock(&alloc_mutex);
                LOG("Free request successfully performed in free_region @ %p\n", tail);
                return;
            }
        }
        tail = tail->next;
    }
    /* if free_reqion flag triggers in free block section, then free region */
    if (free_region)
    {
        void *block_draft = block->region_start;
        munmap(block->region_start, block->region_size);
        if (block_draft == g_head)
        {
            g_head = next_region;
        } else {
            struct mem_block *previous = g_head;
            while (previous->next != block)
            {
                if (previous->next == block_draft)
                {
                    break;
                } else {
                    previous = previous->next;
                }
            }
            previous->next = next_region;
        }
    }
    pthread_mutex_unlock(&alloc_mutex);
}

/**
 * void *calloc(size_t nmemb, size_t size)
 *
 * Calloc dynamic memory
 *
 * @param nmemb       void pointer
 * @param size        memory size
 * @return void       void pointer
  */
void *calloc(size_t nmemb, size_t size)
{
    LOG("Calloc request @ %ld; size = %zu\n", nmemb, size);
    size_t actual_size = nmemb * size;
    void *ptr = malloc(actual_size);
    if (ptr == NULL) {
        return NULL;
    }
    /* for calloc we malloc and set it to zero */
    pthread_mutex_lock(&alloc_mutex);
    memset(ptr, 0x00, actual_size);
    pthread_mutex_unlock(&alloc_mutex);
    return ptr;
}

/**
 * void *realloc(void *ptr, size_t size)
 *
 * Realloc dynamic memory to extend malloc
 *
 * @param *ptr        void pointer
 * @param size        memory size
 * @return void       void pointer
  */
void *realloc(void *ptr, size_t size)
{
    LOG("Re-allocation request @ %p; size = %zu\n", ptr, size);

    /*
    * Realloc algorithm:
    * 1. create a new block of the requested size
    * 2. copy the old contents to the new block
    * 3. delete the old block
    * 4. return pointer to the new block
    *
    * A few things to consider:
    * - what if they realloc to the new block
    *       -> just return the same pointer.
    * - what if it is bigger
    *       -> that is the case above (creating the new block), unless the block itself has space already.
    * - what if it is smaller
    *       -> resize the block
    * - what if it is 0?
    *       -> free the memory
    */


    size_t actual_size = size + sizeof(struct mem_block);
    if (actual_size % 8 != 0) {
        actual_size = actual_size + (8 -actual_size % 8);
        LOG("Aligned size: %zu\n", actual_size);
    }
    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    struct mem_block *block = (struct mem_block*) ptr - 1;
    if (actual_size <= block->size) {
        block->usage = actual_size;
        return ptr;
    } else if (actual_size > block->size) {
        void *malloc_ptr = malloc(size);
        pthread_mutex_lock(&alloc_mutex);
        memcpy(malloc_ptr, ptr, block->usage - sizeof(struct mem_block));
        pthread_mutex_unlock(&alloc_mutex);

        free(ptr);
        return malloc_ptr;
    }
    return NULL;
}

/**
 * void save_memory(FILE *fd)
 *
 * Get the memory sate and save it to the file.
 *
 * @param fd         File
 * @return void
  */
void save_memory(FILE *fd)
{
    if(fd == NULL) {
        fd = stdout;
    }

    fputs("-- Current Memory State --\n", fd);
    struct mem_block *current_block = g_head;
    struct mem_block *current_region = NULL;
    while (current_block != NULL) {
        if (current_block->region_start != current_region) {
            current_region = current_block->region_start;
            char s[1024];
            sprintf(s, "[REGION] %p-%p %zu\n",
                    current_region,
                    (void *) current_region + current_region->region_size,
                    current_region->region_size);
            fputs(s, fd);
        }
        char s2[1024];
        sprintf(s2, "[BLOCK]  %p-%p (%lu) '%s' %zu %zu %zu\n",
                current_block,
                (void *) current_block + current_block->size,
                current_block->alloc_id,
                current_block->name,
                current_block->size,
                current_block->usage,
                current_block->usage == 0
                    ? 0 : current_block->usage - sizeof(struct mem_block));

        fputs(s2, fd);
        current_block = current_block->next;
    }
}

/**
 * print_memory
 *
 * Prints out the current memory state, including both the regions and blocks.
 * Entries are printed in order, so there is an implied link from the topmost
 * entry to the next, and so on.
 */
void print_memory(void)
{
    save_memory(NULL);
}

