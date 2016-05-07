/**
 * @file dca/defaults.h
 * Default values for dca_t fields.
 */
#ifndef DCA_DEFAULTS_H
#define DCA_DEFAULTS_H

#include <opus/opus.h>

/// Default audio bit rate
#define DCA_DEFAULT_BIT_RATE 64000

/// Default audio sample rate
#define DCA_DEFAULT_SAMPLE_RATE 48000

/// Default number of encoded channels
#define DCA_DEFAULT_CHANNELS 2

/// Default number of samples per frame
#define DCA_DEFAULT_FRAME_SIZE 960

/// Default OPUS mode/application
#define DCA_DEFAULT_OPUS_MODE OPUS_APPLICATION_AUDIO

#endif
