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

uint32_t heap_begin = 0;
size_t heap_size = 0;

void initialize_heap(uint32_t kernel_end)
{
	heap_begin = kernel_end + 0x1000;
	*(uint32_t*)(heap_begin) = 0x00000000; //ensure size is 0
	*(uint32_t*)(heap_begin+4) = 0x00000000; //ensure next is nullptr
	*(uint32_t*)(heap_begin+8) = 0x00000000; //ensure previous is nullptr
}

//if size is 0 then the block is free
//next - ptr can be used to find size of free block
void* findNextFree(void* ptr, size_t nsize)
{
	size_t size = *(size_t*)(ptr-12);
	void* next = (void*)*(uint32_t*)(ptr-8);
	
	if (next == 0) return ptr;
	
	if (size == 0 && next - ptr > nsize + 12) return ptr;	
	
	return findNextFree(next, nsize);
}

void* kmalloc(size_t size)
{	
	void* free_mem_addr = findNextFree((void*)(heap_begin + 12), size);
	
	
	uint32_t oldNext = *(uint32_t*)(free_mem_addr-8);
	
	*(size_t*)(free_mem_addr-12) = size;
	*(uint32_t*)(free_mem_addr-8) = (uint32_t)(free_mem_addr + size + 12);
	
	
	//*(size_t*)(free_mem_addr+size+4) = 0;
	
	*(uint32_t*)(free_mem_addr+size+4) = oldNext;
	*(uint32_t*)(free_mem_addr+size+8) = (uint32_t)(free_mem_addr);
	
	
	heap_size += size + 12;
	memset(free_mem_addr, 0, size); //clean up leftovers
	
    return free_mem_addr;
}

void kfree(void* ptr)
{
	size_t freedSize = *(size_t*)(ptr-12); 
	void* next = (void*)*(uint32_t*)(ptr-8);
	void* prev = (void*)*(uint32_t*)(ptr-4);
	
	//Clean crumbs
	//If you don't metadata corruption can occur and that's no good
	memset(ptr, 0, freedSize);
	*(size_t*)(ptr-12) = 0;
	
	size_t nsize = *(size_t*)(next-12);
	size_t psize = *(size_t*)(prev-12);
	
	
	if (nsize == 0)
	{
		*(uint32_t*)(ptr-8) = *(uint32_t*)(next-8);
		
		*(uint32_t*)(next-8) = 0x0;
		*(uint32_t*)(next-4) = 0x0;
	}
	
	if (psize == 0)
	{
		*(uint32_t*)(next-4) = (uint32_t)(prev);
		*(uint32_t*)(prev-8) = *(uint32_t*)(ptr-8);
	}
	
	heap_size -= (freedSize + 12);
}

void* krealloc(void* ptr, size_t size)
{
	size_t curSize = *(size_t*)(ptr-12);
	
	if (size < curSize)
	{
		*(size_t*)(ptr-12) = size;
		return ptr;
	}
	
	void* next = (void*)*(uint32_t*)(ptr-8);
	size_t nsize = *(size_t*)(next-12);
	
	while (next != 0x0 && nsize == 0)
	{
		next = (void*)*(uint32_t*)(next-8);
		nsize = *(size_t*)(next-12);
	}
	
	if (next == 0x0 || next - ptr > size + 12)
	{
		*(size_t*)(ptr-12) = size;
		*(size_t*)(ptr-8) += size - curSize;
		
		*(uint32_t*)(ptr+size+4) = (uint32_t)next;
		*(uint32_t*)(ptr+size+8) = (uint32_t)(ptr);
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

void dump_blocks(void* block)
{
	size_t size = *(size_t*)(block-12);
	void* next = (void*)*(uint32_t*)(block-8);
	void* prev = (void*)*(uint32_t*)(block-4);
	
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
	for (size_t i = 0; i < size; i++)
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
	dump_blocks((void*)(heap_begin + 12));
	
}

void dump_mem(void* block)
{
	size_t size = *(size_t*)(block-12);
	void* next = (void*)*(uint32_t*)(block-8);
	void* prev = (void*)*(uint32_t*)(block-4);
	
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
	
	kprint_color(GRAY_TEXT);
	for (size_t i = 0; i < size; i++)
	{
		char hexstr[16] = "";
		hex_to_ascii(*(char*)(block+i), hexstr);
		kprint(hexstr);
		kprint(" ");
	}
}
#endif
