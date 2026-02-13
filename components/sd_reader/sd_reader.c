#include "include/sd_reader.h"
#include <cJSON.h>
#include <stdbool.h>
#include <string.h>

const char* TAG = "SdReader";

char** sample_names = NULL;
int sample_names_size = 0;

static sdmmc_card_t sd_card;

/* Function to explore the mount point and to fill the sample_names array*/
// static esp_err_t sd_exploration();
static esp_err_t sd_fs_init();
static esp_err_t set_json(char* dir_path, bool bitcrusher_enabled, uint8_t downsample, uint8_t bit_depth, float pitch_factor, bool distortion_enabled, uint16_t threshold, float gain, float start_ptr, uint32_t end_ptr);

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

    ESP_ERROR_CHECK_WITHOUT_ABORT(sd_fs_init());

    // ESP_ERROR_CHECK_WITHOUT_ABORT(sd_exploration());
    return ESP_OK;
}

esp_err_t ld_sample(int in_bank_index, char* sample_name, sample_t** out_sample_ptr) {
    ESP_LOGI(TAG, "ld_sample(): in_bank_index: %i, sample_name: %s", in_bank_index, sample_name);
    if (out_sample_ptr == NULL)
        return ESP_ERR_INVALID_ARG;

    //Defining the file path to read from
    char file_path[MAX_BUFF_SIZE];
    snprintf(file_path, sizeof(file_path),"%s/%s/%s.wav", GRVCHP_MNTPOINT, WAV_FILES_DIR, sample_name);
    ESP_LOGI(TAG, "requested file path is %s", file_path);

    //Opening the file in read mode
    FILE* fp = fopen(file_path, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG,"Sample not found");
        return ESP_ERR_NOT_FOUND;
    }
    
    //Allocating in the PSRAM the section of memory for the sample infos
    // *out_sample_ptr = heap_caps_malloc(sizeof(sample_t), MALLOC_CAP_SPIRAM);
    *out_sample_ptr = malloc(sizeof(sample_t));
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
    //out_sample -> raw_data = heap_caps_malloc((out_sample -> header).data_size, MALLOC_CAP_SPIRAM);
    out_sample -> raw_data = malloc((out_sample -> header).data_size);

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

// static esp_err_t sd_exploration() {
//     struct dirent* dir_entry;

//     ESP_LOGI(TAG, "begin SD exploration");

//     char inner_mnt[MAX_BUFF_SIZE];
//     if (snprintf(inner_mnt, sizeof(inner_mnt), "%s/%s", GRVCHP_MNTPOINT, WAV_FILES_DIR) < 0) return ESP_FAIL;
    
//     ESP_EARLY_LOGI(TAG, "trying to open path %s", inner_mnt);

//     //opening the SD filesystem via ANSI C functions 
//     DIR* dir = opendir(inner_mnt);

//     if (dir == NULL) {
//         return ESP_FAIL;
//     }

//     //obtaining the number of files (=entries in the directory) 
//     sample_names_size = 0;
//     while ((dir_entry = readdir(dir)) != NULL) sample_names_size++;
    
//     //allocating the array with the previously calculated size 
//     sample_names = malloc(sample_names_size*sizeof(char *));

//     //rewinding the directory stream
//     rewinddir(dir);

//     //Filling the array with the names of the samples in the SD card
//     for (int i = 0; i < sample_names_size; i++) {
//         dir_entry = readdir(dir);
//         if (dir_entry == NULL) {
//             return ESP_FAIL;
//         }
//         sample_names[i] = malloc(MAX_SIZE * sizeof(char));
//         sscanf(dir_entry -> d_name, "%[^.]", sample_names[i]);
//     }
//     return ESP_OK;
// }  


static esp_err_t sd_fs_init() {
    char wav_path[128], json_path[128];
    snprintf(wav_path, sizeof(wav_path), "%s/%s", GRVCHP_MNTPOINT, WAV_FILES_DIR);
    snprintf(json_path, sizeof(json_path), "%s/%s", GRVCHP_MNTPOINT, JSON_FILES_DIR);

    DIR *dir = opendir(wav_path);
    if (!dir) return ESP_FAIL;

    struct dirent* entry;
    int count = 0;

    //read directory entries while you can
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        //skip all files that don't end in .wav
        char *ext = strrchr(entry->d_name, '.');
        if (!ext || (strcasecmp(ext, ".wav") != 0)) continue;

        //extract and truncate the file name
        char clean_name[MAX_SIZE];
        snprintf(clean_name, sizeof(clean_name), "%.*s", (int)(ext - entry->d_name), entry->d_name);

        //if the name was truncated rename the original file
        char full_old_path[256];
        char full_new_path[256];
        snprintf(full_old_path, sizeof(full_old_path)*2, "%s/%s", wav_path, entry->d_name);
        snprintf(full_new_path, sizeof(full_new_path)*2, "%s/%s.wav", wav_path, clean_name);

        if (strcmp(entry->d_name, clean_name) != 0 && !strstr(entry->d_name, clean_name)) {
             if (rename(full_old_path, full_new_path) != 0) {
                 ESP_LOGE(TAG, "Rename failed: %s", entry->d_name);
                 continue; 
             }
        }

        //copy the necessary information for the json
        wav_header_t header;
        FILE* fp = fopen(full_new_path, "rb");
        if (fp) {
            fread(&header, sizeof(wav_header_t), 1, fp);
            fclose(fp);

            char json_file_path[256];
            snprintf(json_file_path, sizeof(json_file_path), "%s/%s.json", json_path, clean_name);
            
            set_json(json_file_path, false, DOWNSAMPLE_MAX, BIT_DEPTH_MAX, 1.0, 
                     false, DISTORTION_THRESHOLD_MAX, DISTORTION_GAIN_MAX, 0, header.data_size);
            
            count++; // Only increment count for valid processed WAVs
        }
    }

    sample_names = malloc(count * sizeof(char*));
    sample_names_size = count;

    //get back to the beginning of the directory
    rewinddir(dir);
    int idx = 0;
    while ((entry = readdir(dir)) != NULL && idx < count) {
        if (entry->d_name[0] == '.') continue;
        
        char *ext = strrchr(entry->d_name, '.');
        if (ext && strcasecmp(ext, ".wav") == 0) {
            //create the list of samples that the fsm will read
            sample_names[idx] = malloc(MAX_SIZE);
            snprintf(sample_names[idx], MAX_SIZE, "%.*s", (int)(ext - entry->d_name), entry->d_name);
            idx++;
        }
    }

    closedir(dir);
    return ESP_OK;
}

/*
"effects" : {
    "bitcrusher" : {
        "enabled" : <val>,
        "downsample" : <val>,
        "bit depth" : <val>
    },
    "pitch" : {
        "pitch_factor" : <val>
    },
    "distortion" : {
        "enabled" : <val>,
        "threshold" : <val>,
        "gain" : <val>
    }
},
"start_ptr" : <val>,
"end_ptr" : <val>
*/

static esp_err_t set_json(char* filename, bool bitcrusher_enabled, uint8_t downsample, uint8_t bit_depth, float pitch_factor, bool distortion_enabled, uint16_t threshold, float gain, float start_ptr, uint32_t end_ptr) {

    //strings to add in the JSON file
    char* bitcrusher_str = "bitcrusher";
    char* effects_str = "effects";
    char* downsample_str = "downsample";
    char* bit_depth_str = "bit depth";
    char* pitch_str = "pitch";
    char* pitch_factor_str = "pitch factor";
    char* distortion_str = "distortion";
    char* threshold_str = "threshold";
    char* gain_str = "gain";
    char* start_ptr_str = "start pointer";
    char* end_ptr_str = "end pointer";
    char* enabled_str = "enabled";

    //JSON setup, according to the structure defined above
    cJSON* distortion_json = cJSON_CreateObject();
    cJSON_AddBoolToObject(distortion_json, enabled_str, (cJSON_bool) distortion_enabled);
    cJSON_AddNumberToObject(distortion_json, threshold_str, threshold);
    cJSON_AddNumberToObject(distortion_json, gain_str, gain);
    
    cJSON* pitch_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(pitch_json, pitch_factor_str, pitch_factor);
    
    cJSON* bitcrusher_json = cJSON_CreateObject();
    cJSON_AddBoolToObject(bitcrusher_json, enabled_str, (cJSON_bool) bitcrusher_enabled);
    cJSON_AddNumberToObject(bitcrusher_json, downsample_str, downsample);
    cJSON_AddNumberToObject(bitcrusher_json, bit_depth_str, bit_depth);
    
    
    cJSON* effects_json = cJSON_CreateObject();
    cJSON_AddItemToObject(effects_json, distortion_str, distortion_json);
    cJSON_AddItemToObject(effects_json, pitch_str, pitch_json);
    cJSON_AddItemToObject(effects_json, bitcrusher_str, bitcrusher_json);
    

    cJSON* metadata_json = cJSON_CreateObject();
    cJSON_AddItemToObject(metadata_json, effects_str, effects_json);
    cJSON_AddNumberToObject(metadata_json, start_ptr_str, start_ptr);
    cJSON_AddNumberToObject(metadata_json, end_ptr_str, end_ptr);

    //Conversion of the JSON object into a string to copy into the file
    char* json_str = cJSON_Print(metadata_json);

    printf("%s\n", filename);
    printf("%s\n", json_str);

    FILE* fp = fopen(filename, "w");
    if (fp == NULL) return ESP_FAIL;

    fputs(json_str, fp);

    fclose(fp);
    free(json_str);
    cJSON_Delete(metadata_json);

    return ESP_OK;
}
