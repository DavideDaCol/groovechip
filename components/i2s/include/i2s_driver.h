#ifndef I2S_DRIVER_H_
#define I2S_DRIVER_H_

#include "driver/i2s_types.h"

//ports that are used for I2S communication
#define GRVCHP_OUT_BCLK 26
#define GRVCHP_OUT_WS 25
#define GRVCHP_OUT_DOUT 33

#define GRVCHP_SAMPLE_FREQ 16000

/*
@brief initializes the i2s driver
*/
i2s_chan_handle_t i2s_driver_init();

#endif
