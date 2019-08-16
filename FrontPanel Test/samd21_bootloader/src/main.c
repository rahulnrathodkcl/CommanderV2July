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

struct bootloader_parameter
{
	uint32_t ulongintDiscard;
	uint8_t ucharFirst_Time_Write;
	uint8_t firmware_download_pending;
	uint8_t firmware_update_process_completed;
	uint8_t firmware_update_error_code;
	uint8_t retries;
	
	uint8_t dummy1;
	uint8_t dummy2;
	uint8_t dummy3;
	
	char firmware_updater_mobile_no[16];
	
}bootloader_parameter;

struct ftp_path_parameter
{
	uint32_t ulongintDiscard;
	uint8_t ucharFirst_Time_Write;
	char ftp_address[30];
	char ftp_path[20];
	
}ftp_path_parameter;

struct ftp_user_parameter
{
	uint32_t ulongintDiscard;
	uint8_t ucharFirst_Time_Write;
	char ftp_username[16];
	char ftp_password[16];
	char ftp_filename[16];
	
}ftp_user_parameter;

struct struct_audio_details_page
{
	uint32_t u32pageheader_ee;  //4
	uint8_t u8tfirst_time_write_ee;//1
	//////////////////////////////////////////////////////////////////////////

	char Audio_File_Alias;//1
	uint8_t dummy_byte1;//1
	uint8_t dummy_byte2;//1

	uint32_t File_start_Address; //4
	uint32_t File_Size;//4
	
}struct_audio_details_page;

void configure_eeprom(void);

void configure_eeprom(void)
{
	enum status_code error_code = eeprom_emulator_init();
	if (error_code == STATUS_ERR_NO_MEMORY)
	{
		#ifdef LCD_SUPPORT
		lcd_printf("SET EEPROM FUSE");
		while (true);
		#endif
	}
	else if (error_code != STATUS_OK)
	{
		eeprom_emulator_erase_memory();
		eeprom_emulator_init();
	}
}

uint8_t page_data[EEPROM_PAGE_SIZE];

void Read_EEprom(void);
void Read_EEprom(void)
{
	configure_eeprom();
	eeprom_emulator_read_page(BOOTLOADER_PARAMETER_PAGE, page_data);
	memcpy(&bootloader_parameter,page_data,sizeof(bootloader_parameter));
	if (bootloader_parameter.ucharFirst_Time_Write != 85)
	{
		bootloader_parameter.ulongintDiscard		   = 0;
		bootloader_parameter.ucharFirst_Time_Write	   = 85;
		bootloader_parameter.firmware_download_pending = false;
		bootloader_parameter.firmware_update_process_completed = false;
		bootloader_parameter.firmware_update_error_code = 0;
		bootloader_parameter.retries= 0;
		
		
		memset(bootloader_parameter.firmware_updater_mobile_no, '\0', sizeof(bootloader_parameter.firmware_updater_mobile_no));
		strcpy(bootloader_parameter.firmware_updater_mobile_no,"0000000000");
		
		memcpy(page_data,&bootloader_parameter,sizeof(bootloader_parameter));
		eeprom_emulator_write_page(BOOTLOADER_PARAMETER_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
	}
	
	eeprom_emulator_read_page(FTP_PATH_PARAMETER_PAGE, page_data);
	memcpy(&ftp_path_parameter,page_data,sizeof(ftp_path_parameter));
	if (ftp_path_parameter.ucharFirst_Time_Write !=85)
	{
		ftp_path_parameter.ulongintDiscard			   = 0;
		ftp_path_parameter.ucharFirst_Time_Write	   = 85;
		
		memset(ftp_path_parameter.ftp_address, '\0', sizeof(ftp_path_parameter.ftp_address));
		strcpy(ftp_path_parameter.ftp_address,"ftp.drivehq.com");
		
		memset(ftp_path_parameter.ftp_path, '\0', sizeof(ftp_path_parameter.ftp_path));
		strcpy(ftp_path_parameter.ftp_path,"/firmware/");
		
		memcpy(page_data,&ftp_path_parameter,sizeof(ftp_path_parameter));
		eeprom_emulator_write_page(FTP_PATH_PARAMETER_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
	}
	
	eeprom_emulator_read_page(FTP_USER_PARAMETER_PAGE, page_data);
	memcpy(&ftp_user_parameter,page_data,sizeof(ftp_user_parameter));
	if (ftp_user_parameter.ucharFirst_Time_Write !=85)
	{
		ftp_user_parameter.ulongintDiscard			   = 0;
		ftp_user_parameter.ucharFirst_Time_Write	   = 85;
		
		memset(ftp_user_parameter.ftp_username, '\0', sizeof(ftp_user_parameter.ftp_username));
		strcpy(ftp_user_parameter.ftp_username,"rahulnrathodkcl");
		
		memset(ftp_user_parameter.ftp_password, '\0', sizeof(ftp_user_parameter.ftp_password));
		strcpy(ftp_user_parameter.ftp_password,"123456789");
		
		memset(ftp_user_parameter.ftp_filename, '\0', sizeof(ftp_user_parameter.ftp_filename));
		strcpy(ftp_user_parameter.ftp_filename,"firmware.bin");
		
		memcpy(page_data,&ftp_user_parameter,sizeof(ftp_user_parameter));
		eeprom_emulator_write_page(FTP_USER_PARAMETER_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
	}
}

#define AUTO_LED PIN_PB08
#define PHASE_LED PIN_PB07
#define MOTOR_LED PIN_PA05

#define AUTO_BUTTON PIN_PA28
#define ON_BUTTON PIN_PB06
#define OFF_BUTTON PIN_PB05
#define LCD_BUTTON PIN_PB09



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
	
	Read_EEprom();
	
	struct nvm_config nvm_cfg;
	/* Initialize the NVM */
	nvm_get_config_defaults(&nvm_cfg);
	nvm_cfg.manual_page_write = false;
	nvm_set_config(&nvm_cfg);
	
	#ifdef FTP_MODE_BOOT_SUPPORT
	spiflash_init();
	#endif


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
bool LCDButtonPress=false;

autoLed=phaseLed=MotorLed=false;
port_pin_set_output_level(AUTO_LED, false);
port_pin_set_output_level(PHASE_LED, false);
port_pin_set_output_level(MOTOR_LED, false);

	while(1)
	{
		
		port_pin_set_output_level(AUTO_LED,port_pin_get_input_level(autoLed));
		
		if(port_pin_get_input_level(LCD_BUTTON))
		{
			autoLed=true;
			phaseLed= true;
			MotorLed=true;
			
		}
		else
		{
					if(port_pin_get_input_level(AUTO_BUTTON))
					autoLed=!autoLed;
					if(port_pin_get_input_level(ON_BUTTON))
					phaseLed=!phaseLed;
					if(port_pin_get_input_level(OFF_BUTTON))
					MotorLed=!MotorLed;
		}
			//port_pin_set_output_level(AUTO_LED, autoLed);
			port_pin_set_output_level(PHASE_LED, phaseLed);
			port_pin_set_output_level(MOTOR_LED, MotorLed);
	}
	
	#endif
	
	for (;;)
	{
		
	}
}


//------------GSM OPERATION----------------------$$$$$$$$$$$$$$
#define GSM_STATUS_POSITION		PIN_PA27
#define GSM_STATUS_OK			port_pin_get_input_level(GSM_STATUS_POSITION)
#define GSM_STATUS_ERROR		(!port_pin_get_input_level(GSM_STATUS_POSITION))

#define GSM_PWR_DDR		REG_PORT_DIR1
#define GSM_PWR_PORT	REG_PORT_OUT1
#define GSM_PWR_POS		PORT_PB16
#define GSM_PWR_AS_OP	GSM_PWR_DDR|=GSM_PWR_POS
#define GSM_PWR_ON		GSM_PWR_PORT|=GSM_PWR_POS
#define GSM_PWR_OFF		GSM_PWR_PORT&=~(GSM_PWR_POS)

static void vTask_FTP(void *params)
{
	bool boolGsm_config_flag			=false;
	bool boolconfig_gprs				=false;
	bool boolstart_gprs					=false;
	bool boolserver_connect				=false;
	
	uint32_t firmware_size = 0;
	
	GSM_PWR_AS_OP;
	
	gsm_init();
	
	#ifdef LCD_SUPPORT
	LCD_clear();
	lcd_printf("    STARTING    ");
	LCD_setCursor(0,1);
	lcd_printf("   GSM SERVICE  ");
	#endif
	
	struct port_config pin_conf_gsm_status;
	port_get_config_defaults(&pin_conf_gsm_status);
	pin_conf_gsm_status.direction  = PORT_PIN_DIR_INPUT;
	pin_conf_gsm_status.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(GSM_STATUS_POSITION, &pin_conf_gsm_status);
	
	
	if(GSM_STATUS_OK)
	{
		boolGsm_config_flag			=false;
		boolconfig_gprs				=false;
		boolstart_gprs				=false;
		boolserver_connect			=false;
		
		GSM_PWR_ON;
		vTaskDelay(3000);
		GSM_PWR_OFF;
		vTaskDelay(5000);
	}
	uint8_t ucharCounter=0;
	for (;;)
	{
		if (GSM_STATUS_OK)
		{
			if (boolGsm_config_flag == false)
			{
				if (gsm_is_network_registered() == GSM_NETWORK_REGISTERED)
				{
					if(gsm_config_module()==GSM_ERROR_NONE)
					{
						#ifdef LCD_SUPPORT
						LCD_clear();
						lcd_printf("    DISABLE     ");
						LCD_setCursor(0,1);
						lcd_printf(" CALL,SMS,TIME  ");
						vTaskDelay(2000);
						#endif
						gsm_reject_all_incomming_calls();
						gsm_disable_new_sms_message_indications();
						gsm_disable_network_time_update();
						
						#ifdef LCD_SUPPORT
						LCD_clear();
						lcd_printf("   CONNECTING  ");
						LCD_setCursor(0,1);
						lcd_printf("     SERVER    ");
						#endif
						boolGsm_config_flag = true;
					}
					else
					{
						boolGsm_config_flag = false;
					}
				}
				else
				{
					vTaskDelay(2000/portTICK_PERIOD_MS);
				}
			}
			else
			{
				if (boolconfig_gprs == false)
				{
					if (gsm_config_gprs() == GSM_ERROR_NONE)
					{
						vTaskDelay(1000/portTICK_PERIOD_MS);
						boolconfig_gprs  = true;
					}
					else
					{
						boolconfig_gprs  = false;
					}
				}
				else
				{
					if (boolstart_gprs == false)
					{
						if (gsm_start_gprs() == GSM_ERROR_NONE)
						{
							vTaskDelay(1000/portTICK_PERIOD_MS);
							boolstart_gprs  = true;
						}
						else
						{
							boolstart_gprs  = false;
						}
					}
					else
					{
						if (boolserver_connect == false)
						{
							if (config_ftp(ftp_path_parameter.ftp_address,ftp_user_parameter.ftp_username,
							ftp_user_parameter.ftp_password,ftp_user_parameter.ftp_filename,ftp_path_parameter.ftp_path) == GSM_ERROR_NONE)
							{
								boolserver_connect  = true;
							}
							else
							{
								boolserver_connect  = false;
								if (++ucharCounter>=100)
								{
									bootloader_parameter.firmware_download_pending = false;
									bootloader_parameter.firmware_update_process_completed = true;
									bootloader_parameter.firmware_update_error_code = 0xFF;
									
									memcpy(page_data,&bootloader_parameter,sizeof(bootloader_parameter));
									eeprom_emulator_write_page(BOOTLOADER_PARAMETER_PAGE, page_data);
									eeprom_emulator_commit_page_buffer();
									vTaskDelay(2000/portTICK_PERIOD_MS);
									gsm_stop_gprs();
									
									#ifdef LCD_SUPPORT
									LCD_clear();
									lcd_printf("    ENABLE     ");
									LCD_setCursor(0,1);
									lcd_printf(" CALL,SMS,TIME  ");
									vTaskDelay(2000);
									#endif
									gsm_enable_all_incomming_calls();
									gsm_enable_new_sms_message_indications();
									gsm_enable_network_time_update();
									
									cpu_irq_disable();
									system_reset();
								}
							}
						}
						else
						{
							#ifdef LCD_SUPPORT
							LCD_clear();
							lcd_printf(" GET FILE SIZE:");
							LCD_setCursor(0,1);
							#endif
							
							enum gsm_error ftp_size_error;
							
							ftp_size_error = gsm_get_the_size_of_specified_file_in_ftp_server(&firmware_size);
							
							if (ftp_size_error == GSM_ERROR_NONE)
							{
								if (firmware_size != 0)
								{
									#ifdef LCD_SUPPORT
									lcd_printf(" %lubyte",firmware_size);
									vTaskDelay(2000/portTICK_PERIOD_MS);
									LCD_clear();
									lcd_printf("  DOWNLOADING   ");
									LCD_setCursor(0,1);
									lcd_printf(" FIRMWARE FILE  ");
									#endif
									
									char ucharTemp_Var = download_firmware_into_flash(firmware_size);
									
									if (ucharTemp_Var != 'O')
									{
										bootloader_parameter.firmware_download_pending = false;
										bootloader_parameter.firmware_update_process_completed = true;
										bootloader_parameter.firmware_update_error_code = 0xFF;
										
										memcpy(page_data,&bootloader_parameter,sizeof(bootloader_parameter));
										eeprom_emulator_write_page(BOOTLOADER_PARAMETER_PAGE, page_data);
										eeprom_emulator_commit_page_buffer();
										vTaskDelay(2000/portTICK_PERIOD_MS);
										gsm_stop_gprs();
										
										#ifdef LCD_SUPPORT
										LCD_clear();
										lcd_printf("    ENABLE     ");
										LCD_setCursor(0,1);
										lcd_printf(" CALL,SMS,TIME  ");
										vTaskDelay(2000);
										#endif
										gsm_enable_all_incomming_calls();
										gsm_enable_new_sms_message_indications();
										gsm_enable_network_time_update();
										
										cpu_irq_disable();
										system_reset();
									}
									
									#ifdef LCD_SUPPORT
									LCD_clear();
									lcd_printf("  PROGRAMMING   ");
									#endif
									
									if (!program_memory(firmware_size))
									{
										
									}
									
									#ifdef LCD_SUPPORT
									LCD_setCursor(0,1);
									for (uint8_t i=0;i<16;i++)
									{
										LCD_write(0);
										delay_ms(100);
									}
									
									#ifdef LCD_SUPPORT
									LCD_clear();
									lcd_printf("    ENABLE     ");
									LCD_setCursor(0,1);
									lcd_printf(" CALL,SMS,TIME  ");
									vTaskDelay(2000);
									#endif
									gsm_enable_all_incomming_calls();
									gsm_enable_new_sms_message_indications();
									gsm_enable_network_time_update();
									
									LCD_clear();
									lcd_printf("    STARTING    ");
									LCD_setCursor(0,1);
									lcd_printf("  APPLICATION   ");
									#endif
									
									bootloader_parameter.firmware_download_pending = false;
									bootloader_parameter.firmware_update_process_completed = true;
									bootloader_parameter.firmware_update_error_code = 0;


									memcpy(page_data,&bootloader_parameter,sizeof(bootloader_parameter));
									eeprom_emulator_write_page(BOOTLOADER_PARAMETER_PAGE, page_data);
									eeprom_emulator_commit_page_buffer();
									
									vTaskDelay(2000/portTICK_PERIOD_MS);
									
									gsm_stop_gprs();
									
									cpu_irq_disable();
									
									system_reset();
								}
								else
								{
									#ifdef LCD_SUPPORT
									LCD_clear();
									lcd_printf("     ERROR:     ");
									LCD_setCursor(0,1);
									lcd_printf("       %d       ",ftp_size_error);
									vTaskDelay(5000);
									#endif
									bootloader_parameter.firmware_download_pending = false;
									bootloader_parameter.firmware_update_process_completed = true;
									bootloader_parameter.firmware_update_error_code = ftp_size_error;
									
									memcpy(page_data,&bootloader_parameter,sizeof(bootloader_parameter));
									eeprom_emulator_write_page(BOOTLOADER_PARAMETER_PAGE, page_data);
									eeprom_emulator_commit_page_buffer();
									vTaskDelay(2000/portTICK_PERIOD_MS);
									gsm_stop_gprs();
									cpu_irq_disable();
									system_reset();
								}
							}
							else
							{
								#ifdef LCD_SUPPORT
								LCD_clear();
								lcd_printf("     ERROR:     ");
								LCD_setCursor(0,1);
								lcd_printf("       %d       ",ftp_size_error);
								vTaskDelay(5000);
								#endif
								bootloader_parameter.firmware_download_pending = false;
								bootloader_parameter.firmware_update_process_completed = true;
								bootloader_parameter.firmware_update_error_code = ftp_size_error;
								memcpy(page_data,&bootloader_parameter,sizeof(bootloader_parameter));
								eeprom_emulator_write_page(BOOTLOADER_PARAMETER_PAGE, page_data);
								eeprom_emulator_commit_page_buffer();
								vTaskDelay(2000/portTICK_PERIOD_MS);
								gsm_stop_gprs();
								cpu_irq_disable();
								system_reset();
							}
						}
					}
				}
			}
		}
		else
		{
			boolGsm_config_flag			=false;
			boolconfig_gprs				=false;
			boolstart_gprs				=false;
			boolserver_connect			=false;
			
			GSM_PWR_ON;
			vTaskDelay(3000);
			GSM_PWR_OFF;
			vTaskDelay(5000);
		}
	}
}

void spiflash_init(void)
{
	struct at25dfx_chip_config at25dfx_chip_config;
	struct spi_config at25dfx_spi_config;
	
	at25dfx_spi_get_config_defaults(&at25dfx_spi_config);
	at25dfx_spi_config.mode_specific.master.baudrate = AT25DFX_CLOCK_SPEED;
	at25dfx_spi_config.mux_setting = AT25DFX_SPI_PINMUX_SETTING;
	at25dfx_spi_config.pinmux_pad0 = AT25DFX_SPI_PINMUX_PAD0;
	at25dfx_spi_config.pinmux_pad1 = AT25DFX_SPI_PINMUX_PAD1;
	at25dfx_spi_config.pinmux_pad2 = AT25DFX_SPI_PINMUX_PAD2;
	at25dfx_spi_config.pinmux_pad3 = AT25DFX_SPI_PINMUX_PAD3;

	spi_init(&at25dfx_spi, AT25DFX_SPI, &at25dfx_spi_config);
	spi_enable(&at25dfx_spi);

	at25dfx_chip_config.type = AT25DFX_MEM_TYPE;
	at25dfx_chip_config.cs_pin = AT25DFX_CS;

	at25dfx_chip_init(&at25dfx_chip, &at25dfx_spi, &at25dfx_chip_config);
}

void start_application(void)
{
	void (*application_code_entry)(void);
	
	#ifdef LCD_SUPPORT
	LCD_setCursor(0,1);
	lcd_printf("APPLICATION MODE");
	delay_ms(2000);
	#endif
	
	/* Re base the Stack Pointer */
	__set_MSP(*(uint32_t *)APP_START_ADDRESS);

	/* Re base the vector table base address */
	SCB->VTOR = ((uint32_t)APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

	/* Load the Reset Handler address of the application */
	application_code_entry = (void (*)(void))(unsigned *)(*(unsigned *)
	(APP_START_ADDRESS + 4));

	/* Jump to user Reset Handler in the application */
	application_code_entry();
}

void check_boot_mode(void)
{
	/* Check if WDT is locked */
	if (!(WDT->CTRL.reg & WDT_CTRL_ALWAYSON))
	{
		/* Disable the Watchdog module */
		WDT->CTRL.reg &= ~WDT_CTRL_ENABLE;
	}
	
	#ifdef USB_DEVIDE_MODE_BOOT_SUPPORT
	udc_start();
	delay_ms(2000);
	if (main_b_cdc_enable)
	{
		#ifdef LCD_SUPPORT
		LCD_setCursor(0,1);
		lcd_printf("USB DEVICE MODE");
		#endif
		
		sam_ba_monitor_init(SAM_BA_INTERFACE_USBCDC);
		while(1)
		{
			sam_ba_monitor_run();
		}
	}
	else
	{
		udc_stop();
	}
	#endif

	#ifdef FTP_MODE_BOOT_SUPPORT
	if (bootloader_parameter.firmware_download_pending == true)
	{
		#ifdef LCD_SUPPORT
		LCD_setCursor(0,1);
		lcd_printf(" FTP BOOT MODE  ");
		
		#endif
		if(bootloader_parameter.retries<2)
		{
			bootloader_parameter.retries++;
			memcpy(page_data,&bootloader_parameter,sizeof(bootloader_parameter));
			eeprom_emulator_write_page(BOOTLOADER_PARAMETER_PAGE, page_data);
			eeprom_emulator_commit_page_buffer();
			return;
		}
		else
		{
			bootloader_parameter.firmware_download_pending = false;
			bootloader_parameter.firmware_update_error_code = 0xFE;
			bootloader_parameter.firmware_update_process_completed = true;
			memcpy(page_data,&bootloader_parameter,sizeof(bootloader_parameter));
			eeprom_emulator_write_page(BOOTLOADER_PARAMETER_PAGE, page_data);
			eeprom_emulator_commit_page_buffer();
		}
	}
	#endif
	
	
	spi_disable(&at25dfx_spi);
	
	/* Enters application mode*/
	start_application();
	
}


char download_firmware_into_flash(uint32_t firmware_size)
{
	uint32_t download_Size=0;
	
	uint16_t request_bytes = 128;
	uint16_t received_bytes = 0;
	
	uint32_t current_page=0;
	
	uint32_t current_address = FIRMWARE_START_ADDRESS_IN_FLASH;
	
	#ifdef LCD_SUPPORT
	LCD_clear();
	lcd_printf("f_Size:%lu",firmware_size);
	LCD_setCursor(0,1);
	lcd_printf("d_Size:%lu",download_Size);
	#endif
	
	char buff[240]={0};
	
	enum gsm_error err;
	
	err = gsm_open_ftp_get_session();
	
	if (err == GSM_ERROR_NONE)
	{
		while((download_Size < firmware_size))
		{
			if((download_Size+128) < firmware_size)
			{
				request_bytes=128;
			}
			else
			{
				request_bytes=firmware_size-download_Size;
			}
			
			err = gsm_read_ftp_download_data(request_bytes,buff,&received_bytes);
			if (err == GSM_FTP_DOWNLOAD_NO_DATA_AVAILABLE_IN_BUFFER)
			{
				vTaskDelay(1000/portTICK_PERIOD_MS);
			}
			else if (err == GSM_FTP_DOWNLOAD_SUCCESS)
			{
				at25dfx_chip_write_buffer(&at25dfx_chip,current_address,&buff[0],received_bytes);
				download_Size +=received_bytes;
				current_address+=received_bytes;
				
				#ifdef LCD_SUPPORT
				LCD_setCursor(0,1);
				lcd_printf("d_Size:%lu",download_Size);
				#endif
				vTaskDelay(50/portTICK_PERIOD_MS);
			}
			else
			{
				#ifdef LCD_SUPPORT
				LCD_clear();
				lcd_printf("   FTP ERROR    ");
				vTaskDelay(2000);
				#endif
				return ('E');
			}
		}
		if (err == GSM_FTP_DOWNLOAD_SUCCESS)
		{
			return ('O');
		}
		else
		{
			#ifdef LCD_SUPPORT
			LCD_clear();
			lcd_printf("   FTP ERROR    ");
			vTaskDelay(2000);
			#endif
			return ('E');
		}
	}
	else
	{
		#ifdef LCD_SUPPORT
		LCD_clear();
		lcd_printf("   FTP ERROR    ");
		vTaskDelay(2000);
		#endif
		return ('E');
	}
}


bool program_memory(uint32_t firmware_size)
{
	
	uint32_t curr_address = 0;
	uint32_t current_page = 0;
	uint16_t rows_clear, i;
	uint16_t j=0;
	enum status_code error_code;
	
	uint8_t page_buffer[NVMCTRL_PAGE_SIZE];
	current_page = APP_START_ADDRESS / NVMCTRL_PAGE_SIZE;
	curr_address = 0;
	
	/* Erase flash rows to fit new firmware */
	rows_clear = ((firmware_size / NVMCTRL_ROW_SIZE) + 1);
	for (i = 0; i < rows_clear; i++)
	{
		do{
			error_code = nvm_erase_row((APP_START_ADDRESS) + (NVMCTRL_ROW_SIZE * i));
		} while (error_code == STATUS_BUSY);
	}
	
	/* Program the flash memory page by page */
	do
	{
		/* Open the input file */
		if (j<(firmware_size/64))
		{
			uint32_t address = j * 64;
			at25dfx_chip_read_buffer(&at25dfx_chip,address,page_buffer,64);
			
			curr_address +=64;
			j++;
			
			/* Disable the global interrupts */
			cpu_irq_disable();
			
			/* Program the Flash Memory. */
			/* Write page buffer to flash */
			do {
				error_code = nvm_write_buffer(current_page * NVMCTRL_PAGE_SIZE,page_buffer, 64);
			} while (error_code == STATUS_BUSY);

			/* Enable the global interrupts */
			cpu_irq_enable();
			
			current_page++;
		}
		else
		{
			uint32_t address = j * 64;
			at25dfx_chip_read_buffer(&at25dfx_chip,address,page_buffer,(firmware_size % 64));
			
			curr_address += (firmware_size % 64);
			j++;
			
			/* Disable the global interrupts */
			cpu_irq_disable();
			
			/* Program the Flash Memory. */
			/* Write page buffer to flash */
			do {
				error_code = nvm_write_buffer(current_page * NVMCTRL_PAGE_SIZE,page_buffer, (firmware_size % 64));
			} while (error_code == STATUS_BUSY);

			/* Enable the global interrupts */
			cpu_irq_enable();
			
			current_page++;
		}
		
	} while (curr_address < firmware_size);
	
	return true;
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