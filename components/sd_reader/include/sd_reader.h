#include "../../spi/spi_driver.h" 
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

//Default mountpoint that we will use
#define GRVCHP_MNTPOINT "/sdcard/"

//Pointer to the FAT table
FATFS** fat_fs;


esp_err sd_reader_init ()