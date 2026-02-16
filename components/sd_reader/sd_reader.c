#include "include/sd_reader.h"
#include <cJSON.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <cJSON.h>
#include <dirent.h>
#include "spi_driver.h" 
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "diskio_sdmmc.h"
#include "mixer.h"
#include "effects.h"
#include "esp_psram.h"
#include <sys/stat.h>
#include "nvs.h"
#include "nvs_flash.h"

const char* TAG = "SdReader";

char** sample_names = NULL;
int sample_names_size = 0;

char *sample_names_bank[SAMPLE_NUM];

static sdmmc_card_t sd_card;

int record_number;

/*
@brief initializes the SD filesystem, checking if there are any new downloaded samples. It includes name's truncation and JSON creation
*/
static esp_err_t sd_fs_init();

/*
@brief creates the JSON file according to the sample's infos. The JSON content follows the parameter that are passed
@param filename path of the JSON that will be generated. It corresponds to GRVCHP_MNTPOINT/JSON_FILES_DIR/<sample_name>.json
@param bitcrusher_enabled sample's bitcrusher state
@param downsample sample's downsample
@param bit_depth sample's bit depth
@param pitch_factor sample's pitch factor
@param distortion_enabled sample's distortion state
@param threshold sample's threshold
@param gain sample's gain
@param start_ptr sample's playback start pointer
@param end_ptr sample's playback end pointer*/
static esp_err_t set_json(char* filename, bool bitcrusher_enabled, uint8_t downsample, uint8_t bit_depth, float pitch_factor, bool distortion_enabled, uint16_t threshold, float gain, float start_ptr, uint32_t end_ptr);

/*
@brief extracts the informations about the sample from the JSON file.
@param filename path of the JSON that will be analyzed. It corresponds to GRVCHP_MNTPOINT/JSON_FILES_DIR/<sample_name>.json, and containes the informations related to <sample_name>
@param bitcrusher_enabled sample's bitcrusher state
@param downsample sample's downsample
@param bit_depth sample's bit depth
@param pitch_factor sample's pitch factor
@param distortion_enabled sample's distortion state
@param threshold sample's threshold
@param gain sample's gain
@param start_ptr sample's playback start pointer
@param end_ptr sample's playback end pointer
*/
static esp_err_t get_json(char *filename, bool* bitcrusher_enabled, uint8_t* downsample, uint8_t* bit_depth, float* pitch_factor, bool* distortion_enabled, uint16_t* threshold, float* gain, float* start_ptr, uint32_t* end_ptr);

/*
@brief truncates the original name to MAX_SIZE characters (max characters accepted from the screen) and eventually renames the file 
@param wav_path path that leads to the directory containing all the wav files
@param original_name current file's name
@param out_clean_name result of the file's name truncation. It can remain the same or be truncated to MAX_SIZE characters
*/
static bool normalize_wav_file(const char* wav_path, const char* original_name, char* out_clean_name);

/*
@brief checks whether the JSON associated to a sample exists or not. If not, it creates one with default values
@param wav_path path to the folder containing the WAV files
@param json_path path to the folder containing the JSON files 
@param clean_name name of the sample we want to create the JSON of
*/
static esp_err_t ensure_json_exists(const char* wav_path, const char* json_path, const char* clean_name);

/*
@brief generates an array with all the samples' names
@param dir pointer to the directory containing all the WAV files
@param count array's size
*/
static esp_err_t populate_sample_names_array(DIR *dir, int count);

/*
@brief generates a new name and adds it to the sample names' list
@param out_sample_name pointer to the newly generated string
*/
static void add_new_rec_to_sample_names(char **out_sample_name);

/*
@brief extracts the record number from the flash memory
*/
static esp_err_t rec_num_init();

esp_err_t sd_reader_init() {
    esp_err_t res;
    
    res = sdspi_driver_init(&sd_card);
    if (res != ESP_OK)
        return res;


    // defining the mountpoint configuration
    esp_vfs_fat_conf_t sd_mntpoint_conf = {
        .base_path = GRVCHP_MNTPOINT,
        .fat_drive = GRVCHP_FAT_DRIVE_STR,
        .max_files = GRVCHP_MAX_FILES
    };

    FATFS* fat_fs;

    // connecting the POSIX and C standard library IO function with FATFS.
    res = esp_vfs_fat_register_cfg(&sd_mntpoint_conf, &fat_fs);
    if (res != ESP_OK){
        ESP_ERROR_CHECK_WITHOUT_ABORT(res);
        return res;
    }
    
    // SD diskio driver registration
    // to provide function pointer and define how the driver has to act with the SD card
    ff_diskio_register_sdmmc(GRVCHP_FAT_DRIVE_INDEX, &sd_card);

    // registering the filesystem in the SD card to the FatFs module
    FRESULT fat_res = f_mount(fat_fs, GRVCHP_FAT_DRIVE_STR, 1); 
    if (fat_res != FR_OK) {
        fprintf(stderr, "Error in mounting the SD card filesystem\n");
        return ESP_FAIL;
    }

    if (rec_num_init() != ESP_OK) return ESP_ERR_NO_MEM;

    ESP_ERROR_CHECK_WITHOUT_ABORT(sd_fs_init());

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
    // defining the file path to read from
    char file_path[MAX_BUFF_SIZE];
    snprintf(file_path, sizeof(file_path),"%s/%s/%s.wav", GRVCHP_MNTPOINT, WAV_FILES_DIR, sample_name);
    ESP_LOGI(TAG, "requested file path is %s", file_path);
    
    // opening the file in read mode
    FILE* fp = fopen(file_path, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG,"Sample not found");
        return ESP_ERR_NOT_FOUND;
    }
    
    printf("Available size: %d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    // allocating in the PSRAM the section of memory for the sample infos
    // *out_sample_ptr = heap_caps_malloc(sizeof(sample_t), MALLOC_CAP_SPIRAM);
    *out_sample_ptr = heap_caps_malloc(sizeof(sample_t), MALLOC_CAP_SPIRAM);
    sample_t* out_sample = *out_sample_ptr;
    if (out_sample_ptr == NULL) {
        ESP_LOGE(TAG, "Error in allocating the sample");
        return ESP_ERR_NO_MEM;
    }

    // assigning the file's content to the pointer out_sample_ptr
    size_t read_cnt = fread(&(out_sample -> header), sizeof(wav_header_t), 1, fp);

    // if the read doesn't succeed
    if (read_cnt != 1) {
        ESP_LOGE(TAG, "Error while reading the file\n");

        free(out_sample);
        out_sample_ptr = NULL;
        return ESP_FAIL;
    }

    
    // allocating the section of memory for the actual sample (wav buffer)
    // out_sample -> raw_data = heap_caps_malloc((out_sample -> header).data_size, MALLOC_CAP_SPIRAM);
    out_sample->raw_data = heap_caps_malloc(out_sample -> header.data_size, MALLOC_CAP_SPIRAM);
    if (out_sample->raw_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate SPIRAM for WAV data");
        heap_caps_free(out_sample);
        *out_sample_ptr = NULL;
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }
    
    // reading the area of the file where the data is located
    read_cnt = fread(out_sample -> raw_data, 1, (out_sample -> header).data_size, fp);

    
    if (read_cnt != (out_sample -> header).data_size) {
        ESP_LOGE(TAG, "Error while reading the file\n");
        ESP_LOGE(TAG, "Read data: %d\n", read_cnt);
        
        heap_caps_free(out_sample -> raw_data);
        heap_caps_free(out_sample);
        return ESP_FAIL;
    }
    
    // getting the json filename
    char filename[MAX_BUFF_SIZE];
    snprintf(filename, sizeof(filename), "%s/%s/%s.json", GRVCHP_MNTPOINT, JSON_FILES_DIR, sample_name);
    
    // closing the file
    fclose(fp);

    // declaring some useful variables for the json parsing
    bool bitcrusher_enabled;
    bool distortion_enabled;
    uint8_t downsample;
    uint8_t bit_depth;
    float gain;
    uint16_t threshold;
    float pitch_factor;

    out_sample -> bank_index = in_bank_index;

    // json parsing 
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

    // setting default values
    out_sample -> volume = 0.1f;
    out_sample -> playback_ptr = out_sample -> start_ptr;
    out_sample -> total_frames = (out_sample -> header).data_size / 2; 

    // assigning bitcrusher values according to the infos in the json file
    set_bit_crusher(in_bank_index, bitcrusher_enabled);
    set_bit_crusher_bit_depth(in_bank_index, bit_depth);
    set_bit_crusher_downsample(in_bank_index, downsample);

    // same for distortion
    set_distortion(in_bank_index, distortion_enabled);
    set_distortion_gain(in_bank_index, gain);
    set_distortion_threshold(in_bank_index, threshold);

    // same for the pitch
    set_pitch_factor(in_bank_index, pitch_factor);

    return ESP_OK;
}

esp_err_t st_sample(int in_bank_index, char *sample_name) {
    if (sample_name == NULL) {
        add_new_rec_to_sample_names(&sample_name);
    }
    sample_t* curr_sample = sample_bank[in_bank_index];

    // defining the sample's file name, based on the id 
    char wav_file_path[MAX_BUFF_SIZE];
    snprintf(wav_file_path, sizeof(wav_file_path), "%s/%s/%s.wav", GRVCHP_MNTPOINT, WAV_FILES_DIR, sample_name);

    char json_file_path[MAX_BUFF_SIZE];
    snprintf(json_file_path, sizeof(json_file_path), "%s/%s/%s.json", GRVCHP_MNTPOINT, JSON_FILES_DIR, sample_name);

    printf("%s, %s\n", wav_file_path, json_file_path);

    FILE* wav_fp;
    // opening the file in write-or-create mode
    // if there's no file containing that sample, it will create one 
    if ((wav_fp = fopen(wav_file_path, "wb")) == NULL) {
        ESP_LOGE(TAG, "Error in creating the new wav file");
        return ESP_FAIL;
    } 

    wav_header_t local_header = curr_sample -> header;
    // writing the sample inside the file
    size_t write_cnt = fwrite(&local_header, sizeof(wav_header_t), 1, wav_fp);
    if (write_cnt != 1) {
        ESP_LOGE(TAG, "Error while writing the wav file's header");
        fclose(wav_fp);
        return ESP_FAIL;
    }

    size_t bytes_remaining = curr_sample->header.data_size;
    uint8_t* data_ptr = curr_sample->raw_data;
    size_t chunk_size = 4096; // 4KB is optimal for FATFS and internal RAM bouncing

    while (bytes_remaining > 0) {
        size_t bytes_to_write = (bytes_remaining > chunk_size) ? chunk_size : bytes_remaining;
        
        // write the chunk
        size_t written = fwrite(data_ptr, 1, bytes_to_write, wav_fp);
        if (written != bytes_to_write) {
            ESP_LOGE(TAG, "Error while writing the wav file data");
            fclose(wav_fp);
            return ESP_FAIL;
        }
        
        // move the pointer forward
        data_ptr += bytes_to_write;
        bytes_remaining -= bytes_to_write;
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
    int valid_wav_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char clean_name[MAX_SIZE];
        
        if (!normalize_wav_file(wav_path, entry->d_name, clean_name)) {
            continue; // skip if it's not a WAV or rename failed
        }

        if (ensure_json_exists(wav_path, json_path, clean_name) == ESP_OK) {
            valid_wav_count++; 
        }
    }

    esp_err_t res = populate_sample_names_array(dir, valid_wav_count);
    
    closedir(dir);
    return res;
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

    // strings to add in the JSON file
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

    // JSON setup, according to the structure defined above
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

    // conversion of the JSON object into a string to copy into the file
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
    
    // fields in the JSON file
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

    // opening the json file
    FILE* fp = fopen(filename, "r");

    // reading the content and store it in a buffer
    char buffer[MAX_JSON_SIZE];
    if (fread(buffer, 1, sizeof(buffer), fp) == 0) {
        fclose(fp);
        return ESP_FAIL;  
    } 
    
    fclose(fp);

    printf("%s\n", buffer);

    // parsing the document
    cJSON* metadata_json = cJSON_Parse(buffer);
    if (metadata_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "%s", error_ptr);
        }
        cJSON_Delete(metadata_json);
        return ESP_FAIL;
    }

    // getting start_ptr
    cJSON* start_ptr_json = cJSON_GetObjectItemCaseSensitive(metadata_json, start_ptr_str);
    // type checking
    if (cJSON_IsNumber(start_ptr_json)) {
        *start_ptr = (float) start_ptr_json -> valuedouble;
    } else {
        ESP_LOGE(TAG, "Wrong %s formatting", start_ptr_str);
        cJSON_Delete(metadata_json);
        return ESP_FAIL;
    }

    // getting end_ptr
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
        
        // effects -> pitch parsing
        cJSON* pitch_json = cJSON_GetObjectItemCaseSensitive(effects_json, pitch_str);
        if (cJSON_IsObject(pitch_json)) {

            // getting the effects -> pitch -> pitch factor
            cJSON *pitch_factor_json = cJSON_GetObjectItemCaseSensitive(pitch_json, pitch_factor_str);
            // type checking
            if (cJSON_IsNumber(pitch_factor_json)) {
                *pitch_factor = (float) pitch_factor_json -> valuedouble;
            } else {
                ESP_LOGE(TAG, "Wrong %s formatting", pitch_factor_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }
        }

        // effects -> distortion parsing
        cJSON *distortion_json = cJSON_GetObjectItemCaseSensitive(effects_json, distortion_str);
        if (cJSON_IsObject(distortion_json)) {

            // getting the effects -> distortion -> enable value
            cJSON *enabled_json = cJSON_GetObjectItemCaseSensitive(distortion_json, enabled_str);
            // type checking
            if (cJSON_IsBool(enabled_json)) {
                *distortion_enabled = (bool) cJSON_IsTrue(enabled_json);
            } else {
                ESP_LOGE(TAG, "Wrong distortion %s formatting", enabled_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }

            // getting the effects -> distortion -> gain
            cJSON *gain_json = cJSON_GetObjectItemCaseSensitive(distortion_json, gain_str);
            // type checking
            if (cJSON_IsNumber(gain_json)) {
                *gain = (float) gain_json -> valuedouble;
            } else {
                ESP_LOGE(TAG, "Wrong %s formatting", gain_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }

            // getting the effects -> distortion -> threshold
            cJSON *threshold_json = cJSON_GetObjectItemCaseSensitive(distortion_json, threshold_str);
            // type checking
            if (cJSON_IsNumber(threshold_json)) {
                *threshold = (float) threshold_json -> valuedouble;
            } else {
                ESP_LOGE(TAG, "Wrong %s formatting", threshold_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }
        }

        // effects -> bitcrusher parsing
        cJSON *bitcrusher_json = cJSON_GetObjectItemCaseSensitive(effects_json, bitcrusher_str);
        if (cJSON_IsObject(bitcrusher_json)) {

            // getting the effects -> bitcrusher -> enable value
            cJSON *enabled_json = cJSON_GetObjectItemCaseSensitive(bitcrusher_json, enabled_str);
            // type checking
            if (cJSON_IsBool(enabled_json)) {
                *bitcrusher_enabled = cJSON_IsTrue(enabled_json);
            } else {
                ESP_LOGE(TAG, "Wrong bitcrusher %s formatting", enabled_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }

            // getting the effects -> bitcrusher -> downsample
            cJSON *downsample_json = cJSON_GetObjectItemCaseSensitive(bitcrusher_json, downsample_str);
            // type checking
            if (cJSON_IsNumber(downsample_json)) {
                *downsample = (uint8_t) downsample_json -> valuedouble;
            } else {
                ESP_LOGE(TAG, "Wrong %s formatting", downsample_str);
                cJSON_Delete(metadata_json);
                return ESP_FAIL;
            }

            // getting the effects -> bitcrusher -> bit depth
            cJSON *bit_depth_json = cJSON_GetObjectItemCaseSensitive(bitcrusher_json, bit_depth_str);
            // type checking
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

static bool normalize_wav_file(const char* wav_path, const char* original_name, char* out_clean_name) {
    char *ext = strrchr(original_name, '.');
    if (!ext || (strcasecmp(ext, ".wav") != 0)) {
        ESP_LOGW(TAG, "Analyzed file %s is not a WAV file, ignoring", original_name);
        return false;
    }

    // extracting the base name (remove the .wav extension)
    snprintf(out_clean_name, MAX_SIZE, "%.*s", (int)(ext - original_name), original_name);

    // if the file name requires renaming (e.g., standardizing the extension string)
    char full_old_path[MAX_BUFF_SIZE];
    char full_new_path[MAX_BUFF_SIZE];
    snprintf(full_old_path, sizeof(full_old_path), "%s/%s", wav_path, original_name);
    snprintf(full_new_path, sizeof(full_new_path), "%s/%s.wav", wav_path, out_clean_name);
    
    char check_name[MAX_SIZE + WAV_EXTENSION_SIZE];
    snprintf(check_name, sizeof(check_name), "%s.wav", out_clean_name);
    
    if (strcmp(original_name, check_name) != 0) {
        if (rename(full_old_path, full_new_path) != 0) {
            ESP_LOGE(TAG, "Rename failed for: %s", original_name);
            return false; 
        }
    }
    return true;
}

static esp_err_t ensure_json_exists(const char* wav_path, const char* json_path, const char* clean_name) {
    char full_wav_path[MAX_BUFF_SIZE];
    char full_json_path[MAX_BUFF_SIZE];
    
    snprintf(full_wav_path, sizeof(full_wav_path), "%s/%s.wav", wav_path, clean_name);
    snprintf(full_json_path, sizeof(full_json_path), "%s/%s.json", json_path, clean_name);

    // check if JSON already exists
    FILE* check_fp = fopen(full_json_path, "r");
    if (check_fp != NULL) {
        fclose(check_fp);
        return ESP_OK; // JSON exists, nothing to do
    }

    // JSON is missing: read WAV header to determine data size
    wav_header_t header;
    FILE* fp = fopen(full_wav_path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Error opening WAV file %s: %s", full_wav_path, strerror(errno));
        return ESP_FAIL;
    }
    fread(&header, sizeof(wav_header_t), 1, fp);
    fclose(fp);

    // generate the default JSON file
    esp_err_t res = set_json(full_json_path, false, 1, BIT_DEPTH_MAX, 1.0, 
                             false, DISTORTION_THRESHOLD_MAX, DISTORTION_GAIN_MAX, 0, header.data_size);
    
    if (res == ESP_OK) {
        ESP_LOGI(TAG, "Created JSON file: %s", full_json_path);
    }
    return res;
}

static esp_err_t populate_sample_names_array(DIR *dir, int count) {
    if (count == 0) {
        sample_names = NULL;
        sample_names_size = 0;
        return ESP_OK;
    }

    sample_names = heap_caps_malloc(count * sizeof(char*), MALLOC_CAP_SPIRAM);
    if (!sample_names) {
        return ESP_ERR_NO_MEM;
    }
    
    sample_names_size = count;

    // get back to the beginning of the directory
    rewinddir(dir);
    struct dirent* entry;
    int idx = 0;

    while ((entry = readdir(dir)) != NULL && idx < count) {
        if (entry->d_name[0] == '.') continue;
        
        char *ext = strrchr(entry->d_name, '.');
        if (ext && strcasecmp(ext, ".wav") == 0) {
            sample_names[idx] = malloc(MAX_SIZE);
            if (sample_names[idx]) {
                snprintf(sample_names[idx], MAX_SIZE, "%.*s", (int)(ext - entry->d_name), entry->d_name);
                idx++;
            }
        }
    }
    return ESP_OK;
}

static void add_new_rec_to_sample_names(char **out_sample_name) {
    // sets sample name
    *out_sample_name = heap_caps_calloc(1, MAX_SIZE, MALLOC_CAP_SPIRAM);
    sprintf(*out_sample_name, "new_rec%d", record_number);

    record_number++;
    nvs_handle_t my_handle;
    if (nvs_open(STRG_NAMESPACE, NVS_READWRITE, &my_handle) == ESP_OK) {
        
        nvs_set_i32(my_handle, REC_NUM_ID, (int32_t)record_number);
        nvs_commit(my_handle); // This actually writes it to the physical flash chip
        nvs_close(my_handle);
        
        ESP_LOGI(TAG, "Saved new record_number (%d) to flash", record_number);
    }

    sample_names = heap_caps_realloc(sample_names, (++sample_names_size)*sizeof(char*), MALLOC_CAP_SPIRAM);
    sample_names[sample_names_size - 1] = *sample_names;

}

static esp_err_t rec_num_init() {
    nvs_handle_t rec_handler;
    if (nvs_open(STRG_NAMESPACE, NVS_READWRITE, &rec_handler) != ESP_OK) {
        ESP_LOGE(TAG, "Flash not available");
        return ESP_ERR_NO_MEM;
    }

    int32_t saved_num;
    if (nvs_get_i32(rec_handler, REC_NUM_ID, &saved_num) == ESP_OK) {
        record_number = saved_num;
        ESP_LOGI(TAG, "Loaded record_number from flash: %d", record_number);
    } else {
        ESP_LOGI(TAG, "No record_number found in flash. Starting at 0.");
    }
    nvs_close(rec_handler);
    return ESP_OK;
}