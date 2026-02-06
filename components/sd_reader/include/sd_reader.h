/*********************************************************************************
 *                                   SD READER                                   *
 *        This module implements a driver to communicate with a SD card.         *
 *           It has been realized only to interact with SPI SD cards.            *
 *     The SD card is supposed to already be formatted using a FAT standard,     *
 *       that can be access using the file path "/sdcard" as a mountpoint.       *
 * Every sample is named based on the id, with extension "smp" (<sample_id>.smp) *
 *********************************************************************************/

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

//Maximum size of the file name
#define MAX_SIZE 20

/* Default initialization process:
   - Configure the physical connections and create the SD SPI bus 
   - Registrate the filesystem in the storage so that we can access it via C library functions (connection between FatFS and VFS)
   - Establish a connection between the FatFS and the SD card driver
   - Mount the SD card filesystem in a predetermined path (defined by GRVCHP_MNTPOINT)*/
esp_err_t sd_reader_init();

/* Function to transfer a sample from the SD card to the internal memory:
   - sample_id (IN): identifier of the sample we want to receive
   - out_sample (OUT): pointer to the sample that is contained in memory */
esp_err_t ld_sample(int sample_id, sample_t* out_sample);

/* Function to transfer a sample from the internal memory to the SD card:
   - sample (IN): actual sample of which the transfer is requested */
esp_err_t st_sample(sample_t *sample);