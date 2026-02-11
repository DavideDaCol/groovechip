#include "include/lcd.h"
#include "driver/i2c_master.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "rom/ets_sys.h"
#include <esp_log.h>

static void i2c_init();
static void LCD_writeNibble(uint8_t nibble, uint8_t mode);
static void LCD_writeByte(uint8_t data, uint8_t mode);
static void LCD_pulseEnable(uint8_t nibble);
i2c_master_dev_handle_t lcd_handle;
static QueueHandle_t lcd_queue;

typedef struct {
    char *first_row;
    char *sec_row;
} lcd_msg_t;

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
    
    // LCD initialization according to the 1602A datasheet
    // performing hardware reset (Strictly follows datasheet Page 17)
    LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(20));      // Wait >4.1ms
    LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);
    ets_delay_us(200);                  // Wait >100us
    LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);
    
    // switching to 4-bit Mode (Critical: Send once as a nibble)
    LCD_writeNibble(LCD_FUNCTION_SET_4BIT, LCD_COMMAND); 
    ets_delay_us(80);

    // sending LCD configuration
    LCD_writeByte(LCD_CONFIGURATION, LCD_COMMAND);
    
    // turning LCD off
    LCD_writeByte(LCD_DISPLAY_OFF, LCD_COMMAND);
    
    // clearing screen
    LCD_clearScreen(); 
    
    // setting entry mode
    LCD_writeByte(LCD_ENTRY_MODE_SET, LCD_COMMAND);
    
    // turning LCD display on 
    LCD_writeByte(LCD_DISPLAY_ON, LCD_COMMAND);

    lcd_queue = xQueueCreate(10, sizeof(lcd_msg_t*));

    xTaskCreate(lcd_task, "LCD Task", 2048, NULL, 5, NULL);
}


void LCD_setCursor(uint8_t col, uint8_t row) {
    row %= 2;
    uint8_t row_offsets[] = {LCD_LINEONE, LCD_LINETWO};
    LCD_writeByte(LCD_SET_DDRAM_ADDR | (col + row_offsets[row]), LCD_COMMAND);
}

void LCD_writeChar(char c) {
    LCD_writeByte(c, LCD_WRITE);                                        // Write data to DDRAM
}

void LCD_writeStr(char* str) {
    while (*str) {
        LCD_writeChar(*str++);
    }
}

void LCD_home(void) {
    LCD_writeByte(LCD_HOME, LCD_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(2));                                   // This command takes a while to complete
}

void LCD_clearScreen(void) {
    LCD_writeByte(LCD_CLEAR, LCD_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static void LCD_writeNibble(uint8_t nibble, uint8_t mode) {
    uint8_t data = (nibble & 0xF0) | mode | LCD_BACKLIGHT;
    i2c_master_transmit(lcd_handle, &data, 1, pdMS_TO_TICKS(2000));
    LCD_pulseEnable(data);                                              
}

static void LCD_writeByte(uint8_t data, uint8_t mode) {
    LCD_writeNibble(data & 0xF0, mode);
    LCD_writeNibble((data << 4) & 0xF0, mode);
}

static void LCD_pulseEnable(uint8_t data) {
    uint8_t buf;
    buf = data | LCD_ENABLE;
    i2c_master_transmit(lcd_handle, &buf, 1, 2000 / portTICK_PERIOD_MS);
    buf = (data & ~LCD_ENABLE);
    i2c_master_transmit(lcd_handle, &buf, 1, 2000 / portTICK_PERIOD_MS);
    ets_delay_us(500);
}


void lcd_task(void *args) {
    lcd_msg_t* msg_ptr = NULL; // This will hold the pointer received from queue

    while(1) {
        printf("Ciao\n");
        // Pass the ADDRESS of the pointer variable (&msg_ptr)
        // FreeRTOS copies the pointer from the queue into msg_ptr
        if (xQueueReceive(lcd_queue, &msg_ptr, portMAX_DELAY) == pdPASS) {
            
            printf("Ciao bello\n");
            // Now msg_ptr points to the malloc'd struct
            if (msg_ptr != NULL) {
                LCD_clearScreen(); // Good practice to clear before writing new frame
                
                LCD_setCursor(0, 0); 
                LCD_writeStr(msg_ptr->first_row);
                
                LCD_setCursor(0, 1);
                LCD_writeStr(msg_ptr->sec_row);
                
                // IMPORTANT: Free the memory allocated in the sender
                free(msg_ptr);
            }
        }
    }
}

void print_single (char* in_row) {
    // 1. Allocate memory for the message struct
    lcd_msg_t* new_msg = malloc(sizeof(lcd_msg_t));
    if (new_msg == NULL) return; // Safety check

    new_msg->first_row = in_row;
    // Note: Ensure "in_row" stays valid! (e.g., String literal or global)
    // If "in_row" is a local array from the caller, it will vanish before the task reads it.
    
    new_msg->sec_row = "           ";

    // 2. Send the POINTER to the queue
    // We pass &new_msg because xQueueSend copies the CONTENT of new_msg (which is the pointer address)
    if (xQueueSend(lcd_queue, &new_msg, 0) != pdPASS) {
        free(new_msg); // Free immediately if queue is full to avoid leak
    }
}

void print_double (char* first_in_row, char* sec_in_row) {
    lcd_msg_t* new_msg = malloc(sizeof(lcd_msg_t));
    if (new_msg == NULL) return;

    new_msg->first_row = first_in_row;
    new_msg->sec_row = sec_in_row;

    if (xQueueSend(lcd_queue, &new_msg, 0) != pdPASS) {
        free(new_msg);
    }
}