#ifndef CONF_AT25DFX_H_
#define CONF_AT25DFX_H_

#define AT25DFX_SPI                 SERCOM1

/** AT25DFx device type */
#define AT25DFX_MEM_TYPE            AT25DFX_081A

#define AT25DFX_SPI_PINMUX_SETTING  SPI_SIGNAL_MUX_SETTING_C
#define AT25DFX_SPI_PINMUX_PAD0     PINMUX_PA16C_SERCOM1_PAD0
#define AT25DFX_SPI_PINMUX_PAD1     PINMUX_PA17C_SERCOM1_PAD1
#define AT25DFX_SPI_PINMUX_PAD2     PINMUX_PA18C_SERCOM1_PAD2
#define AT25DFX_SPI_PINMUX_PAD3     PINMUX_UNUSED

#define AT25DFX_CS                  PIN_PA19


//! SPI master speed in Hz.
#define AT25DFX_CLOCK_SPEED         3200000




#endif /* CONF_AT25DFX_H_ */