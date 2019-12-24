#include "lcd_service.h"
#include <sys\_stdint.h>

static void lcd_displaying_task(void *params);
static QueueHandle_t xfour_Second_Queue;
static TimerHandle_t four_Second_timeout_timer=NULL;
static void four_second_timer_callback(TimerHandle_t timer);

static void lcd_displaying_task(void *params)
{
	UNUSED(params);
	
	gsmStatusChange=true;
	lcd_in_sleep =false;
	setNetworkCharacter=true;
	LCD_PWR_CONFIG();
	LCD_PWR_EN();
	vTaskDelay(500);
	
	LCD_init();
	
	uint8_t screen=1;
	//bool four_sec_timer_is_active = true;
	//bool two_sec_timer_is_active = false;
	
	uint8_t time=0;
	
	
	
	
	LCD_clear();
	LCD_setCursor(0,0);
	lcd_printf(" KRISHNA  SMART ");
	LCD_setCursor(0,1);
	lcd_printf("   TECHNOLOGY   ");
	vTaskDelay(3000);
	
	LCD_clear();
	LCD_setCursor(0,0);
	lcd_printf("  SOFTWARE VER  ");
	LCD_setCursor(0,1);
	lcd_printf("%s",VERSION_NO);
	vTaskDelay(2000);
	
	four_Second_timeout_timer = xTimerCreate(NULL,(1 * 4000 / portTICK_PERIOD_MS), pdTRUE, NULL, four_second_timer_callback);
	xTimerStart( four_Second_timeout_timer, 0 );
	
	
	for (;;)
	{
		if (lcd_in_sleep)
		{
			lcd_in_sleep = false;
			LCD_PWR_EN();
			vTaskDelay(100);
			LCD_init();
		}
		
		if(setNetworkCharacter)
		{
			setNetworkCharacter=false;
			byte Network_0[8]={
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B00000
			};
			
			byte Network_1[8]={
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B11111,
				0B11111
			};
			byte Network_2[8]={
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B11111,
				0B11111,
				0B11111
			};
			byte Network_3[8]={
				0B00000,
				0B00000,
				0B00000,
				0B00000,
				0B11111,
				0B11111,
				0B11111,
				0B11111
			};
			byte Network_4[8]={
				0B00000,
				0B00000,
				0B11111,
				0B11111,
				0B11111,
				0B11111,
				0B11111,
				0B11111
			};

			byte Network_5[8]={
				0B11111,
				0B11111,
				0B11111,
				0B11111,
				0B11111,
				0B11111,
				0B11111,
				0B11111
			};
			
			LCD_Create_Custom_createChar(0,Network_0);
			LCD_Create_Custom_createChar(1,Network_1);
			LCD_Create_Custom_createChar(2,Network_2);
			LCD_Create_Custom_createChar(3,Network_3);
			LCD_Create_Custom_createChar(4,Network_4);
			LCD_Create_Custom_createChar(5,Network_5);
		}
		
		
		if(firstEvent)
		{
			LCD_setCursor(0,0);
			lcd_printf("Initializing... ");
			LCD_setCursor(0,1);
			lcd_printf("COMMANDER v2    ");
			screen=1;
		}
		else
		{
			if(gsmStatusChange)
			{
				LCD_setCursor(0,0);
				lcd_printf("PHONE STATUS    ");
				if(boolGsm_config_flag)
				{
					LCD_setCursor(0,1);
					lcd_printf("ON              ");
				}
				else
				{
					LCD_setCursor(0,1);
					lcd_printf("OFF             ");
				}
				gsmStatusChange = false;
				vTaskDelay(2000/portTICK_PERIOD_MS);
			}
			
			switch(screen)
			{
				case  1:
				{
					LCD_setCursor(0,0);
					lcd_printf("VRY   VYB   VBR ");
					LCD_setCursor(0,1);
					lcd_printf("%03lu   ",(Analog_Parameter_Struct.PhaseRY_Voltage));
					lcd_printf("%03lu   ",(Analog_Parameter_Struct.PhaseYB_Voltage));
					lcd_printf("%03lu ",(Analog_Parameter_Struct.PhaseBR_Voltage));
					break;
				}
				case 2:
				{
					LCD_setCursor(0,0);
					lcd_printf("3 PHASE SEQ:");
					if (structThreePhase_state.u8t_phase_sequence_flag == THREEPHASE_OK)
					{
						lcd_printf(" OK ");
					}
					else
					{
						lcd_printf(" ERR");
					}
					LCD_setCursor(0,1);
					lcd_printf("PHASE STATE:");
					if (structThreePhase_state.u8t_phase_ac_state == AC_3PH)
					{
						lcd_printf(" 3PH ");
					}
					else if(structThreePhase_state.u8t_phase_ac_state == AC_2PH)
					{
						lcd_printf(" 2PH ");
					}
					else
					{
						lcd_printf(" OFF");
					}
					break;
				}
				case  3:
				{
					LCD_setCursor(0,0);
					if(!getMotorState())
					{
						lcd_printf("MOTOR :  OFF    ");
					}
					else
					{
						lcd_printf("MOTOR :  ON     ");
					}
					
					LCD_setCursor(0,1);
					lcd_printf("                ");
					if(user_settings_parameter_struct.autoStartAddress)
					{
						lcd_printf("AUTO  :  ON     ");
					}
					else
					{
						lcd_printf("AUTO  :  OFF    ");
					}
					//else
					//{
					//LCD_setCursor(0,0);
					//lcd_printf("MOTOR WATTAGE:  ");
					//LCD_setCursor(0,1);
					//lcd_printf("%03lu.%02lu kW         ",(Analog_Parameter_Struct.Motor_Power_IntPart),(Analog_Parameter_Struct.Motor_Power_DecPart));
					//}
					break;
				}
				case  4:
				{
					LCD_setCursor(0,0);
					lcd_printf("MOTOR CURRENT:  ");
					LCD_setCursor(0,1);
					lcd_printf("%03lu.%02lu            ",(Analog_Parameter_Struct.Motor_Current_IntPart),(Analog_Parameter_Struct.Motor_Current_DecPart));
					break;
				}
				case 5:
				{
					LCD_setCursor(0,0);
					lcd_printf("CURRENT SETTING ");
					LCD_setCursor(0,1);
					if(user_settings_parameter_struct.currentDetectionAddress)
					{
						lcd_printf("ON              ");
					}
					else
					{
						lcd_printf("OFF             ");
					}
					break;
				}
				case 6:
				{
					if(user_settings_parameter_struct.currentDetectionAddress)
					{
						LCD_setCursor(0,0);
						//lcd_printf("                ");
						uint32_t temp_Int = user_settings_parameter_struct.overloadAddress / 100;
						uint32_t temp_Dec = user_settings_parameter_struct.overloadAddress % 100;
						lcd_printf("OVER  : %03lu.%02lu  ",temp_Int,temp_Dec);
						LCD_setCursor(0,1);
						//lcd_printf("                ");
						temp_Int = user_settings_parameter_struct.underloadAddress / 100;
						temp_Dec = user_settings_parameter_struct.underloadAddress % 100;
						lcd_printf("UNDER : %03lu.%02lu  ",temp_Int,temp_Dec);
					}
					break;
				}
				case 7:
				{
					LCD_setCursor(0,0);
					lcd_printf("O-LEVEL : ");
					if (overheadLevel == OVERHEADHIGHLEVEL)
					{
						lcd_printf("HIGH  ");
					}
					else if (overheadLevel == OVERHEADMIDLEVEL)
					{
						lcd_printf("MID   ");
					}
					else if (overheadLevel == OVERHEADCRITICALLEVEL)
					{
						lcd_printf("LOW   ");
					}
					LCD_setCursor(0,1);
					lcd_printf("U-LEVEL : ");
					if (undergroundLevel == CRITICALLEVEL)
					{
						lcd_printf("CRTCL ");
					}
					else if (undergroundLevel == LOWLEVEL)
					{
						lcd_printf("LOW   ");
					}
					else if (undergroundLevel == MIDLEVEL)
					{
						lcd_printf("MID   ");
					}
					else if (undergroundLevel == HIGHLEVEL)
					{
						lcd_printf("HIGH  ");
					}
					break;
				}
				case 8:
				{
					LCD_setCursor(0,0);
					lcd_printf("BatteryPer: %u%% ",Analog_Parameter_Struct.Battery_percentage);
					LCD_setCursor(0,1);
					lcd_printf("SIGNAL : ");
					LCD_setCursor(9,1);
					lcd_printf("       ");
					LCD_setCursor(9,1);
					
					uint8_t tempSignal = Signal_Strength;
					for (uint8_t i=0;i<=tempSignal;i++)
					{
						LCD_write(i);
					}

					break;
				}
				case 9:
				{
					LCD_setCursor(0,0);
					switch(callStateOnLCD)
					{
						case LCDCALLSTATE_RINGING:
						{
							lcd_printf("INCOMING CALL.. ");
							LCD_setCursor(0,1);
							lcd_printf("%-16s",numberOnLCD);
							break;
						}
						case LCDCALLSTATE_INCALL:
						{
							lcd_printf("IN CALL...      ");
							LCD_setCursor(0,1);
							lcd_printf("%-16s",numberOnLCD);
							break;
						}
						case LCDCALLSTATE_OUTGOING:
						{
							lcd_printf("DIALING...      ");
							LCD_setCursor(0,1);
							lcd_printf("%-16s",numberOnLCD);
							break;
						}
						case LCDCALLSTATE_INCOMINGSMS:
						{
							lcd_printf("INCOMING SMS... ");
							LCD_setCursor(0,1);
							lcd_printf("%-16s",numberOnLCD);
							setCallStateOnLCD(LCDCALLSTATE_IDLE,NULL,false);
							break;
						}
						case LCDCALLSTATE_OUTGOINGSMS:
						{
							lcd_printf("OUTGOING SMS... ");
							LCD_setCursor(0,1);
							lcd_printf("%-16s",numberOnLCD);
							setCallStateOnLCD(LCDCALLSTATE_IDLE,NULL,false);
							break;
						}
					}
					break;
				}
				case 10:
				{
					if(Analog_Parameter_Struct.Battery_percentage<35)
					{
						LCD_setCursor(0,0);
						lcd_printf("LOW BATTERY     ");
						LCD_setCursor(0,1);
						lcd_printf("                ");
						break;
					}
					else
					{
						screen++;
					}
					break;
				}
				
				//case 8:
				//{
				//LCD_clear();
				//LCD_setCursor(0,0);
				//lcd_printf("%d",xTaskGetTickCount());
				//LCD_setCursor(0,1);
				//lcd_printf("%d;%d",lastGSMCommunicationTime,lastToLastGSMCommunicationTime);
				//break;
				//}
			}
		}
		
		if (xQueueReceive(xfour_Second_Queue,&time,0))
		{
			xTimerChangePeriod( four_Second_timeout_timer, 4000/portTICK_PERIOD_MS, portMAX_DELAY);
			if(varPauseDisplay==false)
			{
				screen++;
				if(!getMotorState() && screen==4)
				{
					screen++;
				}
				
				if(screen==6 && (!user_settings_parameter_struct.currentDetectionAddress || !getMotorState()))
				{
					screen++;
				}

				if(!factory_settings_parameter_struct.ENABLE_WATER && screen==7)
				{
					screen++;
				}
				
				if(callStateOnLCD==LCDCALLSTATE_IDLE && screen==9)
				{
					screen++;
				}
				else if(callStateOnLCD!=LCDCALLSTATE_IDLE)
				{
					screen=9;
				}
			}
		}
		
		//screen = 8;
		vTaskDelay(500);

		if (screen>10)
		{
			screen=1;
			
			LCD_PWR_DIS();
			lcd_in_sleep = true;
			setNetworkCharacter=true;
			vTaskDelay(100/portTICK_PERIOD_MS);
		}
	}
}


void start_lcd_service(void)
{
	xfour_Second_Queue=xQueueCreate(1,sizeof(uint8_t));
	xTaskCreate(lcd_displaying_task,NULL,(uint16_t)400,NULL,1,NULL);
	
}

static void four_second_timer_callback(TimerHandle_t timer)
{
	uint8_t ucharfour_Second=1;
	xQueueSendFromISR(xfour_Second_Queue,&ucharfour_Second,0);
}

