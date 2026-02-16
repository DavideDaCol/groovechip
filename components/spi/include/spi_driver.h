/**********************************************************************************
 *                                   SPI DRIVER                                   *
 * This module provides a standard initialization of a connection with an SD card *
 *                                    via SPI.                                    *
 *          The pins have been selected according to the ESP32 datasheet.         *
 **********************************************************************************/

#ifndef SPI_H
#define SPI_H

#include "sdmmc_cmd.h"

//Ports used for the SPI communication 
#define GRVCHP_SCLK GPIO_NUM_18
#define GRVCHP_MOSI GPIO_NUM_23
#define GRVCHP_MISO GPIO_NUM_19
#define GRVCHP_CS GPIO_NUM_5

/*
@brief initializes the SD SPI driver
@param out_card pointer to the card's informations structure
*/
esp_err_t sdspi_driver_init(sdmmc_card_t* out_card);

#endif