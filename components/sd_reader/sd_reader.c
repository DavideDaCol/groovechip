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

    //Connecting the POSIX and C standard library IO function with FATFS.
    res = esp_vfs_fat_register_cfg(&sd_mntpoint_conf, &fat_fs);
    if (res != ESP_OK){
        ESP_ERROR_CHECK_WITHOUT_ABORT(res);
        return res;
    }
    
    //SD diskio driver registration
    //To provide function pointer and define how the driver has to act with the SD card
    ff_diskio_register_sdmmc(GRVCHP_FAT_DRIVE_INDEX, &sd_card);

    //Registering the filesystem in the SD card to the FatFs module
    FRESULT fat_res = f_mount(fat_fs, GRVCHP_FAT_DRIVE_STR, 1); 
    if (fat_res != FR_OK) {
        fprintf(stderr, "Error in mounting the SD card filesystem\n");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t ld_sample(int in_sample_id, sample_t* out_sample) {
    //Defining the file path to read from
    char file_path[MAX_SIZE];
    sprintf(file_path, "%s/%d.smp", GRVCHP_MNTPOINT, in_sample_id);

    //Opening the file in read mode
    FILE* fp = fopen(file_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Sample not found\n");
        return ESP_ERR_NOT_FOUND;
    }

    //Assigning the file's content to the pointer out_sample
    size_t read_cnt = fread(out_sample, sizeof(sample_t), 1, fp);

    //Closing the file
    fclose(fp);

    if (read_cnt != 1) {
        fprintf(stderr, "Error while reading the file\n");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t st_sample(sample_t* in_sample) {
    int in_sample_id = in_sample->sample_id;

    //Defining the sample's file name, based on the id 
    char file_path[MAX_SIZE];
    sprintf(file_path, "%s/%d.smp", GRVCHP_MNTPOINT, in_sample_id);

    //Opening the file in write-or-create mode
    //If there's no file containing that sample, it will create one 
    FILE* fp = fopen(file_path, "wb");

    //Writing the sample inside the file
    size_t write_cnt = fwrite(in_sample, sizeof(sample_t), 1, fp);

    //Closing the file
    fclose(fp);

    if (write_cnt != 1) {
        fprintf(stderr, "Error while writing the file\n");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void create_wav_file (int sample_id) {

}