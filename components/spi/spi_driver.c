#include "include/spi_driver.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"

esp_err_t sdspi_driver_init(sdmmc_card_t* out_card) {
    
    //Defining the SPI bus configuration 
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GRVCHP_MOSI,
        .miso_io_num = GRVCHP_MISO,
        .sclk_io_num = GRVCHP_SCLK,
        .max_transfer_sz = 4000
    };

    //res = return error of esp_err functions
    esp_err_t res;
    
    //Initializing the bus according to the defined configuration
    res = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (res != ESP_OK){
        ESP_ERROR_CHECK_WITHOUT_ABORT(res);
        return res;
    }
    
    //SD SPI device configuration
    sdspi_device_config_t sdspi_conf = SDSPI_DEVICE_CONFIG_DEFAULT();
    sdspi_conf.host_id = SPI2_HOST;
    sdspi_conf.gpio_cs = GRVCHP_CS;


    sdspi_dev_handle_t sdspi_id = 0;
    
    //Attaching the device to the bus
    res = sdspi_host_init_device(&sdspi_conf, &sdspi_id);
    if (res != ESP_OK){
        ESP_ERROR_CHECK_WITHOUT_ABORT(res);
        return res;
    }

    //sdspi_host = settings, function pointers, rules the driver needs to follow
    sdmmc_host_t sdspi_host = SDSPI_HOST_DEFAULT();

    //Linking the host to the SD SPI bus 
    sdspi_host.slot = sdspi_id;
    res = sdmmc_card_init(&sdspi_host, out_card);
    if (res != ESP_OK)
        ESP_ERROR_CHECK_WITHOUT_ABORT(res);
    
    return res;
}
