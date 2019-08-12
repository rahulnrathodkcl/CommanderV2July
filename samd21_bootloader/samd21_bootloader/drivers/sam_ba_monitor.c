#include <asf.h>
#include <string.h>
#include "sam_ba_monitor.h"
#include "conf_board.h"
#include "main.h"
#include "lcd_driver.h"

const char RomBOOT_Version[] = SAM_BA_VERSION;


static const uint16_t crc16Table[256]=
{
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
	0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
	0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
	0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
	0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
	0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
	0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
	0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
	0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
	0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
	0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
	0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
	0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
	0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
	0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
	0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
	0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
	0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
	0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
	0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
	0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
	0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
	0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};



typedef struct
{
	/* send one byte of data */
	int (*putc)(int value);
	/* Get one byte */
	int (*getc)(void);
	/* Receive buffer not empty */
	bool (*is_rx_ready)(void);
	/* Send given data (polling) */
	uint32_t (*putdata)(void const* data, uint32_t length);
	/* Get data from comm. device */
	uint32_t (*getdata)(void* data, uint32_t length);
	/* Send given data (polling) using xmodem (if necessary) */
	uint32_t (*putdata_xmd)(void const* data, uint32_t length);
	/* Get data from comm. device using xmodem (if necessary) */
	uint32_t (*getdata_xmd)(void* data, uint32_t length);
} t_monitor_if;


const t_monitor_if usbcdc_if =
{ udi_cdc_putc, udi_cdc_getc, udi_cdc_is_rx_ready, udi_cdc_write_buf,
		udi_cdc_read_no_polling, udi_cdc_write_buf, udi_cdc_read_buf };


/* The pointer to the interface object use by the monitor */
t_monitor_if * ptr_monitor_if;

/* b_terminal_mode mode (ascii) or hex mode */
volatile bool b_terminal_mode = false;


/**
 * \brief This function initializes the SAM-BA monitor
 *
 * \param com_interface  Communication interface to be used.
 */
void sam_ba_monitor_init(uint8_t com_interface)
{

	if (com_interface == SAM_BA_INTERFACE_USBCDC)
		ptr_monitor_if = (t_monitor_if*) &usbcdc_if;
}


/**
 * \brief This function allows data rx by USART
 *
 * \param *data  Data pointer
 * \param length Length of the data
 */
void sam_ba_putdata_term(uint8_t* data, uint32_t length)
{
	uint8_t temp, buf[12], *data_ascii;
	uint32_t i, int_value;

	if (b_terminal_mode)
	{
		if (length == 4)
			int_value = *(uint32_t *) data;
		else if (length == 2)
			int_value = *(uint16_t *) data;
		else
			int_value = *(uint8_t *) data;

		data_ascii = buf + 2;
		data_ascii += length * 2 - 1;

		for (i = 0; i < length * 2; i++)
		{
			temp = (uint8_t) (int_value & 0xf);

			if (temp <= 0x9)
				*data_ascii = temp | 0x30;
			else
				*data_ascii = temp + 0x37;

			int_value >>= 4;
			data_ascii--;
		}
		buf[0] = '0';
		buf[1] = 'x';
		buf[length * 2 + 2] = '\n';
		buf[length * 2 + 3] = '\r';
		ptr_monitor_if->putdata(buf, length * 2 + 4);
	}
	else
		ptr_monitor_if->putdata(data, length);
	return;
}

volatile uint32_t sp;
/**
 * \brief Execute an applet from the specified address
 *
 * \param address Applet address
 */
void call_applet(uint32_t address)
{
	uint32_t app_start_address;

	cpu_irq_disable();

	sp = __get_MSP();

	/* Rebase the Stack Pointer */
	__set_MSP(*(uint32_t *) address);

	/* Load the Reset Handler address of the application */
	app_start_address = *(uint32_t *)(address + 4);

	/* Jump to application Reset Handler in the application */
	asm("bx %0"::"r"(app_start_address));
}


uint32_t current_number;
uint32_t i, length;
uint8_t cmd, *ptr_data, *ptr, data[SIZEBUFMAX];
uint8_t j;
uint32_t u32tmp;


uint32_t PAGE_SIZE, PAGES, MAX_FLASH;


//*----------------------------------------------------------------------------
//* \brief Compute the CRC
//*----------------------------------------------------------------------------
unsigned short serial_add_crc(char ptr, unsigned short crc)
{
	return (crc << 8) ^ crc16Table[((crc >> 8) ^ ptr) & 0xff];
}

static void put_uint32(uint32_t n)
{
	char buff[8];
	int i;
	for (i=0; i<8; i++)
	{
		int d = n & 0XF;
		n = (n >> 4);

		buff[7-i] = d > 9 ? 'A' + d - 10 : '0' + d;
	}
	ptr_monitor_if->putdata(buff,8);
}


/**
 * \brief This function starts the SAM-BA monitor.
 */

void sam_ba_monitor_run(void)
{
	uint32_t pageSizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
	PAGE_SIZE = pageSizes[NVMCTRL->PARAM.bit.PSZ];
	PAGES = NVMCTRL->PARAM.bit.NVMP;
	MAX_FLASH = PAGE_SIZE * PAGES;

	ptr_data = NULL;
	cmd = 'z';
	while (1)
	{
		sam_ba_monitor_loop();
	}
}


static void sam_ba_monitor_loop(void)
{
	length = ptr_monitor_if->getdata(data, SIZEBUFMAX);
	ptr = data;
	
	 for (i = 0; i < length; i++, ptr++)
	 {
		 if (*ptr == 0xff) continue;
		 
		 if (*ptr == '#')
		 {
			 if (b_terminal_mode)
			 {
				 ptr_monitor_if->putdata("\n\r", 2);
			 }
			 if (cmd == 'S')
			 {
				 //Check if some data are remaining in the "data" buffer
				 if(length>i)
				 {
					 //Move current indexes to next avail data (currently ptr points to "#")
					 ptr++;
					 i++;

					 //We need to add first the remaining data of the current buffer already read from usb
					 //read a maximum of "current_number" bytes
					 if ((length-i) < current_number)
					 {
						 u32tmp=(length-i);
					 }
					 else
					 {
						 u32tmp=current_number;
					 }

					 memcpy(ptr_data, ptr, u32tmp);
					 i += u32tmp;
					 ptr += u32tmp;
					 j = u32tmp;
				 }
				 //update i with the data read from the buffer
				 i--;
				 ptr--;
				 //Do we expect more data ?
				 if(j<current_number)
				ptr_monitor_if->getdata_xmd(ptr_data, current_number-j);

				 __asm("nop");
			 }
			 else if (cmd == 'R')
			 {
				 ptr_monitor_if->putdata_xmd(ptr_data, current_number);
			 }
			 else if (cmd == 'O')
			 {
				 *ptr_data = (char) current_number;
			 }
			 else if (cmd == 'H')
			 {
				 *((uint16_t *) ptr_data) = (uint16_t) current_number;
			 }
			 else if (cmd == 'W')
			 {
				 *((int *) ptr_data) = current_number;
			 }
			 else if (cmd == 'o')
			 {
				 sam_ba_putdata_term(ptr_data, 1);
			 }
			 else if (cmd == 'h')
			 {
				 current_number = *((uint16_t *) ptr_data);
				 sam_ba_putdata_term((uint8_t*) &current_number, 2);
			 }
			 else if (cmd == 'w')
			 {
				 current_number = *((uint32_t *) ptr_data);
				 sam_ba_putdata_term((uint8_t*) &current_number, 4);
			 }
			 else if (cmd == 'G')
			 {
				 call_applet(current_number);
				 /* Rebase the Stack Pointer */
				 __set_MSP(sp);
				 __enable_irq();
			 }
			 else if (cmd == 'T')
			 {
				b_terminal_mode = 1;
				ptr_monitor_if->putdata("\n\r", 2);
			 }
			 else if (cmd == 'N')
			 {
				 if (b_terminal_mode == 0)
				 {
					 ptr_monitor_if->putdata("\n\r", 2);
				 }
				 b_terminal_mode = 0;
			 }
			 else if (cmd == 'V')
			 {
				 ptr_monitor_if->putdata("v", 1);
				 ptr_monitor_if->putdata((uint8_t *) RomBOOT_Version,
				 strlen(RomBOOT_Version));
				 ptr_monitor_if->putdata(" ", 1);
				 ptr = (uint8_t*) &(__DATE__);
				 i = 0;
				 while (*ptr++ != '\0')
				 i++;
				 ptr_monitor_if->putdata((uint8_t *) &(__DATE__), i);
				 ptr_monitor_if->putdata(" ", 1);
				 i = 0;
				 ptr = (uint8_t*) &(__TIME__);
				 while (*ptr++ != '\0')
				 i++;
				 ptr_monitor_if->putdata((uint8_t *) &(__TIME__), i);
				 ptr_monitor_if->putdata("\n\r", 2);
			 }
			 
			 else if (cmd == 'J')
			 {
				 static uint32_t *src_buff_addr = NULL;				
				 if (current_number == 0)
				 {
					 // Set buffer address
					 src_buff_addr = (uint32_t*)ptr_data;
				 }
				 else
				 {
					 uint32_t size = current_number;
					 uint8_t *data = (uint8_t *)src_buff_addr;
					 uint32_t dst_address = (uint32_t*)ptr_data;				 
					 at25dfx_chip_write_buffer(&at25dfx_chip,dst_address,&data[0],size);
				 }
				 // Notify command completed
				 ptr_monitor_if->putdata("J\n\r", 3);
			 }
			 else if (cmd == 'K')
			 { 
				 for (uint32_t address=ptr_data;address<=current_number;(address=address+0x001000))
				 {
					 at25dfx_chip_erase_block(&at25dfx_chip,address,AT25DFX_BLOCK_SIZE_4KB);
				 }
				// Notify command completed				
				ptr_monitor_if->putdata("K\n\r", 3);
			 }
			 else if (cmd =='P')
			 {
				 uint32_t size = current_number;
				 uint32_t dst_address = (uint32_t*)ptr_data;
				 uint8_t page_buffer[size];
				 at25dfx_chip_read_buffer(&at25dfx_chip,dst_address,page_buffer,size);
				 ptr_monitor_if->putdata(page_buffer,size);
				 
			 }
			 
			 else if (cmd == 'I')
			 {
				 //if (ptr_data == 0)
				 {
					 uint8_t page_data[EEPROM_PAGE_SIZE];
					 eeprom_emulator_read_page((uint8_t)current_number, page_data);
					 ptr_monitor_if->putdata(page_data,EEPROM_PAGE_SIZE);
				 }
				 
			 }
			 else if (cmd == 'L')
			 {
				 static uint32_t *src_buff_addr = NULL; 
				 if (current_number == 0)
				 {
					 src_buff_addr = (uint32_t*)ptr_data;
				 }
				 else
				 {
					 uint32_t size = current_number;
					 uint8_t *databuff = (uint8_t *)src_buff_addr;
					 uint32_t dst_address = (uint32_t*)ptr_data;
					 
					  uint8_t page_data[EEPROM_PAGE_SIZE];
					  
					 memcpy(page_data,&databuff[0],size);
					 eeprom_emulator_write_page((uint8_t)dst_address, page_data);
					 eeprom_emulator_commit_page_buffer();
				 }
				 
				 ptr_monitor_if->putdata("L\n\r", 3);
				 
			 }
			 
			 else if (cmd == 'X')
			 {
				 // Syntax: X[ADDR]#
				 // Erase the flash memory starting from ADDR to the end of flash.

				 // Note: the flash memory is erased in ROWS, that is in block of 4 pages.
				 //       Even if the starting address is the last byte of a ROW the entire
				 //       ROW is erased anyway.

				 uint32_t dst_addr = current_number; // starting address
				 
				 while (dst_addr < MAX_FLASH)
				 {
					 // Execute "ER" Erase Row
					 NVMCTRL->ADDR.reg = dst_addr / 2;
					 NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
					 while (NVMCTRL->INTFLAG.bit.READY == 0)
					 ;
					 dst_addr += PAGE_SIZE * 4; // Skip a ROW
				 }

				 // Notify command completed
				 ptr_monitor_if->putdata("X\n\r", 3);
			 }
			 else if (cmd == 'Y')
			 {
				 // This command writes the content of a buffer in SRAM into flash memory.

				 // Syntax: Y[ADDR],0#
				 // Set the starting address of the SRAM buffer.

				 // Syntax: Y[ROM_ADDR],[SIZE]#
				 // Write the first SIZE bytes from the SRAM buffer (previously set) into
				 // flash memory starting from address ROM_ADDR

				 static uint32_t *src_buff_addr = NULL;

				 if (current_number == 0)
				 {
					 // Set buffer address
					 src_buff_addr = (uint32_t*)ptr_data;
				 }
				 else
				 {
					 // Write to flash
					 uint32_t size = current_number/4;
					 uint32_t *src_addr = src_buff_addr;
					 uint32_t *dst_addr = (uint32_t*)ptr_data;

					 // Set automatic page write
					 NVMCTRL->CTRLB.bit.MANW = 0;

					 // Do writes in pages
					 while (size)
					 {
						 // Execute "PBC" Page Buffer Clear
						 NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
						 while (NVMCTRL->INTFLAG.bit.READY == 0)
						 ;

						 // Fill page buffer
						 uint32_t i;
						 for (i=0; i<(PAGE_SIZE/4) && i<size; i++)
						 {
							 dst_addr[i] = src_addr[i];
						 }

						 // Execute "WP" Write Page
						 //NVMCTRL->ADDR.reg = ((uint32_t)dst_addr) / 2;
						 NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
						 while (NVMCTRL->INTFLAG.bit.READY == 0)
						 ;

						 // Advance to next page
						 dst_addr += i;
						 src_addr += i;
						 size     -= i;
					 }
				 }

				 // Notify command completed
				 ptr_monitor_if->putdata("Y\n\r", 3);
			 }
			 else if (cmd == 'Z')
			 {
				 // This command calculate CRC for a given area of memory.
				 // It's useful to quickly check if a transfer has been done
				 // successfully.

				 // Syntax: Z[START_ADDR],[SIZE]#
				 // Returns: Z[CRC]#

				 uint8_t *data = (uint8_t *)ptr_data;
				 uint32_t size = current_number;
				 uint16_t crc = 0;
				 uint32_t i = 0;
				 for (i=0; i<size; i++)
				 crc = serial_add_crc(*data++, crc);

				 // Send response
				 ptr_monitor_if->putdata("Z", 1);
				 put_uint32(crc);
				 ptr_monitor_if->putdata("#\n\r", 1);
			 }
			 
			 cmd = 'z';
			 current_number = 0;

			if (b_terminal_mode)
			{
				ptr_monitor_if->putdata(">", 1);
			}
		 }
		 else
		 {
			 if (('0' <= *ptr) && (*ptr <= '9'))
			 {
				 current_number = (current_number << 4) | (*ptr - '0');
			 }
			 else if (('A' <= *ptr) && (*ptr <= 'F'))
			 {
				 current_number = (current_number << 4) | (*ptr - 'A' + 0xa);
			 }
			 else if (('a' <= *ptr) && (*ptr <= 'f'))
			 {
				 current_number = (current_number << 4) | (*ptr - 'a' + 0xa);
			 }
			 else if (*ptr == ',')
			 {
				 ptr_data = (uint8_t *) current_number;
				 current_number = 0;
			 }
			 else
			 {
				 cmd = *ptr;
				 current_number = 0;
			 }
		 }
	 }
}
