#include "../../spi/include/spi_driver.h" 
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "diskio_sdmmc.h"

//Default mountpoint
#define GRVCHP_MNTPOINT "/sdcard"

//FAT drive index (0 by default)
#define GRVCHP_FAT_DRIVE_INDEX 0
#define GRVCHP_FAT_DRIVE_STR "0:"

//Maximum number of files that can be opened simultaneously
#define GRVCHP_MAX_FILES 1


/* Default initialization process:
*  - Configure the physical connections and create the SD SPI bus 
*  - Registrate the filesystem in the storage so that we can access it via C library functions (connection between FatFS and VFS)
*  - Establish a connection between the FatFS and the SD card driver
*  - Mount the SD card filesystem in a predetermined path (defined by GRVCHP_MNTPOINT)
*/
esp_err_t sd_reader_init();