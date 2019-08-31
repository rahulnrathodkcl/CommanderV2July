
#ifndef SAM_BA_MONITOR_H_
#define SAM_BA_MONITOR_H_

#define SAM_BA_VERSION              "2.16"

/* Selects USART as the communication interface of the monitor */
#define SAM_BA_INTERFACE_USART      1
/* Selects USB as the communication interface of the monitor */
#define SAM_BA_INTERFACE_USBCDC     0

/* Selects USB as the communication interface of the monitor */
#define SIZEBUFMAX                  64

/**
 * \brief Initialize the monitor
 *
 */
void sam_ba_monitor_init(uint8_t com_interface);

/**
 * \brief Main function of the SAM-BA Monitor
 *
 */
void sam_ba_monitor_run(void);

/**
 * \brief
 */
void sam_ba_putdata_term(uint8_t* data, uint32_t length);

/**
 * \brief
 */
void call_applet(uint32_t address);

static void sam_ba_monitor_loop(void);
unsigned short serial_add_crc(char ptr, unsigned short crc);
static void put_uint32(uint32_t n);


#endif