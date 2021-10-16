C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h)

OBJ = ${C_SOURCES:.c=.o cpu/interrupt.o}

all: os-image

run: os-image.bin
	qemu-system-i386 -hda os-image.bin
	
os-image: boot/bootsect.bin kernel.bin vdrive.bin
	cat $^ > os-image.bin
	
kernel.bin: boot/kernel_entry.o ${OBJ}
	ld -m elf_i386 -o $@ -Ttext 0x1000 $^ --oformat binary
	
vdrive.bin: vdrive.asm
	nasm $< -f bin -o $@
	
%.o : %.c ${HEADERS}
	gcc -m32 -fno-pie -ffreestanding -c $< -o $@

%.o : %.asm
	nasm $< -f elf32 -o $@

%.bin : %.asm
	nasm $< -f bin -I '../../16bit/' -o $@
	
clean:
	rm -fr *.bin *.dis *.o os-image
	rm -fr kernel/*.o boot/*.bin drivers/*.o cpu/*.o libc/*.o
