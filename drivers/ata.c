#include "ata.h"

#include "../cpu/ports.h"
#include "../cpu/isr.h"

#include "screen.h"
#include "../libc/mem.h"

const uint16_t io_base = 0x01f0;

uint8_t* identify_buf = 0x0;

#define ATA_PRIMARY_IO 0x1f0
#define ATA_SECONDARY_IO 0x170

#define ATA_PRIMARY 0x0
#define ATA_SECONDARY 0x01

void ide_select_drive(uint8_t bus, uint8_t i)
{
	if(bus == ATA_PRIMARY)
		port_byte_out(ATA_PRIMARY_IO + ATA_DRIVE_HEAD_REG, i);
	else
		port_byte_out(ATA_SECONDARY_IO + ATA_DRIVE_HEAD_REG, i);
}

void ata_400ns_delay()
{
	for(int i = 0;i < 4; i++)
		port_byte_in(io_base + ATA_ALT_STATUS_REG);
}

uint16_t timeout = 0;
#define TIMEOUT 1000 //this amount of timeout seems to work
void ata_poll(uint16_t io)
{
	timeout = 0;
	
	ata_400ns_delay();
	uint8_t status;
	do
	{
		status = port_byte_in(io + ATA_STATUS_REG);
		if (timeout++ > TIMEOUT)
		{
			kprint("ATA timeout!\n");
			return;
		}
	} while (status & ATA_SR_BSY);
	
	timeout = 0;
	do
	{
		status = port_byte_in(io + ATA_STATUS_REG);
		if (status & ATA_SR_ERR)
		{
			kprint("ATA error!\n");
		}
		
		if (timeout++ > TIMEOUT)
		{
			kprint("ATA timeout!\n");
			return;
		}
	} while(!(status & ATA_SR_DRQ));
}

uint8_t ata_identify(uint8_t bus, uint8_t drive)
{
	
	uint16_t io = 0;
	ide_select_drive(bus, drive);
	if(bus == ATA_PRIMARY) io = ATA_PRIMARY_IO;
	else io = ATA_SECONDARY_IO;
	
	port_byte_out(io + ATA_SECTOR_COUNT_REG, 0);
	port_byte_out(io + ATA_SECTOR_REG, 0);
	port_byte_out(io + ATA_CYLINDER_LOW_REG, 0);
	port_byte_out(io + ATA_CYLINDER_HIGH_REG, 0);
	
	port_byte_out(io + ATA_CMD_REG, ATA_CMD_IDENTIFY);
	
	uint8_t status = port_byte_in(io + ATA_STATUS_REG);
	if (status)
	{
		ata_poll(io);
		if (timeout > TIMEOUT)
		{
			return 0;
		}
		
		for (uint16_t i = 0; i < 256; i++)
		{
			*(uint16_t*)(identify_buf + i*2) = port_word_in(io + ATA_DATA_REG);
		}
		return 1;
	}
	return 0;
}

void lba_read_one(uint32_t lba, uint8_t* buffer)
{
	uint8_t drive =  0xE0;
	port_byte_out(io_base + ATA_DRIVE_HEAD_REG, drive | (uint8_t)(lba >> 23 & 0x0f)); //select the drive
	
	port_byte_out(io_base + ATA_FEATURES_REG, 0x00);
	
	port_byte_out(io_base + ATA_SECTOR_COUNT_REG, 1);
	
	port_byte_out(io_base + ATA_SECTOR_REG, (uint8_t)lba);
	port_byte_out(io_base + ATA_CYLINDER_LOW_REG, (uint8_t)(lba >> 8));
	port_byte_out(io_base + ATA_CYLINDER_HIGH_REG, (uint8_t)(lba >> 16));
	
	port_byte_out(io_base + ATA_CMD_REG, ATA_CMD_READ_PIO); //Read with retry
	
	ata_poll(io_base);
	if (timeout > TIMEOUT)
	{
		return;
	}

	for (uint16_t i = 0; i < 256; i++)
	{
		uint16_t data = port_word_in(io_base + ATA_DATA_REG);
		*(uint16_t*)(buffer + i * 2) = data;
	}
	ata_400ns_delay();
}

void lba_read(uint32_t lba, uint32_t sectors, uint8_t* buffer)
{
	for (uint32_t i = 0; i < sectors; i++)
	{
		lba_read_one(lba + i, buffer);
		if (timeout > TIMEOUT)
		{
			i--; //retry on timeout
		}
		else
		{
			buffer += 512;
		}
	}
}

void ata_probe()
{
	kprint_color(GREEN_TEXT);
	if(ata_identify(ATA_PRIMARY, ATA_MASTER))
	{
		kprint("Master ata drive exists! ");	
	}
	else
	{
		kprint_color(RED_TEXT);
		kprint("Master ata drive doesn't exist!!! Do not use disk Read/Write! ");	
	}
	
	if(ata_identify(ATA_PRIMARY, ATA_SLAVE))
	{
		kprint("Slave ata drive exists! ");
	}
	kprint_color(WHITE_TEXT);
}

void primary_irq()
{
	port_byte_out(0xA0, 0x20);
	port_byte_out(0x20, 0x20);
}

void secondary_irq()
{
	port_byte_out(0xA0, 0x20);
	port_byte_out(0x20, 0x20);
}

void initialize_ata()
{
	identify_buf = kmalloc(512);
	register_interrupt_handler(IRQ14, primary_irq); 
	register_interrupt_handler(IRQ15, secondary_irq); 
	ata_probe();
	kfree(identify_buf);
}
