#include "keyboard.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "screen.h"
#include "../libc/string.h"
#include "../libc/function.h"
#include "../kernel/kernel.h"
#include <stdint.h>

#define BACKSPACE 0x0E
#define ENTER 0x1C
#define LSHIFT 0x2A
#define RSHIFT 0x36

static char key_buffer[256];

#define SC_MAX 57
/*const char *sc_name[] = { "ERROR", "Esc", "1", "2", "3", "4", "5", "6", 
    "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E", 
        "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl", 
        "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`", 
        "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", 
        "/", "RShift", "Keypad *", "LAlt", "Spacebar"};*/
const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '?', '?', 'q', 'w', 'e', 'r', 't', 'y', 
        'u', 'i', 'o', 'p', '[', ']', '?', '?', 'a', 's', 'd', 'f', 'g', 
        'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'z', 'x', 'c', 'v', 
        'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' '};

#define SHIFT_FLAG 1
char inputFlags = 0;

char get_ascii(uint8_t scancode)
{
	char letter = sc_ascii[(int)scancode];
	if (inputFlags & SHIFT_FLAG)
	{
		switch(letter)
		{
			case '1':
				letter = '!';
				break;
			case '2':
				letter = '@';
				break;
			case '3':
				letter = '#';
				break;
			case '4':
				letter = '$';
				break;
			case '5':
				letter = '%';
				break;
			case '6':
				letter = '^';
				break;
			case '7':
				letter = '&';
				break;
			case '8':
				letter = '*';
				break;
			case '9':
				letter = '(';
				break;
			case '0':
				letter = ')';
				break;
			case '-':
				letter = '_';
				break;
			case '=':
				letter = '+';
				break;
			case '`':
				letter = '~';
				break;
			case '[':
				letter = '{';
				break;
			case ']':
				letter = '}';
				break;
			case '\\':
				letter = '|';
				break;
			case ';':
				letter = ':';
				break;
			case '\'':
				letter = '"';
				break;
			case ',':
				letter = '<';
				break;
			case '.':
				letter = '>';
				break;
			case '/':
				letter = '?';
				break;	
			default:
				if (letter >= 'a' && letter <= 'z')
				{
					letter -= 32;
				}		
		}
	}
	return letter;
}

static void keyboard_callback(registers_t *regs) {
    /* The PIC leaves us the scancode in port 0x60 */
    uint8_t scancode = port_byte_in(0x60);
    
    switch (scancode)
    {
    	case BACKSPACE:
    		backspace(key_buffer);
        	kprint_backspace();
        	break;
        case ENTER:
        	kprint("\n");
        	user_input(key_buffer); /* kernel-controlled function */
        	key_buffer[0] = '\0';
        	break;
        case LSHIFT:
        case RSHIFT:
        case LSHIFT + 0x80:
        case RSHIFT + 0x80:
        	inputFlags ^= SHIFT_FLAG;
        	break;
        default:
        {
        	if (scancode > SC_MAX) return;
        	char letter = get_ascii(scancode);
        	/* Remember that kprint only accepts char[] */
        	char str[2] = {letter, '\0'};
        	append(key_buffer, letter);
        	kprint(str);
        }
    }	
    UNUSED(regs);
}

void init_keyboard() {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}
