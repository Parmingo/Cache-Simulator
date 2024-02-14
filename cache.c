

/**
 * Cache simulator
 * This file is modified to complete the functions of cache_new, cache_load_word, cache_load_block,
 * and cache_store_word. These functions are utilized in main.c and are tested from there as well.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "main.h"

// need a value that is not valid in order to initialize the tag
#define INVALID_TAG 0xFFFFFFFF

// line struct with pointers for doubly linked list
typedef struct line_t {
    unsigned long tag;
    char *data;
    struct line_t *prev;
    struct line_t *next;
} line_t;

// set struct with pointers for head & tail
typedef struct set_t {
    line_t *head;
    line_t *tail;
} set_t;

// struct to represent entire cache (did not use the one in cache.h)
typedef struct cache_t1 {
    set_t *sets;
    unsigned int word_size;
    unsigned int set_cnt;
    unsigned int line_cnt;
    unsigned int line_size;
    unsigned int clock;
} cache_t1;

/**
 * This is a helper function which helps split the address into its three components.
 * @param c pointer to the cache structure
 * @param addr the memory address that needs to be split
 * @param tag this is the pointer to store tag
 * @param index pointer to store index
 * @param offset pointer to store the offset
 */
static void split_addr(cache_t1 *c, unsigned long addr, unsigned long *tag,
                       unsigned long *index, unsigned long *offset) {

    // calculations for offset, index & tag (three components of address)
    *offset = addr % c -> line_size;
    *index = (addr / c -> line_size) % c -> set_cnt;
    *tag = (addr / c -> line_size) / c -> set_cnt;
}

/**
 * This is a helper function to find a line by searching for it by tag
 * @param set pointer to the set in which line can be found in
 * @param tag of line to be found
 * @return the current line or NULL if not found
 */
static line_t *find_line(set_t *set, unsigned long tag) {

    // assign current pointer in line and set it to head
    line_t *current = set -> head;

    // while current is not null
    while (current) {
        // check if curr is tag, and if it is then return current
        if (current -> tag == tag) {
            return current;
        }
        // else move to next
        current = current -> next;
    }
    return NULL;
}

/**
 * This is a helper function to move a line to the head of the linked list. This is used to
 * access the LRU by moving it to the head.
 * @param c pointer to cache structure
 * @param index to locate set where line is located in
 * @param line to be moved
 */
static void move_to_head(cache_t1 *c, unsigned int index, line_t *line) {

    // check if line is equal to the head at index, and just return if it is
    if (line == c -> sets[index].head) {
        return;
    }

    // if it isnt, then move to next, and detach
    if (line->next) {
        line -> next -> prev = line -> prev;
    }

    // if line is not at head, then adjust pointers of adjacent nodes to detach it
    if (line -> prev) {
        line -> prev -> next = line -> next;
    }

    // if line at tail, update the pointer to prev node
    if (line == c -> sets[index].tail) {
        c -> sets[index].tail = line -> prev;
    }

    // move line to the head of set, and set prev to null
    line -> next = c -> sets[index].head;
    line -> prev = NULL;
    if (line -> next) {
        line -> next -> prev = line;
    }

    // update head to newly moved line
    c -> sets[index].head = line;
}

/**
 * This is a helper function which is called when a cache miss occurs.
 * @param c pointer to cache structure
 * @param index of set which does the mishandling
 * @param tag of line to be modified or changed
 * @param addr is the address of line causing the miss
 */
static void mishandling(cache_t1 *c, unsigned int index, unsigned long tag, unsigned long addr) {

    // create pointer to the lru line, which is at tail, and then move it to head to make it accessible
    line_t *line = c -> sets[index].tail;
    move_to_head(c, index, line);

    //  update tag of line to reflect new data
    line -> tag = tag;

    // load block from next level of memory into cache line, since data for given address was not found
    load_block_from_next_level(addr, line -> data);
}

/**
 * This function instantiates a new cache and returns a handle (pointer) to it.
 * @param word_size is the size of the machine word in bits
 * @param sets the number of sets in the cache
 * @param lines the number of lines in each set
 * @param line_size the number of bytes stored in each line
 * @return a value of type cache_t, pointer to newly created cache
 */
extern cache_t cache_new(unsigned int word_size, unsigned int sets,
                         unsigned int lines, unsigned int line_size) {

    // allocate memory to the size of cache, and make sure its not null
    cache_t1 *c = malloc(sizeof(cache_t1));
    assert(c != NULL);

    // allocate memory for the size of set and make sure its not null
    c -> sets = malloc(sets * sizeof(set_t));
    assert(c -> sets != NULL);

    // for loop to iterate over each set to initialize it
    for (unsigned int i = 0; i < sets; i++) {
        c -> sets[i].head = NULL;
        c -> sets[i].tail = NULL;
        line_t *prev_line = NULL;

        // for loop to iterate over each line in the set and initialize it
        for (unsigned int j = 0; j < lines; j++) {

            // memory allocation for line so it can be initialized
            line_t *line = malloc(sizeof(line_t));
            line -> tag = INVALID_TAG;
            line -> data = malloc(line_size);
            line -> prev = prev_line;
            line -> next = NULL;

            // if previous line is not null, link the line together for linkedlist
            if (prev_line != NULL) {
                prev_line -> next = line;
            } else {
                // otherwise this line becomes head
                c -> sets[i].head = line;
            }

            // update tail of set to current line, and update previous line
            c -> sets[i].tail = line;
            prev_line = line;
        }
    }

    // set parameters for cache
    c -> word_size = word_size;
    c -> set_cnt = sets;
    c -> line_cnt = lines;
    c -> line_size = line_size;
    c -> clock = 0;

    // return pointer to newly created cache
    return (cache_t)c;
}

/**
 * This function takes a handle (pointer) to a cache that was created by a cache_new() and loads
 * a word at memory address.
 * @param handle is the pointer to the cache, created by cache_new()
 * @param address the location of value to be loaded. This is the address of a reference in the
 * reference stream.
 * @param word pointer to a buffer of where the word is to be loaded (copied into).
 */
extern void cache_load_word(cache_t handle, unsigned long address, void *word) {

    // variables declared for the address
    unsigned long tag;
    unsigned int index;
    unsigned int offset;

    // initialize a pointer to cache structure with the handle, casting handle back
    cache_t1 *c = (cache_t1 *)handle;

    // split the address
    split_addr(c, address, &tag, &index, &offset);

    // find the line
    line_t *line = find_line(&c -> sets[index], tag);

    // if line isn't null, move it to head
    if (line != NULL) {
        move_to_head(c, index, line);

        // byte offset calculation, and then copy the word
        unsigned int byte_offset = (offset % c -> line_size);
        memcpy(word, line -> data + byte_offset, c -> word_size / 8);
    } else {
        // its a miss and call mishandling function
        mishandling(c, index, tag, address);
        line = find_line(&c -> sets[index], tag);
        unsigned int byte_offset = (offset % c -> line_size);
        memcpy(word, line -> data + byte_offset, c -> word_size / 8);
    }
}

/**
 * This function takes a handle (pointer) to a cache and loads a block of size bytes of memory that
 * contains the memory address
 * @param handle the pointer to the cache, created by cache_new()
 * @param address a location within the block of memory to be loaded
 * @param block a pointer to buffer of where the block is to be loaded (copied into)
 * @param size the size of the block to be loaded. The block is no bigger than a line in the cache
 * but may be smaller than a line.
 */
extern void cache_load_block(cache_t handle, unsigned long address, void *block,
                             unsigned int size) {

    // variables declared for the address
    unsigned long tag;
    unsigned int index;
    unsigned int offset;

    // initialize a pointer to cache structure with the handle, casting handle back
    cache_t1 *c = (cache_t1 *)handle;

    // split the address
    split_addr(c, address, &tag, &index, &offset);

    // find the line
    line_t *line = find_line(&c -> sets[index], tag);

    // if line isn't null move to head and copy the block
    if (line != NULL) {
        move_to_head(c, index, line);
        memcpy(block, line -> data, size);
    } else {
        // else miss and call mishandling function
        mishandling(c, index, tag, address);
        line = find_line(&c -> sets[index], tag);
        memcpy(block, line -> data, size);
    }
}

/**
 * This function takes a handle (pointer) to a cache and stores a word at memory address. I.e., this
 * is what the CPU does when it needs to store a word into memory.
 * @param handle the pointer to the cache, created by cache_new()
 * @param address the location of the value to be stored. This is the address of a reference in the
 * reference stream.
 * @param word a pointer to a buffer from where the word is stored (copied into).
 */
extern void cache_store_word(cache_t handle, unsigned long address, void *word) {

    // variables declared for the address
    unsigned long tag;
    unsigned int index;
    unsigned int offset;

    // initialize a pointer to cache structure with the handle, casting handle back
    cache_t1 *c = (cache_t1 *)handle;

    // split the address
    split_addr(c, address, &tag, &index, &offset);

    // find the line
    line_t *line = find_line(&c -> sets[index], tag);

    // if line isn't null move it to head
    if (line != NULL) {
        move_to_head(c, index, line);
        unsigned int byte_offset = (offset % c -> line_size);
        // key diff b/w store_word and load_word is how memcpy is used
        // this difference is due to the data flow direction, storing data from user location -> specific position
        // while loading is specific position -> user specified location
        memcpy(line -> data + byte_offset, word, c -> word_size / 8);
    } else {
        mishandling(c, index, tag, address);
        line = find_line(&c->sets[index], tag);
        // copy word to store
        unsigned int byte_offset = (offset % c -> line_size);
        memcpy(line -> data + byte_offset, word, c -> word_size / 8);
    }
}

