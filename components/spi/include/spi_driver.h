#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"

//Ports used for the SPI communication 
#define GRVCHP_SCLK 30
#define GRVCHP_MOSI 28
#define GRVCHP_MISO 29
#define GRVCHP_CS 31

//SD SPI driver initialization procedure
esp_err_t sdspi_driver_init(sdmmc_card_t* out_card);