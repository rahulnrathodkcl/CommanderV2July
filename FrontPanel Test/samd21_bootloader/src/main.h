#ifndef _MAIN_H_
#define _MAIN_H_

#include <asf.h>

#include "usb_protocol_cdc.h"

struct spi_module at25dfx_spi;
struct at25dfx_chip_module at25dfx_chip;

//////////////////////////////////////////////////////////////////////////
// PAGE LOCATIONS::::::::::::
//////////////////////////////////////////////////////////////////////////
#define BOOTLOADER_PARAMETER_PAGE		0
#define FTP_PATH_PARAMETER_PAGE			1
#define FTP_USER_PARAMETER_PAGE			2

//////////////////////////////////////////////////////////////////////////


//#define USB_ID
#define USB_TARGET_DP_PIN            PIN_PA25G_USB_DP
#define USB_TARGET_DP_MUX            MUX_PA25G_USB_DP
#define USB_TARGET_DP_PINMUX         PINMUX_PA25G_USB_DP
#define USB_TARGET_DM_PIN            PIN_PA24G_USB_DM
#define USB_TARGET_DM_MUX            MUX_PA24G_USB_DM
#define USB_TARGET_DM_PINMUX         PINMUX_PA24G_USB_DM
#define USB_VBUS_PIN                 PIN_PA21
#define USB_VBUS_EIC_LINE            5
#define USB_VBUS_EIC_MUX             MUX_PA21A_EIC_EXTINT5
#define USB_VBUS_EIC_PINMUX          PINMUX_PA21A_EIC_EXTINT5
//#define USB_ID_PIN                   PIN_PA13
//#define USB_ID_EIC_LINE              13
//#define USB_ID_EIC_MUX               MUX_PA13A_EIC_EXTINT13
//#define USB_ID_EIC_PINMUX            PINMUX_PA13A_EIC_EXTINT13


bool usb_program_memory(void);
void spiflash_init(void);
void start_application(void);
void usb_firmware_download(void);
void check_boot_mode(void);
char download_firmware_into_flash(uint32_t firmware_size);
bool program_memory(uint32_t firmware_size);


void main_suspend_lpm_action(void);
void main_remotewakeup_lpm_enable(void);
void main_remotewakeup_lpm_disable(void);
bool main_cdc_enable(uint8_t port);
void main_cdc_disable(uint8_t port);
void main_cdc_set_dtr(uint8_t port, bool b_enable);
void main_cdc_rx_notify(uint8_t port);
void main_cdc_set_coding(uint8_t port, usb_cdc_line_coding_t * cfg);


#endif // _MAIN_H_
