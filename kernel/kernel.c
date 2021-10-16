#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "kernel.h"
#include "../drivers/ata.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include <stdint.h>

void kernel_main() {
	
	clear_screen();
	kprint_color(GRAY_TEXT);
	kprint_at("Welcome To JesseOS!", 30, 1);
	kprint_color(TEAL_TEXT);
	kprint_at("Initializing isr...", 0, 2);
    isr_install();
    kprint_at("Initializing irq...", 0, 3);
    irq_install();
    kprint_at("Initializing heap...", 0, 4);
    initialize_heap(0x200000);
    kprint_at("Initializing ata... ", 0, 5);
    initialize_ata();
    kprint_color(GREEN_TEXT);
    kprint_at("Initialized! ", 0, 6); 
    kprint_color(DGRAY_TEXT);
    kprint("Type END to exit");
    kprint_color(WHITE_ON_BLACK);
    
    kprint("\n> ");
}

void parse_shell_command(char* input)
{
	char* tmp = input;
	size_t args = 0;
	while (*input != '\0')
	{
		if (*input == ' ')
		{
			*input = '\0';
			args++;
		}
		input++;
	}
	input = tmp;

	if (strcmp(input, "END") == 0) {
    	kprint_color(RED_TEXT);
        kprint("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    }
    else if (strcmp(input, "kclear") == 0)
    {
    	clear_screen();
    }
    else if (strcmp(input, "dumph") == 0)
    {
    	#ifdef HEAP_DEBUG
    	dump_heap();
    	kprint_color(WHITE_ON_BLACK);
    	#endif
    }
    else if (strcmp(input, "malloc") == 0)
    {
    	int ipt = stoi(input+7);
    	if (ipt <= 0) return;
    	char adr[16] = "";
    	hex_to_ascii((uint32_t)kmalloc(ipt), adr);
    	kprint("Allocated at ");
    	kprint_color(RED_TEXT);
    	kprint(adr);
    	kprint("\n");
    	kprint_color(WHITE_TEXT);
    }
    else if (strcmp(input, "diskr") == 0)
    {
    	int lba = stoi(input+6);
    	
    	uint8_t sectors = 2;
    	uint8_t* buf = kmalloc(512 * sectors);
    	lba_read(lba, sectors, buf);
    	kfree(buf);
    	
    }
}

void user_input(char *input) {
    
    parse_shell_command(input);
    kprint("> ");
}
