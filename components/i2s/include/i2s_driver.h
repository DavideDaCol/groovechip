#ifndef I2S_DRIVER_H_
#define I2S_DRIVER_H_

#include <stdint.h>
#include "driver/i2s_types.h"

#define GRVCHP_OUT_BCLK 26
#define GRVCHP_OUT_WS 25
#define GRVCHP_OUT_DOUT 33

#define GRVCHP_SAMPLE_FREQ 16000

i2s_chan_handle_t i2s_driver_init();

#endif
