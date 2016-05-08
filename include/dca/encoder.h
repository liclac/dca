#ifndef DCA_ENCODER_H
#define DCA_ENCODER_H

#include <dca/dca.h>
#include <opus/opus.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>

/**
 * A DCA encoder.
 */
typedef struct {
	dca_t *dca;							///< DCA metadata.
	enum AVSampleFormat in_sample_fmt;	///< Input sample format
	int in_sample_rate;					///< Input sample rate

	SwrContext *swr;					///< Resampler.
	OpusEncoder *opus;					///< OPUS Encoder.
	AVAudioFifo *samples;				///< FIFO sample buffer.

	void *tmp_buf;						///< Temporary buffer.
	size_t tmp_buf_size;				///< Temporary buffer size.
} dca_encoder_t;

/**
 * Creates a new DCA encoder.
 *
 * The caller takes ownership of it, and must free it using dca_encoder_free().
 *
 * @param  dca DCA structure for metadata.
 * @return     A newly allocated encoder.
 */
dca_encoder_t* dca_encoder_new(dca_t *dca, enum AVSampleFormat in_sample_fmt, int in_sample_rate);

/**
 * Frees a DCA encoder.
 *
 * @param enc DCA encoder to free.
 */
void dca_encoder_free(dca_encoder_t *enc);

/**
 * Check if more samples are needed.
 *
 * @param  enc Encoder.
 * @return     Samples needed for a frame, or 0.
 */
int dca_encoder_needs_more(dca_encoder_t *enc);

/**
 * Feeds the encoder samples.
 *
 * The number of bytes read is `av_get_bytes_per_sample(in_sample_fmt) * dca->channels * count`.
 *
 * @param  enc     Encoder.
 * @param  samples Pointer to a sample buffer.
 * @param  count   Number of samples to read.
 * @return         < 0 on error.
 */
int dca_encoder_feed(dca_encoder_t *enc, void *samples, int count);

/**
 * Feeds the encoder an AVFrame.
 *
 * This is a wrapper around dca_encoder_feed() that passes in data from the frame.
 *
 * @param  enc   Encoder.
 * @param  frame Source frame.
 * @return       < 0 on error.
 */
int dca_encoder_feed_frame(dca_encoder_t *enc, AVFrame *frame);

/**
 * Emits an OPUS frame.
 *
 * @param  enc     Encoder.
 * @param  len     Set to the length of the OPUS frame.
 * @param  buf     Output buffer.
 * @param  buf_len Size of the output buffer.
 * @return         Number of samples consumed, < 0 on error.
 */
int dca_encoder_emit(dca_encoder_t *enc, int16_t *len, void *buf, size_t buf_len);

/**
 * Reserves internal buffer space.
 *
 * After this call, enc->tmp_buf is guaranteed to be a valid buffer with space for at least
 * count samples. Existing data in the buffer is NOT guaranteed to be preserved.
 *
 * @param enc   Encoder.
 * @param count Samples to reserve space for.
 */
void dca_encoder_reserve_samples(dca_encoder_t *enc, int count);

#endif
