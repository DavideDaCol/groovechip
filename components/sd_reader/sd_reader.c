#include "include/sd_reader.h"
#include <sys/stat.h>

char** sample_names = NULL;
int sample_names_size = 0;

static sdmmc_card_t sd_card;

/* Function to explore the mount point and to fill the sample_names array*/
static esp_err_t sd_exploration();
static esp_err_t set_new_samples();

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

    set_new_samples();

    return sd_exploration();
}

esp_err_t ld_sample(int in_bank_index, char* sample_name, sample_t** out_sample_ptr) {
    if (out_sample_ptr == NULL)
        return ESP_ERR_INVALID_ARG;

    //Defining the file path to read from
    char file_path[MAX_SIZE];
    sprintf(file_path, "%s/%s.wav", GRVCHP_MNTPOINT, sample_name);

    //Opening the file in read mode
    FILE* fp = fopen(file_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Sample not found\n");
        return ESP_ERR_NOT_FOUND;
    }
    
    //Allocating in the PSRAM the section of memory for the sample infos
    *out_sample_ptr = heap_caps_malloc(sizeof(sample_t), MALLOC_CAP_SPIRAM);
    
    sample_t* out_sample = *out_sample_ptr;

    //Assigning the file's content to the pointer out_sample_ptr
    size_t read_cnt = fread(&(out_sample -> header), sizeof(wav_header_t), 1, fp);

    //If the read doesn't succeed
    if (read_cnt != 1) {
        fprintf(stderr, "Error while reading the file\n");

        free(out_sample);
        out_sample_ptr = NULL;
        return ESP_FAIL;
    }

    //Allocating the section of memory for the actual sample (wav buffer)
    out_sample -> raw_data = heap_caps_malloc((out_sample -> header).data_size, MALLOC_CAP_SPIRAM);

    //Reading the area of the file where the data is located
    read_cnt = fread(out_sample -> raw_data, 1, (out_sample -> header).data_size, fp);

    if (read_cnt != (out_sample -> header).data_size) {
        fprintf(stderr, "Error while reading the file\n");
        fprintf(stderr, "Read data: %d\n", read_cnt);

        free(out_sample -> raw_data);
        free(out_sample);
        return ESP_FAIL;
    }

    //Setting some default values
    out_sample -> bank_index = in_bank_index;
    out_sample -> start_ptr = 0.0f;
    out_sample -> end_ptr = (out_sample -> header).data_size;
    out_sample -> volume = 0.1f;
    out_sample -> playback_ptr = out_sample -> start_ptr;
    out_sample -> total_frames = (out_sample -> header).data_size / 4; 

    //Closing the file
    fclose(fp);

    return ESP_OK;
}

esp_err_t st_sample(sample_t* in_sample) {
    int in_sample_id = in_sample->bank_index;

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

static esp_err_t sd_exploration() {
    struct dirent* dir_entry;

    //opening the SD filesystem via ANSI C functions 
    DIR* dir = opendir(GRVCHP_MNTPOINT);

    if (dir == NULL) {
        return ESP_FAIL;
    }

    //obtaining the number of files (=entries in the directory) 
    sample_names_size = 0;
    while ((dir_entry = readdir(dir)) != NULL) sample_names_size++;
    
    //allocating the array with the previously calculated size 
    sample_names = malloc(sample_names_size*sizeof(char *));

    //rewinding the directory stream
    rewinddir(dir);

    //Filling the array with the names of the samples in the SD card
    for (int i = 0; i < sample_names_size; i++) {
        dir_entry = readdir(dir);
        if (dir_entry == NULL) {
            return ESP_FAIL;
        }
        sample_names[i] = malloc(MAX_SIZE * sizeof(char));
        sscanf(dir_entry -> d_name, "%s", sample_names[i]);
    }
    return ESP_OK;
}  


static esp_err_t set_new_samples() {
    //generating a file path that brings to the directory with the samples to insert
    char file_path[GRVCHP_MNTPOINT_SIZE + NEW_SAMPLES_DIR_SIZE + 2];
    snprintf(file_path, sizeof(file_path), "%s/%s", GRVCHP_MNTPOINT, NEW_SAMPLES_DIR);
    
    //opening the directory
    DIR *new_files_dir = opendir(file_path);
    if (!new_files_dir) return ESP_FAIL;

    struct dirent* dir_entry;

    //buffers that will contain the file names
    char temp_path1[MAX_BUFF_SIZE];
    char temp_path2[MAX_BUFF_SIZE + GRVCHP_MNTPOINT_SIZE + NEW_SAMPLES_DIR_SIZE + 5];
    char sample_name[MAX_SIZE + 1];

    //
    while ((dir_entry = readdir(new_files_dir)) != NULL) {
        if (dir_entry->d_name[0] == '.') continue;

        if (sscanf(dir_entry->d_name, RESOLVE(MAX_SIZE), sample_name) == 1) {
            // Construct paths using snprintf for safety
            snprintf(temp_path1, sizeof(temp_path1), "%s/%s", GRVCHP_MNTPOINT, sample_name);
            
            // Always check if directory creation was successful or if it already exists
            mkdir(temp_path1, STD_ACCESS_MODE);

            snprintf(temp_path2, sizeof(temp_path2), "%s/%s", file_path, dir_entry->d_name);
            // Reuse temp_path1 carefully or use a third buffer
            char destination[MAX_BUFF_SIZE];
            snprintf(destination, sizeof(destination), "%s/%s/%s", GRVCHP_MNTPOINT, sample_name, SAMPLE_FL_NAME);
            
            if (rename(temp_path2, destination) != 0) {
                return ESP_FAIL;
            }
        }
    }

    closedir(new_files_dir);
    return ESP_OK;
}

esp_err_t 