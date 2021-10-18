#include "mem.h"

void memcpy(void* source, void *dest, uint32_t nbytes) {
    int i;
    for (i = 0; i < nbytes; i++) {
        *((char*)dest + i) = *((char*)source + i);
    }
}

void memset(void *dest, int val, uint32_t len) {
    uint8_t *temp = (uint8_t *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

typedef struct __attribute__((packed)) heap_meta
{
	size_t size;
	struct heap_meta* next;
	struct heap_meta* prev;
} __attribute__((packed)) heap_meta;

heap_meta* heap_begin = 0;
size_t heap_size = 0;

void initialize_heap(uint32_t kernel_end)
{
	heap_begin = (heap_meta*)(kernel_end + 0x1000);
	heap_begin->size = 0;
	heap_begin->next = 0x0; 
	heap_begin->prev = 0x0; 
}

//if size is 0 then the block is free
//next - ptr can be used to find size of free block
heap_meta* findNextFree(heap_meta* ptr, size_t nsize)
{	
	if (ptr->next == 0x0) return ptr;
	
	if (ptr->size == 0 && (void*)ptr->next - (void*)ptr > nsize + sizeof(heap_meta)) return ptr;	
	
	return findNextFree(ptr->next, nsize);
}

void* kmalloc(size_t size)
{	
	if (size == 0) return 0x0;

	heap_meta* free_mem_addr = findNextFree(heap_begin, size);
	
	heap_meta* oldNext = free_mem_addr->next;
	
	free_mem_addr->size = size;
	
	heap_meta* newNext = (heap_meta*)((void*)(free_mem_addr) + size + sizeof(heap_meta));
	free_mem_addr->next = newNext;
	
	newNext->next = oldNext;
	newNext->prev = (void*)(free_mem_addr) + sizeof(heap_meta);
	
	heap_size += size + sizeof(heap_meta);
	memset(free_mem_addr+sizeof(heap_meta), 0, size); //clean up leftovers
	
    return (void*)(free_mem_addr) + sizeof(heap_meta);
}

void kfree(void* ptr)
{
	if (ptr == 0x0) return;

	heap_meta* meta = (heap_meta*)(ptr - sizeof(heap_meta));

	size_t freedSize = meta->size; 
	if (freedSize == 0) return;
	
	//Clean crumbs
	memset(ptr, 0, freedSize);
	meta->size = 0;
	
	heap_meta* next = meta->next;
	heap_meta* prev = meta->prev;
	
	if (next->size == 0)
	{
		meta->next = next->next;
		
		next->next = 0x0;
		next->prev = 0x0;
	}
	
	if (prev->size == 0)
	{
		next->prev = prev;
		prev->next = meta->next;
		
		meta->next = 0x0;
		meta->prev = 0x0;
	}
	
	heap_size -= (freedSize + sizeof(heap_meta));	
}

void* krealloc(void* ptr, size_t size)
{
	if (ptr == 0x0) return 0x0;

	heap_meta* meta = (heap_meta*)(ptr - sizeof(heap_meta));

	size_t curSize = meta->size;
	if (size < curSize)
	{
		meta->size = size;
		return ptr;
	}
	
	heap_meta* next = meta->next;
	
	while (next != 0x0 && next->size == 0)
	{
		next = next->next;
	}
	
	if (next == 0x0 || (void*)next - (void*)ptr > size)
	{
		meta->size = size;
		meta->next = (heap_meta*)((void*)(meta->next) + size - curSize);
		
		meta->next->next = next;
		meta->next->prev = meta;
		return ptr;
	}
	
	void* newp = kmalloc(size);
	memcpy(ptr, newp, size);
	kfree(ptr);
	return newp;	
}


#ifdef HEAP_DEBUG
#include "string.h"
#include "../drivers/screen.h"
void dump_blocks(heap_meta* block)
{
	size_t size = block->size;
	heap_meta* next = block->next;
	heap_meta* prev = block->prev;
	
	kprint_color(GREEN_TEXT);
	char sizestr[16] = "";
	int_to_ascii(size, sizestr);
	kprint(sizestr);
	kprint(" ");
	
	kprint_color(RED_TEXT);
	char nextstr[16] = "";
	hex_to_ascii((uint32_t)next, nextstr);
	kprint(nextstr);
	kprint(" ");
	
	kprint_color(BLUE_TEXT);
	char prevstr[16] = "";
	hex_to_ascii((uint32_t)prev, prevstr);
	kprint(prevstr);
	kprint(" ");
	
	/*
	kprint_color(GRAY_TEXT);
	for (size_t i = 0; i < size + 12; i++)
	{
		char hexstr[16] = "";
		hex_to_ascii(*(char*)(block+i), hexstr);
		kprint(hexstr);
		kprint(" ");
	}
	*/
	
	if (next != 0) dump_blocks(next);
}

void dump_heap()
{
	//clear_screen();
	char hsize[16] = "";
	int_to_ascii(heap_size, hsize);
	kprint("Heap size: ");
	kprint(hsize);
	kprint(" bytes \n");
	dump_blocks(heap_begin);
}
#endif
