/*********************************************************************************
 *                                   SD READER                                   *
 *        This module implements a driver to communicate with a SD card.         *
 *           It has been realized only to interact with SPI SD cards.            *
 *     The SD card is supposed to already be formatted using a FAT standard,     *
 *       that can be access using the file path "/sdcard" as a mountpoint.       *
 * Every sample is named based on the id, with extension "smp" (<bank_index>.smp) *
 *********************************************************************************/
#ifndef SD_READER_H
#define SD_READER_H

#include "mixer.h"

extern char **sample_names;
extern int sample_names_size;


//Default mountpoint
#define GRVCHP_MNTPOINT "/sdcard"
#define GRVCHP_MNTPOINT_SIZE 7

//FAT drive index (0 by default)
#define GRVCHP_FAT_DRIVE_INDEX 0
#define GRVCHP_FAT_DRIVE_STR "0:"

//Maximum number of files that can be opened simultaneously
#define GRVCHP_MAX_FILES 1

//Maximum size of the name that will be printed in the screen 
#define MAX_SIZE 17

//Directory containing all the samples' WAV files
#define WAV_FILES_DIR "wav_files"
#define WAV_FILES_DIR_SIZE 10
#define WAV_EXTENSION_SIZE 4

//Directory containing all the samples' JSON files
#define JSON_FILES_DIR "json_files"
#define JSON_FILES_DIR_SIZE 11
#define JSON_EXTENSION_SIZE 5

#define MAX_BUFF_SIZE 256
#define MAX_JSON_SIZE 512

#define WAV_EXTENSION_SIZE 4
#define JSON_EXTENSION_SIZE 5

#define FORMAT(S) "%" #S "[^.]"
#define RESOLVE(S) FORMAT(S)


/*
@brief initializes SD reader's driver and internal filesystem
*/
esp_err_t sd_reader_init();

/*
@brief transfers a sample from the SD card into the external SPI RAM
@param bank_index index that refers to sample_bank. Indicates where the sample will be located
@param sample_name name of the sample we want to transfer in RAM
@param out_sample_ptr pointer to the sample that has just been transferred*/
esp_err_t ld_sample(int in_bank_index, char* sample_name, sample_t** out_sample_ptr);

/*
@brief copies the sample's info into memory. It generates the sample's WAV file if new
@param index that refers to sample_bank. Indicates where the sample is located
@param sample_name name of the sample
*/
esp_err_t st_sample(int in_bank_index, char *sample_name);

#endif