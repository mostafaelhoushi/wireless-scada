//////////////////////////////////////////////////////////////////////////////
//
// SofTec Microsystems AK-S12NE64-A Demo Board Sample
//
// ---------------------------------------------------------------------------
// Copyright (c) 2004 SofTec Microsystems
// http://www.softecmicro.com/
//
//////////////////////////////////////////////////////////////////////////////
#include <hidef.h>
#include <string.h>

#include "debug.h"

#include "MC9S12NE64.h"
#include "main.h"


void eeprom_unlock(void);
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Instruction Set for serial EEPROM (M95010) 
#define EEPROM_CMD_WREN             0x06    // Write Enable
#define EEPROM_CMD_WRDI             0x04    // Write Disable
#define EEPROM_CMD_RDSR             0x05    // Read Status Register
#define EEPROM_CMD_WRSR             0x01    // Write Status Register
#define EEPROM_CMD_READ             0x03    // Read Data Bytes
#define EEPROM_CMD_WRITE            0x02    // Write Data Bytes

// Instruction Set for serial FLASH (M25P80)
#define FLASH_CMD_WREN              0x06    // Write Enable
#define FLASH_CMD_WRDI              0x04    // Write Disable
#define FLASH_CMD_RDSR              0x05    // Read Status Register
#define FLASH_CMD_WRSR              0x01    // Write Status Register
#define FLASH_CMD_READ              0x03    // Read Data Bytes
#define FLASH_CMD_PP                0x02    // Page Program
#define FLASH_CMD_FAST_READ         0x0B    // Read Data Bytes at Higher Speed
#define FLASH_CMD_SE                0xD8    // Sector Erase
#define FLASH_CMD_BE                0xC7    // Bulk Erase
#define FLASH_CMD_DP                0xB9    // Deep Power-down
#define FLASH_CMD_RES               0xAB    // Release from Deep Power-dowm

// Macro definitions
#define flash_cs_h()                PTS |= 0x80  
#define flash_cs_l()                PTS &= ~0x80                 
#define eeprom_cs_h()               PTL |= 0x40  
#define eeprom_cs_l()               PTL &= ~0x40  

#define spi_low_speed()             SPIBR = 0x02    // BaudRate = 3.125 MHz                
#define spi_high_speed()            SPIBR = 0x00    // BaudRate = 12.5 MHz                
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void init_serial_memories(void)
{
// SPI enabled, No interrupt, Master Mode, CPOL=0, CPHA=0, MSB first
SPICR1 = 0x50;
// MODFEN=0, Normal pin (MISO & MOSI)
SPICR2 = 0x00;
// BaudRate = 12.5 MHz 
SPIBR = 0x00;

// CS for serial FLASH
PTS |= 0x80;
DDRS |= 0x80;
// CS for serial EEPROM
PTL |= 0x40;
DDRL |= 0x40;

eeprom_unlock();
}

// ---------------------------------------------------------------------------
// Serial FLASH functions (1MByte)
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
unsigned char flash_spi(unsigned char data)
{
while(!(SPISR & 0x20))
    ;
SPIDR = data;
while(!(SPISR & 0x80))
    ;
return(SPIDR);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void flash_wre(void)
{
flash_cs_l();
(void)flash_spi(FLASH_CMD_WREN);
flash_cs_h();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void flash_wait_WIP(void)
{
flash_cs_l();
(void)flash_spi(FLASH_CMD_RDSR);
while(flash_spi(0x00) & 0x01)
    kick_WD();
flash_cs_h();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void flash_bulk_erase(void)
{
flash_wre();
flash_cs_l();
(void)flash_spi(FLASH_CMD_BE);
flash_cs_h();
flash_wait_WIP();
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void flash_program(unsigned long addr, int len, unsigned char (*flash_callback_write)(unsigned long))
{
int nblock;

while(len > 0)
    {
    nblock = (int)(256 - (addr & 0xFF));
    if(len < nblock)
        nblock = len;
    flash_wre();
    flash_cs_l();
    (void)flash_spi(FLASH_CMD_PP);
    (void)flash_spi((unsigned char)(addr>>16));
    (void)flash_spi((unsigned char)(addr>>8));
    (void)flash_spi((unsigned char)(addr>>0));
    len  -= nblock;
    while(nblock-- > 0)
        {
        kick_WD();
        (void)flash_spi(flash_callback_write(addr++));
        }
    flash_cs_h();
    flash_wait_WIP();
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void flash_read(unsigned long addr, int len, void (*flash_callback_read)(unsigned long, unsigned char))
{
flash_cs_l();
(void)flash_spi(FLASH_CMD_FAST_READ);
(void)flash_spi((unsigned char)(addr>>16));
(void)flash_spi((unsigned char)(addr>>8));
(void)flash_spi((unsigned char)(addr>>0));
(void)flash_spi(0x00);        // Dummy Byte
while(len-- > 0)
    {
    kick_WD();
    flash_callback_read(addr++, flash_spi(0x00));
    }
flash_cs_h();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void flash_read_buffer(unsigned long addr, unsigned char *buf, int len)
{
flash_cs_l();
(void)flash_spi(FLASH_CMD_FAST_READ);
(void)flash_spi((unsigned char)(addr>>16));
(void)flash_spi((unsigned char)(addr>>8));
(void)flash_spi((unsigned char)(addr>>0));
(void)flash_spi(0x00);        // Dummy Byte
while(len-- > 0)
    {
    kick_WD();
    *buf++ = flash_spi(0x00);
    }
flash_cs_h();
}

// ---------------------------------------------------------------------------
// Serial EEPROM functions (128 bytes)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
unsigned char eeprom_spi(unsigned char data)
{
while(!(SPISR & 0x20))
    ;
SPIDR = data;
while(!(SPISR & 0x80))
    ;
return(SPIDR);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void eeprom_wre(void)
{
eeprom_cs_l();
(void)eeprom_spi(EEPROM_CMD_WREN);
eeprom_cs_h();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void eeprom_wait_WIP(void)
{
eeprom_cs_l();
(void)eeprom_spi(EEPROM_CMD_RDSR);
while(eeprom_spi(0x00) & 0x01)
    kick_WD();
eeprom_cs_h();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void eeprom_unlock(void)
{
spi_low_speed(); 
eeprom_wre();
eeprom_cs_l();
(void)eeprom_spi(EEPROM_CMD_WRSR);
(void)eeprom_spi(0xF3);
eeprom_cs_h();

eeprom_wait_WIP();
spi_high_speed();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void eeprom_read_buffer(unsigned int addr, unsigned char *buf, int len)
{
spi_low_speed();     
eeprom_cs_l();
(void)eeprom_spi(EEPROM_CMD_READ | (unsigned char)((addr >> 5) & 0x08));
(void)eeprom_spi((unsigned char)addr);
while(len-- > 0)
    {
    kick_WD();
    *buf++ = eeprom_spi(0x00);
    }
eeprom_cs_h();
spi_high_speed();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void eeprom_write_buffer(unsigned int addr, unsigned char *buf, int len)
{
int nblock;

spi_low_speed();        // switch to low speed 
while(len > 0)
    {
    nblock = 16 - (addr & 0x0F);
    if(len < nblock)
        nblock = len;
    
    eeprom_wre();
    eeprom_cs_l();
    (void)eeprom_spi(EEPROM_CMD_WRITE | (unsigned char)((addr >> 5) & 0x08));
    (void)eeprom_spi((unsigned char)addr);
    len  -= nblock;
    while(nblock-- > 0)
        {
        kick_WD();
        (void)eeprom_spi(*buf++);
        addr++;
        }
    eeprom_cs_h();
    eeprom_wait_WIP();
    }
spi_high_speed();       // switch to high speed (for serial FLASH)
}

