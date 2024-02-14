#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "cache.h"

typedef struct level {
    char name[20];
    cache_t cache;
    int hits;
    int misses;
    unsigned int line_size;
} level;

static level *caches;
static int current = 0;
static void *memory;
static int levels;

static void log_result(int curr, unsigned long address, int hit) {
    if (hit) {
        caches[curr].hits++;
        printf("Cache %s hit @ 0x%4.4lx\n", caches[curr].name, address);
    } else {
        printf("Cache %s miss @ 0x%4.4lx\n", caches[curr].name, address);
        caches[curr].misses++;
    }
}

int main() {
    if (scanf("%d", &levels) != 1) {
        printf("Error reading number of levels\n");
        return 1;
    }

    int word_size = 0;
    if (scanf("%d", &word_size) != 1) {
        printf("Error reading word size\n");
        return 1;
    }

    int mem_size = 0;
    if (scanf("%d", &mem_size) != 1) {
        printf("Error reading memory size\n");
        return 1;
    }

    memory = calloc(mem_size, 1);
    assert(memory);

    caches = calloc(levels, sizeof(level));
    assert(caches);
    for (int i = 0; i < levels; i++) {
        unsigned sets = 0;
        unsigned lines = 0;
        unsigned line_size = 0;
        scanf("%s %u %u %u", caches[i].name, &sets, &lines, &line_size);
        caches[i].cache = cache_new(word_size, sets, lines, line_size);
        assert(caches[i].cache);
        caches[i].line_size = line_size;
        printf("Level %u cache: sets: %u, lines %u, block size: %u\n", i + 1, sets, lines, line_size);
    }

    int num_refs = 0;
    if (scanf("%d", &num_refs) != 1) {
        printf("Error reading number of references\n");
        return 1;
    }

    int wbytes = word_size / 8;
    int cache_boundary = caches[0].line_size - wbytes;
    int memory_boundary = mem_size - wbytes;

    for (int i = 0; i < num_refs; i++) {
        char buffer[10];
        unsigned long address;
        if (scanf("%2s %lu", buffer, &address) != 2) {
            printf("Error reading operation\n");
            return 1;
        }

        assert(address <= memory_boundary);

        int adjustment = address % caches[0].line_size - cache_boundary;
        if (adjustment > 0) {
          printf("Boundary adjustment: %lu -> %lu\n",
                  address, address - adjustment);
          address -= adjustment;
        }

        unsigned long word;
        current = 0;
        switch (buffer[0]) {
            case 'L':
                cache_load_word(caches[current].cache, address, &word);
                break;
            case 'S':
                if (scanf("%lu", &word) != 1) {
                    printf("Error reading value to be stored.\n");
                    return 1;
                }

                for (int j = 0; j < levels; j++) {
                    cache_store_word(caches[j].cache, address, &word);
                }
                memcpy(memory + address, &word, wbytes );
                break;
            default:
                printf("Unknown operation (line %d): %s\n", i + 1, buffer);
                return 1;
        }

        log_result(0, address, current == 0);

        switch (buffer[0]) {
            case 'L':
                printf("Loaded word: 0x%lx\n", word);
                break;
            case 'S':
                printf("Stored word: 0x%lx\n", word);
                break;
        }
    }

    for (int i = 0; i < levels; i++) {
        printf("Cache: %s, hits: %d, misses: %d\n", caches[i].name, caches[i].hits,
               caches[i].misses);
    }
    return 0;
}

extern void load_block_from_next_level(unsigned long address, void *block) {
    unsigned int size = caches[current].line_size;
    //printf("load_block_from_next_level: Address: %lu, Current Cache Level: %d\n", address, current);
    current++;
    int curr = current;
    if (current < levels) {
        //printf("Forwarding to next cache level: %d\n", current);
        cache_load_block(caches[current].cache, address, block, size);
        log_result(curr, address,curr == current);
    } else {
        //printf("Loading from main memory\n");
        unsigned long offset = address - (address % size);
        memcpy(block, memory + offset, size);
    }

    /**
    // DEBUGGING PRINT STATEMENTS
    if (current >= levels) {
        unsigned long offset = address - (address % size);
        memcpy(block, memory + offset, size);
        //printf("Copying from memory address %lu: ", offset);
        for (int i = 0; i < size; i++) {
            //printf("%02x ", ((unsigned char*)block)[i]);
        }
        //printf("\n");
    }
     **/
}
