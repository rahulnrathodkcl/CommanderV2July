#include <asf.h>
#include "main.h"
#include "lcd_driver.h"
#include "conf_bootloader.h"
#include "conf_at25dfx.h"
#include "gsm_driver.h"
#include "sam_ba_monitor.h"

//////////////////////////////////////////////////////////////////////////
#define MSC_DELAY_SOF_COUNT     1000  // 1000ms
static volatile uint32_t sof_count = 0;
static volatile bool lun_connected = false;

static volatile bool main_b_cdc_enable = false;

COMPILER_WORD_ALIGNED
volatile uint8_t buffer[FLASH_BUFFER_SIZE];

//////////////////////////////////////////////////////////////////////////


static void vTask_FTP(void *params);


#define AUTO_LED PIN_PB08
#define PHASE_LED PIN_PB07
#define MOTOR_LED PIN_PA05

#define AUTO_BUTTON PIN_PA28
#define ON_BUTTON PIN_PB06
#define OFF_BUTTON PIN_PB05
#define LCD_BUTTON PIN_PB09

#define GSM_PWR_DDR		REG_PORT_DIR1
#define GSM_PWR_PORT	REG_PORT_OUT1
#define GSM_PWR_POS		PORT_PB16
#define GSM_PWR_AS_OP	GSM_PWR_DDR|=GSM_PWR_POS
#define GSM_PWR_ON		GSM_PWR_PORT|=GSM_PWR_POS
#define GSM_PWR_OFF		GSM_PWR_PORT&=~(GSM_PWR_POS)

int main (void)
{
	system_init();
	
	sleepmgr_init();
	
	irq_initialize_vectors();
	cpu_irq_enable();
	
	delay_init();
	
	#ifdef LCD_SUPPORT
	LCD_PWR_CONFIG();
	LCD_PWR_EN();
	delay_ms(200);
	LCD_init();
	#endif
	
	struct nvm_config nvm_cfg;
	/* Initialize the NVM */
	nvm_get_config_defaults(&nvm_cfg);
	nvm_cfg.manual_page_write = false;
	nvm_set_config(&nvm_cfg);
	

	#ifdef LCD_SUPPORT
	byte progressbar[8] = {
		B01111,
		B01111,
		B01111,
		B01111,
		B01111,
		B01111,
		B01111,
	};
	LCD_Create_Custom_createChar(0,progressbar);
	LCD_clear();
	lcd_printf("   BOOTING UP   ");
	LCD_setCursor(0,1);
	for (uint8_t i=0;i<16;i++)
	{
		LCD_write(0);
		delay_ms(50);
	}
	LCD_clear();

struct port_config pin_conf_LED;
port_get_config_defaults(&pin_conf_LED);
pin_conf_LED.direction  = PORT_PIN_DIR_OUTPUT;
port_pin_set_config(AUTO_LED, &pin_conf_LED);
port_pin_set_config(PHASE_LED, &pin_conf_LED);
port_pin_set_config(MOTOR_LED, &pin_conf_LED);


port_get_config_defaults(&pin_conf_LED);
pin_conf_LED.direction = PORT_PIN_DIR_INPUT;
pin_conf_LED.input_pull = PORT_PIN_PULL_UP;
//
port_pin_set_config(AUTO_BUTTON, &pin_conf_LED);
port_pin_set_config(ON_BUTTON, &pin_conf_LED);
port_pin_set_config(OFF_BUTTON, &pin_conf_LED);
port_pin_set_config(LCD_BUTTON, &pin_conf_LED);


bool autoLed,phaseLed,MotorLed;
bool autoButtonLevel,onButtonLevel,offButtonLevel,LCDButtonLevel;
autoButtonLevel=onButtonLevel=offButtonLevel=true;
LCDButtonLevel=false;
autoLed=phaseLed=MotorLed=false;
port_pin_set_output_level(AUTO_LED, false);
port_pin_set_output_level(PHASE_LED, false);
port_pin_set_output_level(MOTOR_LED, false);
GSM_PWR_AS_OP;
GSM_PWR_ON;
LCD_setCursor(0,0);
lcd_printf("PRESS BUTTON TO");
LCD_setCursor(0,1);
lcd_printf("GLOW LED");
	while(1)
	{

		
		if(autoButtonLevel!=(!port_pin_get_input_level(AUTO_BUTTON)))
		{
			autoLed=!autoLed;	
			autoButtonLevel=!port_pin_get_input_level(AUTO_BUTTON);
		}
			port_pin_set_output_level(AUTO_LED, autoLed);
		
		if(onButtonLevel!=(!port_pin_get_input_level(ON_BUTTON)))
		{
			phaseLed=!phaseLed;	
			onButtonLevel=!port_pin_get_input_level(ON_BUTTON);
		}
			port_pin_set_output_level(PHASE_LED, phaseLed);
		
		if(offButtonLevel!=(!port_pin_get_input_level(OFF_BUTTON)))
		{
			MotorLed=!MotorLed;	
			offButtonLevel=!port_pin_get_input_level(OFF_BUTTON);
		}
			port_pin_set_output_level(MOTOR_LED, MotorLed);
		
		if(LCDButtonLevel!=(!port_pin_get_input_level(LCD_BUTTON)))
		{
			LCD_clear();
			LCD_setCursor(0,1);
			lcd_printf("LCD BUTTON PRESS");
			delay_ms(1000);
			LCDButtonLevel=!port_pin_get_input_level(LCD_BUTTON);
			LCD_clear();
		}
		continue;
		

	}
	
	#endif
	
	for (;;)
	{
		
	}
}



static void vTask_FTP(void *params)
{

}

void spiflash_init(void)
{
}

void start_application(void)
{

}

void check_boot_mode(void)
{

}


char download_firmware_into_flash(uint32_t firmware_size)
{

}


bool program_memory(uint32_t firmware_size)
{
	

}





void main_suspend_lpm_action(void)
{
}

void main_remotewakeup_lpm_disable(void)
{
}

void main_remotewakeup_lpm_enable(void)
{
}


bool main_cdc_enable(uint8_t port)
{
	main_b_cdc_enable = true;
	return true;
}

void main_cdc_disable(uint8_t port)
{
	main_b_cdc_enable = false;
}

void main_cdc_set_dtr(uint8_t port, bool b_enable)
{
}

void main_cdc_rx_notify(uint8_t port)
{
}

void main_cdc_set_coding(uint8_t port, usb_cdc_line_coding_t * cfg)
{
}