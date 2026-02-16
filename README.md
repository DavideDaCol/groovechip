# GrooveChip

GrooveChip is an ESP-32 based sampler, an electronic musical instrument that can play back portions of sound recordings (_samples_). 

Groovechip features:
- simultaneous playback of up to 8 samples (10 seconds per sample)
- high fidelity audio output via dedicated I2S peripherals
- 4 different playback modes: hold, oneshot, oneshot-loop, loop
- 2 lines I2C screen
- (WIP) custom sample playback via SD
- (WIP) custom-made effects pipeline
- (WIP) sensor-based effect parameters modification
- sample recording from previous samples

## Project structure

### ESP32 Pinout

[Here](https://docs.google.com/spreadsheets/d/18M2RtzVNmbK-rlVUsBhTf8DuOmzg080vyFPP_4lzk1Q/edit?usp=sharing) you can find the pinout map for the project, indicating which pin has to be used by which component.

### Software components


```
.
├── CMakeFiles
│   └── pkgRedirects
├── CMakeLists.txt
├── components
│   ├── adc1
│   ├── effects
│   ├── fsm
│   ├── i2s
│   ├── joystick
│   ├── lcd
│   ├── mixer
│   ├── pad_section
│   ├── playback_mode
│   ├── potentiometer
│   ├── recorder
│   ├── sd_reader
│   ├── spi
│   └── template
│       ├── CMakeLists.txt
│       ├── example.c
│       └── include
│           └── example.h
├── dependencies.lock
├── main
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── main.c
├── README.md
├── remux.sh
├── sdkconfig
└── sdkconfig.old
```


## Hardware requirements

1. ESP32-WROVER Development Board
2. I2S DAC with AUX output
3. SPI SD card reader
4. Joystick with integrated click button
5. I2C digital screen
6. Potentiometer

## Software requirements

1. ESP Integrated Development Framework
2. (Optional) Visual Studio Code
3. (Optional) ESP-IDF Extension for VS Code

## Build instruction

The project can either be built manually using ESP's tools directly or with the aid of the ESP-IDF extension for Visual Studio Code; we recommend the latter.

### Using Visual Studio Code

1. [Follow the instructions](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) for downloading both the ESP-IDF extension for Visual Studio Code and ESP-IDF itself; ESP-IDF is the actual framework needed to build the application and it can be easily installed directly from the extension;
2. Press `ctrl + shift + p` or `cmd + shift + p` to open the command palette;
3. Click on `ESP-IDF: Add VS Code Configuration Folder` to create the necessary bindings for the extension to work properly;
3. Click on `ESP-IDF: Build Your Project` to build the source code;
4. Click on `ESP-IDF: Select Port to Use` and choose the interface that is currently being used by the ESP32 
5. Click on `ESP-IDF: Build, Flash and Start a Monitor on Your Device` to directly upload the code to the ESP32 and view its logs

### Using ESP-IDF directly

For the process of setting up ESP-IDF manually, please refer to the official documentation as provided by Espressif:
- [MacOS / Linux](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-macos-setup.html)
- [Windows](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/windows-setup.html)

## User Guide

### Menu navigation:

```
.
├── general menu
│       ├── Settings
|       |       ├── Volume
|       |       └── Metronome
|       |               ├── On/Off
|       |               └── Bpm
│       └── Effects
|       |       ├── Bitcrusher
|       |       |       ├── On/Off
|       |       |       ├── Bit depth
|       |       |       └── Downsample
|       |       └── Distortion
|       |               ├── On/Off
|       |               ├── Gain
|       |               └── Threshold
└── button menu
        ├── Settings
        |       ├── Volume
        |       └── Mode
        ├── Effects
        |       ├── Bitcrusher
        |       |       ├── On/Off
        |       |       ├── Bit depth
        |       |       └── Downsample
        |       ├── Pitch
        |       └── Distortion
        |               ├── On/Off
        |               ├── Gain
        |               └── Threshold
        ├── Chopping
        |       ├── Start
        |       └── End
        ├── Sample load
        |       ├── Sample 1
        |       ├── Sample 2
        |       └── ...
        └── Save changes
```

### Menu navigation flow

**Joystick up/down**:
  - Move up and down in the menu

**Joystick left**:
  - Move to the previous menu

**Joystick right**:
  - Move to the selected menu

**Joystick button**:
  - Move to recording mode (see the flow below)

**Button**:
  - Move to the button menu if a sample is associated to that button
  - Move to the sample load menu
  - Select the button to record into if in record mode

**Potentiometer**:
  - Changes the value or toggle the option depending on the current menu


### Recording flow

Record button (start recording) -> Select button -> Record button/wait 5 sec (stop recording)

### Audio file normalization

In order for the files to be played correctly by the ESP32, they have to be formatted in a common format (WAV) with the same set of parameters.

If you're unsure whether the file you intend to upload meets these standars, we have supplied a simple FFmpeg script (`remux.sh`) that creates a copy of the original audio file with the correct parameters.

To use this script, it is necessary to install FFmpeg on your computer. This software is free and open source and you can find more info [here](https://www.ffmpeg.org/download.html)

The command can be used as follows:
```bash
#in the root folder of the project
./remux.sh relative/path/to/file.wav
```

If FFmpeg has been installed correctly, a copy of the original file with the `_clean` suffix will be created.

## Presentation

## Youtube Pitch Video

## The Team

- Tommaso Ascolani: (Add your contributions)
- Davide Da Col: I2S driver, audio mixer, sample playback
- Giovanni Sbalchiero: (Add your contributions)
- Marco Zanatta: (Add your contributions)
