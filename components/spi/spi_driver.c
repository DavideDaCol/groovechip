#include "include/spi_driver.h"

//out_card = card-specific information
esp_err sdspi_driver_init(sdmmc_card_t* out_card) {
    
    //SPI bus configuration 
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GRVCHP_MOSI
        .miso_io_num = GRVCHP_MISO
        .sclk_io_num = GRVCHP_SCLK
    };

    //res = return error of esp_err functions
    esp_err res;
    
    //Bus initialization according to the defined configuration
    esp_err res = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (res != ESP_OK)
        return res;
    
    //SD SPI device configuration
    sdspi_device_config_t sdspi_conf = SDSPI_DEVICE_CONFIG_DEFAULT();
    sdspi_conf.host_id = SPI2_HOST;
    sdspi_conf.gpio_cs = GRVCHP_CS;


    sdspi_dev_handle_t sdspi_id = 0;
    
    //Attachement of the device to the bus
    res = sdspi_host_init_device(&sdspi_conf, &sdspi_id);
    if (res != ESP_OK)
        return res;
    
    //sdspi_host = settings, function pointers, rules the driver needs to follow
    sdmmc_host_t sdspi_host = SDSPI_HOST_DEFAULT();

    //Linking the host to the SD SPI bus 
    sdspi_host.slot = sdspi_id;
    
    return sdmmc_card_init(&sdspi_host, out_card);

}
