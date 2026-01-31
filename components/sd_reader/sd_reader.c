#include "include/sd_reader.h"

static sdmmc_card_t sd_card;

esp_err_t sd_reader_init() {
    esp_err_t res;
    
    //SD SPI driver initialization
    res = sdspi_driver_init(&sd_card);

    if (res != ESP_OK)
        return res;

    //Defining the mountpoint configuration
    esp_vfs_fat_conf_t sd_mntpoint_conf = {
        .base_path = GRVCHP_MNTPOINT,
        .fat_drive = GRVCHP_FAT_DRIVE_STR,
        .max_files = GRVCHP_MAX_FILES
    };

    //Pointer to the FAT table
    FATFS* fat_fs;

    //Connection of POSIX and C standard library IO function with FATFS.
    res = esp_vfs_fat_register_cfg(&sd_mntpoint_conf, &fat_fs);
    if (res != ESP_OK)
        return res;
    
    //SD diskio driver registration
    //To provide function pointer and define how the driver has to act with the SD card
    ff_diskio_register_sdmmc(GRVCHP_FAT_DRIVE_INDEX, &sd_card);

    //Registering the filesystem in the SD card to the FatFs module
    if (f_mount(fat_fs, GRVCHP_FAT_DRIVE_STR, 1) != FR_OK) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

