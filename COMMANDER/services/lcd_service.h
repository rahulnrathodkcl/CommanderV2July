
#ifndef LCD_SERVICE_H_
#define LCD_SERVICE_H_

#define VERSION_NO "  B17-30-07-19  "



#include "lcd_driver.h"
#include "motor_service.h"
#include "gsm_service.h"

volatile bool lcd_in_sleep;
volatile bool initDisplay;
volatile bool setNetworkCharacter;

void start_lcd_service(void);


#endif /* LCD_SERVICE_H_ */