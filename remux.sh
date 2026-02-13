#!/bin/sh
# small bash script to convert WAV files in
# the correct format to be used on the sampler.
# 16khz sampling rate, no LIST or INFO metadata,
# 2 channel stereo and 16 bits per sample.

# usage: remux.sh /path/to/sample.wav

filename=$1

ffmpeg -i $1 -map_metadata -1 -write_id3v2 0 -fflags +bitexact -flags +bitexact -ar 16000 -ac 2 -sample_fmt s16 -f wav -rf64 never ./$(basename $filename .wav)_clean.wav
