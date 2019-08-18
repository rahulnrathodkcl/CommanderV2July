#include <asf.h>
#include "eeprom_driver.h"
#include "gsm_service.h"
#include "motor_service.h"
#include "lcd_service.h"

#define sleepWaitTime 18000

//#define sleepWaitTime 2000

#define CONF_MASTER_SPI_MODULE  EXT2_SPI_MODULE
#define CONF_MASTER_SS_PIN      EXT2_PIN_SPI_SS_0
#define CONF_MASTER_MUX_SETTING EXT2_SPI_SERCOM_MUX_SETTING
#define CONF_MASTER_PINMUX_PAD0 EXT2_SPI_SERCOM_PINMUX_PAD0
#define CONF_MASTER_PINMUX_PAD1 PINMUX_UNUSED
#define CONF_MASTER_PINMUX_PAD2 EXT2_SPI_SERCOM_PINMUX_PAD2
#define CONF_MASTER_PINMUX_PAD3 EXT2_SPI_SERCOM_PINMUX_PAD3


#define CONF_PERIPHERAL_TRIGGER_TX   SERCOM1_DMAC_ID_TX
#define CONF_PERIPHERAL_TRIGGER_RX   SERCOM0_DMAC_ID_RX
#define BUF_LENGTH 20
#define TEST_SPI_BAUDRATE             1000000UL
#define SLAVE_SELECT_PIN CONF_MASTER_SS_PIN

static const uint8_t buffer_tx[BUF_LENGTH] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
	0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
};
static uint8_t buffer_rx[BUF_LENGTH];

struct spi_module spi_master_instance;
struct spi_module spi_slave_instance;

static volatile bool transfer_tx_is_done = false;
static volatile bool transfer_rx_is_done = false;

struct spi_slave_inst slave;

COMPILER_ALIGNED(16)
DmacDescriptor example_descriptor_tx SECTION_DMAC_DESCRIPTOR;
DmacDescriptor example_descriptor_rx SECTION_DMAC_DESCRIPTOR;

static void transfer_tx_done(struct dma_resource* const resource )
{
	transfer_tx_is_done = true;
}
static void transfer_rx_done(struct dma_resource* const resource )
{
	transfer_rx_is_done = true;
}
static void configure_dma_resource_tx(struct dma_resource *tx_resource)
{
	struct dma_resource_config tx_config;
	dma_get_config_defaults(&tx_config);
	tx_config.peripheral_trigger = CONF_PERIPHERAL_TRIGGER_TX;
	tx_config.trigger_action = DMA_TRIGGER_ACTION_BEAT;
	dma_allocate(tx_resource, &tx_config);
}
static void configure_dma_resource_rx(struct dma_resource *rx_resource)
{
	struct dma_resource_config rx_config;
	dma_get_config_defaults(&rx_config);
	rx_config.peripheral_trigger = CONF_PERIPHERAL_TRIGGER_RX;
	rx_config.trigger_action = DMA_TRIGGER_ACTION_BEAT;
	dma_allocate(rx_resource, &rx_config);
}
static void setup_transfer_descriptor_tx(DmacDescriptor *tx_descriptor)
{
	struct dma_descriptor_config tx_descriptor_config;
	dma_descriptor_get_config_defaults(&tx_descriptor_config);
	tx_descriptor_config.beat_size = DMA_BEAT_SIZE_BYTE;
	tx_descriptor_config.dst_increment_enable = false;
	tx_descriptor_config.block_transfer_count = sizeof(buffer_tx)/sizeof(uint8_t);
	tx_descriptor_config.source_address = (uint32_t)buffer_tx + sizeof(buffer_tx);
	tx_descriptor_config.destination_address =
	(uint32_t)(&spi_master_instance.hw->SPI.DATA.reg);
	dma_descriptor_create(tx_descriptor, &tx_descriptor_config);
}
static void setup_transfer_descriptor_rx(DmacDescriptor *rx_descriptor)
{
	struct dma_descriptor_config rx_descriptor_config;
	dma_descriptor_get_config_defaults(&rx_descriptor_config);
	rx_descriptor_config.beat_size = DMA_BEAT_SIZE_BYTE;
	rx_descriptor_config.src_increment_enable = false;
	rx_descriptor_config.block_transfer_count = sizeof(buffer_rx)/sizeof(uint8_t);
	rx_descriptor_config.source_address =
	(uint32_t)(&spi_slave_instance.hw->SPI.DATA.reg);
	rx_descriptor_config.destination_address =
	(uint32_t)buffer_rx + sizeof(buffer_rx);
	dma_descriptor_create(rx_descriptor, &rx_descriptor_config);
}
static void configure_spi_master(void)
{
	struct spi_config config_spi_master;
	struct spi_slave_inst_config slave_dev_config;
	/* Configure and initialize software device instance of peripheral slave */
	spi_slave_inst_get_config_defaults(&slave_dev_config);
	slave_dev_config.ss_pin = SLAVE_SELECT_PIN;
	spi_attach_slave(&slave, &slave_dev_config);
	/* Configure, initialize and enable SERCOM SPI module */
	spi_get_config_defaults(&config_spi_master);
	config_spi_master.mode_specific.master.baudrate = TEST_SPI_BAUDRATE;
	config_spi_master.mux_setting = CONF_MASTER_MUX_SETTING;
	config_spi_master.pinmux_pad0 = CONF_MASTER_PINMUX_PAD0;
	config_spi_master.pinmux_pad1 = CONF_MASTER_PINMUX_PAD1;
	config_spi_master.pinmux_pad2 = CONF_MASTER_PINMUX_PAD2;
	config_spi_master.pinmux_pad3 = CONF_MASTER_PINMUX_PAD3;
	spi_init(&spi_master_instance, CONF_MASTER_SPI_MODULE, &config_spi_master);
	spi_enable(&spi_master_instance);
}
static void configure_spi_slave(void)
{
	struct spi_config config_spi_slave;
	/* Configure, initialize and enable SERCOM SPI module */
	spi_get_config_defaults(&config_spi_slave);
	config_spi_slave.mode = SPI_MODE_SLAVE;
	config_spi_slave.mode_specific.slave.preload_enable = true;
	config_spi_slave.mode_specific.slave.frame_format = SPI_FRAME_FORMAT_SPI_FRAME;
	config_spi_slave.mux_setting = CONF_SLAVE_MUX_SETTING;
	config_spi_slave.pinmux_pad0 = CONF_SLAVE_PINMUX_PAD0;
	config_spi_slave.pinmux_pad1 = CONF_SLAVE_PINMUX_PAD1;
	config_spi_slave.pinmux_pad2 = CONF_SLAVE_PINMUX_PAD2;
	config_spi_slave.pinmux_pad3 = CONF_SLAVE_PINMUX_PAD3;
	spi_init(&spi_slave_instance, CONF_SLAVE_SPI_MODULE, &config_spi_slave);
	spi_enable(&spi_slave_instance);
}


bool checkSleepElligible(void);
static void vTask_sleep_manager(void *params);

void gotoSleep(void);

bool initSleepSeqeunce=false;
uint32_t tempSleepWait=0;

int main (void)
{
	system_init();
	
	/* Disable digital interfaces to unused peripherals */
	system_apb_clock_clear_mask(SYSTEM_CLOCK_APB_APBA,
	PM_APBAMASK_PAC0 | PM_APBAMASK_WDT);
	system_apb_clock_clear_mask(SYSTEM_CLOCK_APB_APBB,
	PM_APBBMASK_PAC1 | PM_APBBMASK_DSU);
	system_apb_clock_clear_mask(SYSTEM_CLOCK_APB_APBC,
	PM_APBCMASK_PAC2 | PM_APBCMASK_AC | PM_APBCMASK_DAC);
	
	/* Disable NVM low power mode during sleep due to lockups (device errata) */
	NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;
	
	system_set_sleepmode(SYSTEM_SLEEPMODE_STANDBY);
	
	system_interrupt_enable_global();
	
	//irq_initialize_vectors();
	//cpu_irq_enable();
	delay_init();
	init_eeprom();
	
	
	
	start_lcd_service();
	start_gsm_service();
	start_motor_service();
	
	xTaskCreate(vTask_sleep_manager,NULL,(uint16_t)100,NULL,1,NULL);
	
	vTaskStartScheduler();
	
	for (;;)
	{
	}
}


bool checkSleepElligible(void)
{
	return (/*!turnOffTimerOn && */!getACPowerState() && motor_checkSleepElligible() && gsm_checkSleepElligible());
}

static void vTask_sleep_manager(void *params)
{
	UNUSED(params);
	for (;;)
	{
		if (checkSleepElligible())
		{
			if(!initSleepSeqeunce)
			{
				tempSleepWait=xTaskGetTickCount();
				initSleepSeqeunce=true;
			}
			else if(initSleepSeqeunce && xTaskGetTickCount()-tempSleepWait>sleepWaitTime)
			{
				uint8_t cnt=10;
				bool led=false;
				do
				{
					led=!led;
					if(led)
					THREEPHASE_OK_LED_ON;
					else
					THREEPHASE_OK_LED_OFF;

					tempSleepWait=xTaskGetTickCount();
					while(xTaskGetTickCount()-tempSleepWait<200)
					{}
				}while(--cnt);
				
				gotoSleep();
			}
		}
		else
		{
			initSleepSeqeunce=false;
		}
	}
}

void gotoSleep(void)
{
	LCD_PWR_DIS();
	
	lcd_in_sleep = true;
	setNetworkCharacter=true;
	system_sleep();
	mcuWakeUpFromSleep=true;
}