#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0

// Status register bitmasks
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

//returned error values from the Status/Command regs
#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

// Status/Command port
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

// values returned by ATA identify command
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

// Registers (offset from I/O base port which is normally 0x1f0)
#define ATA_DATA_REG 0
#define ATA_ERROR_REG 1 //read
#define ATA_FEATURES_REG 1 //write
#define ATA_SECTOR_COUNT_REG 2
#define ATA_SECTOR_REG 3
#define ATA_CYLINDER_LOW_REG 4
#define ATA_CYLINDER_HIGH_REG 5
#define ATA_DRIVE_HEAD_REG 6 //can set both drive and head with this
#define ATA_STATUS_REG 7 //read
#define ATA_CMD_REG 7 //write

// Control base port Registers
#define ATA_ALT_STATUS_REG 0 //read
#define ATA_DEVICE_CONTROL_REG 0 //write
#define ATA_DRIVE_ADDR_REG 1 //read

void lba_read(uint32_t lba, uint32_t sectors, uint8_t* buffer);
void lba_read_one(uint32_t lba, uint8_t* buffer);

void initialize_ata();
#endif
