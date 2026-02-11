#include "include/lcd.h"
#include "driver/i2c_master.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "rom/ets_sys.h"
#include <esp_log.h>

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
#define LCD_FUNCTION_SET_4BIT   0x20        // 4-bit data, 2-line display, 5 x 7 font
#define LCD_SET_CURSOR          0x80        // set cursor position

static void i2c_init();
static void LCD_writeNibble(uint8_t nibble, uint8_t mode);
static void LCD_writeByte(uint8_t data, uint8_t mode);
static void LCD_pulseEnable(uint8_t nibble);
i2c_master_dev_handle_t lcd_handle;

static void i2c_init() {
    i2c_master_bus_handle_t i2c_bus;

    //Defining the I2C bus configuration
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = GRVCHP_I2C_PORT_AUTO,
        .sda_io_num = GRVCHP_SDA,
        .scl_io_num = GRVCHP_SCL,
        .flags.enable_internal_pullup = true,
    };

    //Creating the I2C bus
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    //Configuring the LCD handler
    i2c_device_config_t lcd_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = GRVCHP_LCD_ADDR,
        .scl_speed_hz = GRVCHP_I2C_CLK_SPEED,
    };

    //Instantiating the LCD handler
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &lcd_cfg, &lcd_handle));
}

void lcd_driver_init() {
    i2c_init();

    // 1. Hardware Reset (Strictly follows datasheet Page 17)
    LCD_writeNibble(0x30, LCD_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(20));      // Wait >4.1ms
    LCD_writeNibble(0x30, LCD_COMMAND);
    ets_delay_us(200);                  // Wait >100us
    LCD_writeNibble(0x30, LCD_COMMAND);
    
    // 2. Switch to 4-bit Mode (Critical: Send once as a nibble)
    LCD_writeNibble(0x20, LCD_COMMAND); 
    ets_delay_us(80);

    // 3. Configure Settings (Now use writeByte for safety/simplicity)
    
    // Function Set: 4-bit, 2-line, 5x8 font -> 0x28
    LCD_writeByte(0x28, LCD_COMMAND);
    
    // Display OFF -> 0x08
    LCD_writeByte(0x08, LCD_COMMAND);
    
    // Clear Display -> 0x01 (Use your function to ensure 2ms delay!)
    LCD_clearScreen(); 
    
    // Entry Mode Set: Increment (I/D=1), No Shift (S=0) -> 0x06
    LCD_writeByte(0x06, LCD_COMMAND);
    
    // Display ON: Display(D=1), Cursor(C=0), Blink(B=0) -> 0x0C
    LCD_writeByte(0x0C, LCD_COMMAND);
}


void LCD_setCursor(uint8_t col, uint8_t row)
{
    row %= 2;
    uint8_t row_offsets[] = {LCD_LINEONE, LCD_LINETWO};
    LCD_writeByte(LCD_SET_DDRAM_ADDR | (col + row_offsets[row]), LCD_COMMAND);
}

void LCD_writeChar(char c)
{
    LCD_writeByte(c, LCD_WRITE);                                        // Write data to DDRAM
}

void LCD_writeStr(char* str)
{
    while (*str) {
        LCD_writeChar(*str++);
    }
}

void LCD_home(void)
{
    LCD_writeByte(LCD_HOME, LCD_COMMAND);
    vTaskDelay(2 / portTICK_PERIOD_MS);                                   // This command takes a while to complete
}

void LCD_clearScreen(void)
{
    LCD_writeByte(LCD_CLEAR, LCD_COMMAND);
    vTaskDelay(2 / portTICK_PERIOD_MS);                                   // This command takes a while to complete
}

static void LCD_writeNibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data = (nibble & 0xF0) | mode | LCD_BACKLIGHT;
    i2c_master_transmit(lcd_handle, &data, 1, 2000 / portTICK_PERIOD_MS);
    LCD_pulseEnable(data);                                              // Clock data into LCD
}

static void LCD_writeByte(uint8_t data, uint8_t mode)
{
    LCD_writeNibble(data & 0xF0, mode);
    LCD_writeNibble((data << 4) & 0xF0, mode);
}

static void LCD_pulseEnable(uint8_t data)
{
    uint8_t buf;
    buf = data | LCD_ENABLE;
    i2c_master_transmit(lcd_handle, &buf, 1, 2000 / portTICK_PERIOD_MS);
    ets_delay_us(1);
    buf = (data & ~LCD_ENABLE);
    i2c_master_transmit(lcd_handle, &buf, 1, 2000 / portTICK_PERIOD_MS);
    ets_delay_us(500);
}