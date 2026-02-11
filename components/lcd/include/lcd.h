#include "esp_lcd_io_i2c.h"
#include "driver/i2c_master.h"

//Pin configurations  
#define GRVCHP_SDA 21
#define GRVCHP_SCL 22

//Parameters used in the configurations
#define GRVCHP_I2C_PORT_AUTO -1
#define GRVCHP_LCD_ADDR 0x27
#define GRVCHP_I2C_CLK_SPEED 100000

void lcd_driver_init();
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_writeChar(char c);
void LCD_writeStr(char* str);
void LCD_clearScreen(void);