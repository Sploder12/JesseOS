#include "filesystem.h"

#include "../libc/mem.h"
#include "../libc/string.h"
#include "../drivers/ata.h"
#include "../drivers/screen.h"

#define FS_TABLE_SECTORS 4

//we wil be using relative lba (starting at kernel_end)
const uint16_t kernel_end = 48; //this is a constant predefined value in bootsect.asm + 1
const uint16_t fs_begin = kernel_end + FS_TABLE_SECTORS; // 4 sectors are reserved for fs table

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

void create_folder(char name[])
{
	int namel = strlen(name) + 1;
	char* pname = kmalloc(namel);
	strcpy(name, pname);
	
	void* newfolder = kmalloc(10);
	*(uint8_t*)(newfolder) = 0;    //type
	*(uint8_t*)(newfolder + 5) = 0;//child cnt
	*(uint32_t*)(newfolder + 6) = (uint32_t)pname;
	
	
	uint8_t oFolderCnt = *(uint8_t*)(fs_current + 5);
	void* rParent = krealloc(fs_current, 14 + oFolderCnt*4); //add 4 bytes to parent
	
	*(uint32_t*)(newfolder + 1) = (uint32_t)rParent;
	*(uint32_t*)(rParent + 10 + oFolderCnt*4) = (uint32_t)newfolder;
	*(uint8_t*)(rParent + 5) += 1;
	
	//update parent's folders parents
	for (uint16_t i = 0; i < oFolderCnt; i++)
	{
		void* oFolder = (void*)*(uint32_t*)(rParent + 10 + i*4);
		*(uint32_t*)(oFolder+1) = (uint32_t)rParent;
	}
	
	if (fs_current == fs_root)
	{
		fs_root = rParent;
	}
	else
	{
		//update location for parent of parent
		void* parentParent = (void*)*(uint32_t*)(rParent+1);
		uint8_t uncleCnt = *(uint8_t*)(parentParent + 5);
		
		for (uint16_t i = 0; i < uncleCnt; i++)
		{
			if (fs_current == (void*)*(uint32_t*)(parentParent + 10 + i*4))
			{
				*(uint32_t*)(parentParent + 10 + i*4) = (uint32_t)rParent;
				break;
			}
		}
		
	}
	fs_current = rParent;
}

uint16_t save_node(void* node, void* buffer)
{
	uint8_t type = *(uint8_t*)(node);
	*(uint8_t*)(buffer) = type;
	
	switch (type)
	{
		case 0:
		{
			uint8_t childrenNum = *(uint8_t*)(node + 5);
			*(uint8_t*)(buffer + 1) = childrenNum;
			
			int nlen = strlen((void*)*(uint32_t*)(node + 6)) + 1;
			
			
			void* str = (void*)*(uint32_t*)(node + 6);

			strcpy(str, buffer+2);
			
			uint16_t coff = 2 + nlen + childrenNum*2;
			
			for (uint16_t i = 0; i < childrenNum; i++)
			{
				*(uint16_t*)(buffer + 2 + nlen + i*2) = coff;
				void* child = (void*)*(uint32_t*)(node + 10 + i* 4);
				
				coff += save_node(child, buffer + coff);
			}
			return coff;
			
		}
		case 1:
		{
			*(uint32_t*)(buffer + 1) = *(uint32_t*)(node + 5);
			*(uint32_t*)(buffer + 5) = *(uint32_t*)(node + 9);
			
			uint16_t nlen = strlen((char*)(node + 13)) + 1;
			strcpy((char*)*(uint32_t*)(node+13), buffer+9);
			return nlen + 9;
		}
	}
}

void save_state()
{
	//step one: allocate buffer	
	//assume that the tree won't exceed the reserved size
	void* buffer = kmalloc(512 * FS_TABLE_SECTORS); 

	//step two: write data
	uint8_t childrenNum = *(uint8_t*)(fs_root + 5);
	*(uint8_t*)(buffer) = childrenNum;
				
	uint16_t coff = 1 + childrenNum*2;
			
	for (uint16_t i = 0; i < childrenNum; i++)
	{
		*(uint16_t*)(buffer + 1 + i*2) = coff;
		void* child = (void*)*(uint32_t*)(fs_root + 10 + i * 4);
				
		coff += save_node(child, buffer + coff);
	}
	
	//step three: flush buffer
	lba_write(kernel_end, FS_TABLE_SECTORS, buffer);
	
	//step four: free buffer
	kfree(buffer);
}

void ls()
{
	void* nameloc = (void*)*(uint32_t*)(fs_current + 6);
	kprint("Currently in ");
	kprint_color(LBLUE_TEXT);
	
	kprint(nameloc);
	
	void* parent = (void*)*(uint32_t*)(fs_current + 1);
	while (parent != 0x0) //Yes, the full path is reversed
	{
		nameloc = (char*)*(uint32_t*)(parent + 6);
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
		kprint("    ");
	}
	kprint_color(WHITE_ON_BLACK);
	kprint("\n");
	
	void (*funPtr)() = (void*)*(uint32_t*)(nameloc);
}

void cd(char dir[])
{
	if (strcmp(dir, "..") == 0 && fs_current != fs_root)
	{
		fs_current = (void*)*(uint32_t*)(fs_current+1);
		return;
	}

	uint8_t children = *(uint8_t*)(fs_current + 5);
	for (uint16_t i = 0; i < children; i++)
	{

		void* childloc = (void*)*(uint32_t*)(fs_current + 10 + i * 4);
		uint8_t childtype = *(uint8_t*)(childloc);
		if(childtype == 0 && strcmp((char*)*(uint32_t*)(childloc+6), dir) == 0)
		{
			fs_current = childloc;
			return;
		}
	}
	
	int idx = stoi(dir);
	if (idx >= children || idx < 0)
	{
		kprint_color(RED_TEXT);
		kprint("No such directory.\n");
		kprint_color(WHITE_ON_BLACK);
		return;
	}
	
	fs_current = (void*)*(uint32_t*)(fs_current+10+(idx*4));
}

void* alloc_name(void* loc)
{
	int namelen = strlen(loc) + 1;
	void* nameloc = kmalloc(namelen);
	strcpy(loc, nameloc);
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
				uint16_t coffset = *(uint16_t*)(nodeptr+2+slen+i*2);
				
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

	void* buffer = kmalloc(512 * FS_TABLE_SECTORS);
	
	lba_read(kernel_end, FS_TABLE_SECTORS, buffer);
	
	uint8_t children = *(uint8_t*)buffer;
	
	void* name = kmalloc(5);
	*(char*)(name) = 'r';
	*(char*)(name+1) = 'o';
	*(char*)(name+2) = 'o';
	*(char*)(name+3) = 't';
	*(char*)(name+4) = '\0';
	
	//10 bytes header, 4*children bytes data
	void* root = kmalloc(10 + 4 * children);
	*(uint8_t*)(root) = 0x0; 		 //type
	*(uint32_t*)(root + 1) = 0x00; 	 //parent
	*(uint8_t*)(root + 5) = children;//children
	
	*(uint32_t*)(root + 6) = (uint32_t)name; 		 //filename
	
	
	for (uint16_t i = 0; i < children; i++) //children addresses
	{
		uint16_t coffset = *(uint16_t*)(buffer + 1 + i*2);
		*(uint32_t*)(root + 10 + i*4) = (uint32_t)create_filesystem(buffer + coffset, root);
	}
	
	kfree(buffer);
	
	fs_root = root;
	fs_current = root;
	
}
