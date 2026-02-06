/**********************************************************************************
 *                                   SPI DRIVER                                   *
 * This module provides a standard initialization of a connection with an SD card *
 *                                    via SPI.                                    *
 *          The pins have been selected according to the ESP32 datasheet.         *
 **********************************************************************************/

#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"

//Ports used for the SPI communication 
#define GRVCHP_SCLK 18
#define GRVCHP_MOSI 23
#define GRVCHP_MISO 19
#define GRVCHP_CS 5

/* SD SPI initialization procedure
*  - out_card (OUT): pointer to the card's info
*/
esp_err_t sdspi_driver_init(sdmmc_card_t* out_card);