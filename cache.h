//
// Created by Alex Brodsky on 2023-11-13.
//

#ifndef CACHESIM_CACHE_H
#define CACHESIM_CACHE_H

typedef void *cache_t;

extern cache_t cache_new(unsigned int word_size, unsigned int sets, unsigned int lines, unsigned int line_size);
extern void cache_load_word(cache_t handle, unsigned long addresss, void *word);
extern void cache_load_block(cache_t handle, unsigned long address, void *block, unsigned int size);
extern void cache_store_word(cache_t handle, unsigned long address, void *word);

#endif //CACHESIM_CACHE_H
