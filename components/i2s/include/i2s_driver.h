#ifndef I2S_DRIVER_H_
#define I2S_DRIVER_H_

#include <stdint.h>
#include "driver/i2s_types.h"

#define GRVCHP_OUT_BCLK 18
#define GRVCHP_OUT_WS 15
#define GRVCHP_OUT_DOUT 22
#define GRVCHP_OUT_DIN 23

i2s_chan_handle_t i2s_driver_init();

#endif
