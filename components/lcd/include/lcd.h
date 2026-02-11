#include "esp_lcd_io_i2c.h"
#include "driver/i2c_master.h"

//Pin configurations  
#define GRVCHP_SDA 21
#define GRVCHP_SCL 22

//Parameters used in the configurations
#define GRVCHP_I2C_PORT_AUTO -1
#define GRVCHP_LCD_ADDR 0x27
#define GRVCHP_I2C_CLK_SPEED 200000

// LCD module defines (based on the 1602A datasheet)
#define LCD_LINEONE             0x00        // start of line 1
#define LCD_LINETWO             0x40        // start of line 2

#define LCD_BACKLIGHT           0x08
#define LCD_ENABLE              0x04               
#define LCD_COMMAND             0x00
#define LCD_WRITE               0x01

#define LCD_SET_DDRAM_ADDR      0x80

// LCD instructions (based on the 1602A datasheet)
#define LCD_CLEAR               0x01        // replace all characters with ASCII 'space'
#define LCD_HOME                0x02        // return cursor to first position on first line
#define LCD_FUNCTION_RESET      0x30        // reset the LCD
#define LCD_FUNCTION_SET_4BIT   0x20        // set 4-bit data length interface
#define LCD_ENTRY_MODE_SET      0x06        // entry mode set: Increment (I/D=1), No Shift (S=0)
#define LCD_SET_CURSOR          0x80        // set cursor position
#define LCD_CONFIGURATION       0x28        // 4-bit data, 2-line display, 5 x 7 font
#define LCD_DISPLAY_OFF         0x08        // set the display off
#define LCD_DISPLAY_ON          0x0C        // display ON: Display(D=1), Cursor(C=0), Blink(B=0)

void lcd_driver_init();
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_writeChar(char c);
void LCD_writeStr(char* str);
void LCD_clearScreen(void);
void lcd_task(void *args);
void print_single (char* in_row);
void print_double (char* first_in_row, char* sec_in_row);