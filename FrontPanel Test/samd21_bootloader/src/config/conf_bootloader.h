#ifndef CONF_BOOTLOADER_H_
#define CONF_BOOTLOADER_H_

#define LCD_SUPPORT
#define FTP_MODE_BOOT_SUPPORT
#define USB_DEVIDE_MODE_BOOT_SUPPORT


//! Enable/disable the CRC check
#define FIRMWARE_CRC_CHECK           ENABLE
//! Enable/disable the Console message output
#define CONSOLE_OUTPUT_ENABLED       ENABLE/*DISABLE*/

//! Firmware file input name
#define FIRMWARE_IN_FILE_NAME        "0:firmware.bin"

//! Application starting offset - Verify with boot loader footprint*/
#define APP_START_OFFSET             0x10000
//! Application starting address in Flash
#define APP_START_ADDRESS            (FLASH_ADDR + APP_START_OFFSET)
//! Maximum possible size of the Application
#define APP_MAX_SIZE                 (FLASH_ADDR + FLASH_SIZE  \
- APP_START_ADDRESS)
//! Buffer size to be used for programming
#define FLASH_BUFFER_SIZE            (FLASH_PAGE_SIZE)

//! Offset for the firmware in the input application binary - 16 bytes
//#define APP_BINARY_OFFSET            (APP_CRC_SIZE + APP_SIGNATURE_SIZE)
//! CRCCU Polynomial Selection
#define APP_CRC_POLYNOMIAL_TYPE      CRC_TYPE_32 //32-bit CRC


//Maximum firmware size in byte
//used for allocation space in SPI FLASH in byte
//200kb = 200*1024 = 204800 = 0x32000

#define MAX_FIRMWARESIZE			   (0x32000-1)

#define FIRMWARE_START_ADDRESS_IN_FLASH 0x0000

#define SPI_FLASH_ID  0xEF



#endif /* CONF_BOOTLOADER_H_ */