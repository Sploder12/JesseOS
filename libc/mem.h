#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

void memcpy(void *source, void *dest, uint32_t nbytes);
void memset(void *dest, int val, uint32_t len);

void initialize_heap(uint32_t kernel_end);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);

#define HEAP_DEBUG
#ifdef HEAP_DEBUG
void dump_heap();

void dump_mem(void* ptr);
#endif

#endif
