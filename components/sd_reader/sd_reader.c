#include "include/sd_reader.h"
#include <cJSON.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

const char* TAG = "SdReader";

char** sample_names = NULL;
int sample_names_size = 0;

static sdmmc_card_t sd_card;

/* Function to explore the mount point and to fill the sample_names array*/
// static esp_err_t sd_exploration();
static esp_err_t sd_fs_init();
static esp_err_t set_json(char* dir_path, bool bitcrusher_enabled, uint8_t downsample, uint8_t bit_depth, float pitch_factor, bool distortion_enabled, uint16_t threshold, float gain, float start_ptr, uint32_t end_ptr);
static esp_err_t get_json(char *filename, bool* bitcrusher_enabled, uint8_t* downsample, uint8_t* bit_depth, float* pitch_factor, bool* distortion_enabled, uint16_t* threshold, float* gain, float* start_ptr, uint32_t* end_ptr);

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

    if (*out_sample_ptr != NULL) {
        heap_caps_free((*out_sample_ptr) -> raw_data);
        heap_caps_free((*out_sample_ptr));
    }
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
    
    printf("Available size: %d", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    //Allocating in the PSRAM the section of memory for the sample infos
    // *out_sample_ptr = heap_caps_malloc(sizeof(sample_t), MALLOC_CAP_SPIRAM);
    *out_sample_ptr = heap_caps_malloc(sizeof(sample_t), MALLOC_CAP_SPIRAM);
    sample_t* out_sample = *out_sample_ptr;

    //Assigning the file's content to the pointer out_sample_ptr
    size_t read_cnt = fread(&(out_sample -> header), sizeof(wav_header_t), 1, fp);

    //If the read doesn't succeed
    if (read_cnt != 1) {
        ESP_LOGE(TAG, "Error while reading the file\n");

        free(out_sample);
        out_sample_ptr = NULL;
        return ESP_FAIL;
    }

    
    //Allocating the section of memory for the actual sample (wav buffer)
    //out_sample -> raw_data = heap_caps_malloc((out_sample -> header).data_size, MALLOC_CAP_SPIRAM);
    out_sample->raw_data = heap_caps_malloc(out_sample -> header.data_size, MALLOC_CAP_SPIRAM);
    if (out_sample->raw_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate SPIRAM for WAV data");
        heap_caps_free(out_sample);
        *out_sample_ptr = NULL;
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }
    
    //Reading the area of the file where the data is located
    read_cnt = fread(out_sample -> raw_data, 1, (out_sample -> header).data_size, fp);

    
    if (read_cnt != (out_sample -> header).data_size) {
        ESP_LOGE(TAG, "Error while reading the file\n");
        ESP_LOGE(TAG, "Read data: %d\n", read_cnt);
        
        heap_caps_free(out_sample -> raw_data);
        heap_caps_free(out_sample);
        return ESP_FAIL;
    }
    
    //getting the json filename
    char filename[MAX_BUFF_SIZE];
    snprintf(filename, sizeof(filename), "%s/%s/%s.json", GRVCHP_MNTPOINT, JSON_FILES_DIR, sample_name);
    
    //Closing the file
    fclose(fp);

    //declaring some useful variables for the json parsing
    bool bitcrusher_enabled;
    bool distortion_enabled;
    uint8_t downsample;
    uint8_t bit_depth;
    float gain;
    uint16_t threshold;
    float pitch_factor;

    out_sample -> bank_index = in_bank_index;

    //json parsing 
    get_json(
        filename,
        &bitcrusher_enabled,
        &downsample,
        &bit_depth,
        &pitch_factor,
        &distortion_enabled,
        &threshold,
        &gain,
        &(out_sample -> start_ptr),
        &(out_sample -> end_ptr) 
    );

    //setting default values
    out_sample -> volume = 0.1f;
    out_sample -> playback_ptr = out_sample -> start_ptr;
    out_sample -> total_frames = (out_sample -> header).data_size / 4; 

    //assigning bitcrusher values according to the infos in the json file
    set_bit_crusher(in_bank_index, bitcrusher_enabled);
    set_bit_crusher_bit_depth(in_bank_index, bit_depth);
    set_bit_crusher_downsample(in_bank_index, downsample);

    //same for distortion
    set_distortion(in_bank_index, distortion_enabled);
    set_distortion_gain(in_bank_index, gain);
    set_distortion_threshold(in_bank_index, threshold);

    //same for the pitch
    set_pitch_factor(in_bank_index, pitch_factor);

    return ESP_OK;
}

esp_err_t st_sample(int in_bank_index, char *sample_name) {
    sample_t* curr_sample = sample_bank[in_bank_index];

    //Defining the sample's file name, based on the id 
    char wav_file_path[MAX_BUFF_SIZE];
    snprintf(wav_file_path, sizeof(wav_file_path), "%s/%s/%s.wav", GRVCHP_MNTPOINT, WAV_FILES_DIR, sample_name);

    char json_file_path[MAX_BUFF_SIZE];
    snprintf(json_file_path, sizeof(json_file_path), "%s/%s/%s.json", GRVCHP_MNTPOINT, JSON_FILES_DIR, sample_name);

    printf("%s, %s\n", wav_file_path, json_file_path);

    //Opening the file in write-or-create mode
    //If there's no file containing that sample, it will create one 
    FILE* wav_fp;
    if ((wav_fp = fopen(wav_file_path, "r")) != NULL) {
        fclose(wav_fp);
        if ((wav_fp = fopen(wav_file_path, "wb")) == NULL) {
            ESP_LOGE(TAG, "Error in creating the new wav file");
            return ESP_FAIL;
        }

        //Writing the sample inside the file
        size_t write_cnt = fwrite(&(curr_sample -> header), sizeof(wav_header_t), 1, wav_fp);
        if (write_cnt != 1) {
            ESP_LOGE(TAG, "Error while writing the wav file's header");
            fclose(wav_fp);
            return ESP_FAIL;
        }

        write_cnt = fwrite(curr_sample -> raw_data, curr_sample -> header.data_size, 1, wav_fp);
        if (write_cnt != 1) {
            ESP_LOGE(TAG, "Error while writing the wav file");
            fclose(wav_fp);
            return ESP_FAIL;
        }
    }
    
    fclose(wav_fp);

    set_json(
        json_file_path,
        get_bit_crusher_state(in_bank_index),
        get_bit_crusher_downsample(in_bank_index),
        get_bit_crusher_bit_depth(in_bank_index),
        get_pitch_factor(in_bank_index),
        get_distortion_state(in_bank_index),
        get_distortion_threshold(in_bank_index),
        get_distortion_gain(in_bank_index),
        curr_sample -> start_ptr,
        curr_sample -> end_ptr
    );

    return ESP_OK;
}

static esp_err_t sd_fs_init() {
    char wav_path[MAX_BUFF_SIZE/2], json_path[MAX_BUFF_SIZE/2];
    snprintf(wav_path, sizeof(wav_path), "%s/%s", GRVCHP_MNTPOINT, WAV_FILES_DIR);
    snprintf(json_path, sizeof(json_path), "%s/%s", GRVCHP_MNTPOINT, JSON_FILES_DIR);

    DIR *dir = opendir(wav_path);
    if (!dir) {
        ESP_LOGE(TAG, "Error: directory not found");
        return ESP_ERR_NOT_FOUND;
    }

    struct dirent* entry;
    int count = 0;

    //read directory entries while you can
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        
        //skip all files that don't end in .wav
        char *ext = strrchr(entry->d_name, '.');
        if (!ext || (strcasecmp(ext, ".wav") != 0)) {
            ESP_LOGE(TAG, "Analyzed file is not a WAV file, ignoring");
            continue;
        }
        
        //extract and truncate the file name
        char clean_name[MAX_SIZE];
        snprintf(clean_name, sizeof(clean_name), "%.*s", (int)(ext - entry->d_name), entry->d_name);
        
        //if the name was truncated rename the original file
        char full_old_path[MAX_BUFF_SIZE/2 + MAX_BUFF_SIZE + 1];
        char full_new_path[MAX_BUFF_SIZE/2 + MAX_SIZE + WAV_EXTENSION_SIZE];
        snprintf(full_old_path, sizeof(full_old_path), "%s/%s", wav_path, entry->d_name);
        snprintf(full_new_path, sizeof(full_new_path), "%s/%s.wav", wav_path, clean_name);
        
        if (strcmp(entry->d_name, clean_name) != 0) {
            char check_name[MAX_SIZE + WAV_EXTENSION_SIZE];
            snprintf(check_name, sizeof(check_name), "%s.wav", clean_name);
            
            if (strcmp(entry->d_name, check_name) != 0) {
                if (rename(full_old_path, full_new_path) != 0) {
                    ESP_LOGE(TAG, "Rename failed for: %s", entry->d_name);
                    continue; 
                }
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
            
            printf("%s\n", json_file_path);
            FILE* check_fp;
            if ((check_fp = fopen(json_file_path, "r")) != NULL) {
                fclose(check_fp);
                count++;
                continue;
            }
            fclose(check_fp);
            set_json(json_file_path, false, 1, BIT_DEPTH_MAX, 1.0, 
                     false, DISTORTION_THRESHOLD_MAX, DISTORTION_GAIN_MAX, 0, header.data_size);
            
            count++; // Only increment count for valid processed WAVs
        } else {
            ESP_LOGE(TAG, "Error in opening the WAV file, %s", strerror(errno));
            continue;
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
    printf("Start pointer: %f, End pointer: %ld\n", start_ptr, end_ptr);
    printf("Gain: %f\n", gain);

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
    char* start_ptr_str = "start_pointer";
    char* end_ptr_str = "end_pointer";
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
    printf("%s\n", json_str);

    FILE* fp = fopen(filename, "w");
    if (fp == NULL) return ESP_FAIL;

    fputs(json_str, fp);

    fclose(fp);
    free(json_str);
    cJSON_Delete(metadata_json);

    ESP_LOGI(TAG, "JSON creation completed");

    return ESP_OK;
}

static esp_err_t get_json(char *filename, bool* bitcrusher_enabled, uint8_t* downsample, uint8_t* bit_depth, float* pitch_factor, bool* distortion_enabled, uint16_t* threshold, float* gain, float* start_ptr, uint32_t* end_ptr) {
    
    //fields in the JSON file
    char* bitcrusher_str = "bitcrusher";
    char* effects_str = "effects";
    char* downsample_str = "downsample";
    char* bit_depth_str = "bit depth";
    char* pitch_str = "pitch";
    char* pitch_factor_str = "pitch factor";
    char* distortion_str = "distortion";
    char* threshold_str = "threshold";
    char* gain_str = "gain";
    char* start_ptr_str = "start_pointer";
    char* end_ptr_str = "end_pointer";
    char* enabled_str = "enabled";

    //opening the json file
    FILE* fp = fopen(filename, "r");

    //reading the content and store it in a buffer
    char buffer[MAX_JSON_SIZE];
    if (fread(buffer, 1, sizeof(buffer), fp) == 0) {
        fclose(fp);
        return ESP_FAIL;  
    } 
    
    fclose(fp);

    printf("%s\n", buffer);

    //parsing the document
    cJSON* metadata_json = cJSON_Parse(buffer);
    if (metadata_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "%s", error_ptr);
        }
        cJSON_Delete(metadata_json);
        return ESP_FAIL;
    }

    //getting start_ptr
    cJSON* start_ptr_json = cJSON_GetObjectItemCaseSensitive(metadata_json, start_ptr_str);
    //type checking
    if (cJSON_IsNumber(start_ptr_json)) {
        *start_ptr = (float) start_ptr_json -> valuedouble;
    } else {
        ESP_LOGE(TAG, "Wrong %s formatting", start_ptr_str);
        cJSON_Delete(metadata_json);
        return ESP_FAIL;
    }

    //getting end_ptr
    cJSON* end_ptr_json = cJSON_GetObjectItemCaseSensitive(metadata_json, end_ptr_str); 
    if (cJSON_IsNumber(end_ptr_json)) {
        *end_ptr = (uint32_t) end_ptr_json -> valuedouble;
    } else {
        ESP_LOGE(TAG, "Wrong %s formatting", end_ptr_str);
        cJSON_Delete(metadata_json);
        return ESP_FAIL;
    }

    cJSON* effects_json = cJSON_GetObjectItemCaseSensitive(metadata_json, effects_str);
    if (cJSON_IsObject(effects_json)) {
        
        //effects -> pitch parsing
        cJSON* pitch_json = cJSON_GetObjectItemCaseSensitive(effects_json, pitch_str);
        if (cJSON_IsObject(pitch_json)) {

            //getting the effects -> pitch -> pitch factor
            cJSON *pitch_factor_json = cJSON_GetObjectItemCaseSensitive(pitch_json, pitch_factor_str);
            //type checking
            if (cJSON_IsNumber(pitch_factor_json)) {
                *pitch_factor = (float) pitch_factor_json -> valuedouble;
            } else {
                ESP_LOGE(TAG, "Wrong %s formatting", pitch_factor_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }
        }

        //effects -> distortion parsing
        cJSON *distortion_json = cJSON_GetObjectItemCaseSensitive(effects_json, distortion_str);
        if (cJSON_IsObject(distortion_json)) {

            //getting the effects -> distortion -> enable value
            cJSON *enabled_json = cJSON_GetObjectItemCaseSensitive(distortion_json, enabled_str);
            //type checking
            if (cJSON_IsBool(enabled_json)) {
                *distortion_enabled = (bool) cJSON_IsTrue(enabled_json);
            } else {
                ESP_LOGE(TAG, "Wrong distortion %s formatting", enabled_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }

            //getting the effects -> distortion -> gain
            cJSON *gain_json = cJSON_GetObjectItemCaseSensitive(distortion_json, gain_str);
            //type checking
            if (cJSON_IsNumber(gain_json)) {
                *gain = (float) gain_json -> valuedouble;
            } else {
                ESP_LOGE(TAG, "Wrong %s formatting", gain_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }

            //getting the effects -> distortion -> threshold
            cJSON *threshold_json = cJSON_GetObjectItemCaseSensitive(distortion_json, threshold_str);
            //type checking
            if (cJSON_IsNumber(threshold_json)) {
                *threshold = (float) threshold_json -> valuedouble;
            } else {
                ESP_LOGE(TAG, "Wrong %s formatting", threshold_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }
        }

        //effects -> bitcrusher parsing
        cJSON *bitcrusher_json = cJSON_GetObjectItemCaseSensitive(effects_json, bitcrusher_str);
        if (cJSON_IsObject(bitcrusher_json)) {

            //getting the effects -> bitcrusher -> enable value
            cJSON *enabled_json = cJSON_GetObjectItemCaseSensitive(bitcrusher_json, enabled_str);
            //type checking
            if (cJSON_IsBool(enabled_json)) {
                *bitcrusher_enabled = cJSON_IsTrue(enabled_json);
            } else {
                ESP_LOGE(TAG, "Wrong bitcrusher %s formatting", enabled_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }

            //getting the effects -> bitcrusher -> downsample
            cJSON *downsample_json = cJSON_GetObjectItemCaseSensitive(bitcrusher_json, downsample_str);
            //type checking
            if (cJSON_IsNumber(downsample_json)) {
                *downsample = (uint8_t) downsample_json -> valuedouble;
            } else {
                ESP_LOGE(TAG, "Wrong %s formatting", downsample_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }

            //getting the effects -> bitcrusher -> bit depth
            cJSON *bit_depth_json = cJSON_GetObjectItemCaseSensitive(bitcrusher_json, bit_depth_str);
            //type checking
            if (cJSON_IsNumber(bit_depth_json)) {
                *bit_depth = (uint8_t) bit_depth_json -> valuedouble;
            } else {
                ESP_LOGE(TAG, "Wrong %s formatting", bit_depth_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }
        }

    }
    cJSON_Delete(metadata_json);
    return ESP_OK;
}
