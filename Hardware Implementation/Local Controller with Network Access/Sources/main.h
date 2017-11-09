#ifndef MAIN_H        /*prevent duplicated includes*/
#define MAIN_H

//==============================================================
//==============================================================
void AK_HCS12NE_Init(void);
void tcp_control_panel_init(void);     
void udp_control_panel_init(void); 
void udp_control_panel_run(void);

void init_serial_memories(void);
void flash_program(unsigned long addr, int len, unsigned char (*flash_callback_write)(unsigned long));
void flash_read(unsigned long addr, int len, void (*flash_callback_read)(unsigned long, unsigned char));
void flash_read_buffer(unsigned long addr, unsigned char *buf, int len);
void flash_bulk_erase(void);

void eeprom_read_buffer(unsigned int addr, unsigned char *buf, int len);
void eeprom_write_buffer(unsigned int addr, unsigned char *buf, int len);

//>>>>>
#define Bool    unsigned char


//==============================================================
//==============================================================
#define EE_ADDR_VERS            0
#define EE_ADDR_MAC             1
#define EE_ADDR_IP_ADDR         7
#define EE_ADDR_IP_SUBNET       11
#define EE_ADDR_IP_GATEWAY      15

#define EE_ADDR_DNS_ENABLE      19
#define EE_ADDR_IP_DNS          20


#endif /*MAIN_H*/