#include "filesystem.h"

#include "../libc/mem.h"
#include "../libc/string.h"
#include "../drivers/ata.h"
#include "../drivers/screen.h"

//we wil be using relative lba (starting at kernel_end)
const uint16_t kernel_end = 32; //this is a constant predefined value in bootsect.asm + 1
const uint16_t fs_begin = kernel_end + 4; // 4 sectors are reserved for fs table

/* JFS (Jesse File System)

// JFS table (on disk)
the first byte represents how many direct children the root directory has (uint8_t)
the next n bytes are the offsets for where the children meta is stored as (uint16_t).

the first byte of a child represents the file type (f)
0 | Folder
1 | Text
2 | ...

the next bytes differs depending on the file type
f0 second byte is number of children (uint8_t)
f0 next n bytes is folder name (ptr to str on heap)
f0 next n bytes are children meta offsets (uint16_t)

f1 second byte is lba of file (uint32_t)
f1 next 4 bytes is the file size in bytes (uint32_t)
f1 next n bytes is file name (ptr to str on heap)
//file size in bytes can be used to get sector count

f2 - f255 currently undefined

// JFS table (on heap)
The heap implementation is similar but has a few differences
the first being that the size will not always be 512 * 4 bytes
the second being that instead of offsets, addresses are used (uint32_t)
the third is that the address of the parent is stored in the child (uint32_t) (as the 2nd element)
the fourth being that root is marked as a folder instead of nothing (uint8_t)
the heap table is initialized using the disk table

the tree can be navigated using a byte array.
each byte represents which child to go to.
if a child does not exist (parent is file or folder doesn't span that far)
then the parent is returned (ex. 0x00 0xff would only take you into the first child of root)
root can technically be accessed with 0xff but that won't work if root has 255 children.
instead root() can be used
*/

void* fs_root;
void* fs_current;

void ls()
{
	void* nameloc = (void*)*(uint32_t*)(fs_current + 6);
	kprint("Currently in ");
	kprint_color(LBLUE_TEXT);
	
	kprint(nameloc);
	
	void* parent = (void*)*(uint32_t*)(fs_current + 1);
	while (parent != 0x0) //Yes, the full path is reversed
	{
		nameloc = (void*)*(uint32_t*)(parent + 6);
		kprint("-");
		kprint(nameloc);
		parent = (void*)*(uint32_t*)(parent + 1);
	}
	
	kprint(".\n");
	
	uint8_t children = *(uint8_t*)(fs_current + 5);
	kprint_color(GRAY_TEXT);
	if (children == 0)
	{
		kprint("no children :(");
		kprint("\n");
	}
	
	for (uint16_t i = 0; i < children; i++)
	{
		char idx[4] = "";
		int_to_ascii(i, idx);
		kprint(idx);
		kprint(" : ");
		
		void* childloc = (void*)*(uint32_t*)(fs_current + 10 + i * 4);
		uint8_t childtype = *(uint8_t*)(childloc);
		switch (childtype)
		{
			case 0:
				nameloc = (void*)*(uint32_t*)(childloc + 6);
				break;
			case 1:
				nameloc = (void*)*(uint32_t*)(childloc + 13);
				break;
		}
		kprint(nameloc);
		kprint("\n");
	}
	kprint_color(WHITE_ON_BLACK);
	
	void (*funPtr)() = (void*)*(uint32_t*)(nameloc);
}

void* alloc_name(void* loc)
{
	int namelen = strlen(loc);
	void* nameloc = kmalloc(namelen+1);
	memcpy(loc, nameloc, namelen+1);
	return nameloc;
}

void* create_filesystem(void* nodeptr, void* parent)
{
	const uint8_t type = *(uint8_t*)nodeptr;
	switch (type)
	{
		case 0:
		{
			const uint8_t children = *(uint8_t*)(nodeptr+1);
			
			void* node = kmalloc(10 + 4 * children);
			*(uint8_t*)(node) = type;
			*(uint32_t*)(node + 1) = (uint32_t)parent;
			*(uint8_t*)(node + 5) = children;
			*(uint32_t*)(node + 6) = (uint32_t)alloc_name(nodeptr + 2);
			int slen = strlen(nodeptr + 2) + 1;
			
			for (uint16_t i = 0; i < children; i++)
			{
				uint16_t coffset = *(uint16_t*)(nodeptr+1+slen+i*2);
				*(uint32_t*)(node + 10 + i*4) = (uint32_t)create_filesystem(nodeptr + coffset, node);
			}
			return node;
		}
		case 1:
		{
			void* node = kmalloc(17);
			*(uint8_t*)(node) = type;
			*(uint32_t*)(node + 1) = (uint32_t)parent;
			*(uint32_t*)(node + 5) = *(uint32_t*)(nodeptr+1); //file lba
			*(uint32_t*)(node + 9) = *(uint32_t*)(nodeptr+5); //file size (bytes NOT sectors)
			*(uint32_t*)(node + 13) = (uint32_t)alloc_name(nodeptr + 9); //file name
			
			return node;
		}
	}
}

void init_filesystem()
{
	void* buffer = kmalloc(512 * 4);
	lba_read(kernel_end, 4, buffer);
	
	uint8_t children = *(uint8_t*)buffer;
	
	//7 bytes header, 4*children bytes data
	void* root = kmalloc(7 + 4 * children);
	*(uint8_t*)(root) = 0x0; 		 //type
	*(uint32_t*)(root + 1) = 0x00; 	 //parent
	*(uint8_t*)(root + 5) = children;//children
	
	void* name = kmalloc(5);
	*(char*)(name) = 'r';
	*(char*)(name+1) = 'o';
	*(char*)(name+2) = 'o';
	*(char*)(name+3) = 't';
	*(char*)(name+4) = '\0';
	
	*(uint32_t*)(root + 6) = (uint32_t)name; 		 //filename
	
	for (uint16_t i = 0; i < children; i++) //children addresses
	{
		uint16_t coffset = *(uint16_t*)(buffer + 1 + i*2);
		*(uint32_t*)(root + 7 + i*4) = (uint32_t)create_filesystem(buffer + coffset, root);
	}
	
	fs_root = root;
	fs_current = root;
	
	kfree(buffer);
}
