#include <dca/encoder.h>
#include <libavutil/channel_layout.h>
#include <stdlib.h>

dca_encoder_t* dca_encoder_new(dca_t *dca, enum AVSampleFormat in_sample_fmt, int in_sample_rate) {
	int err;

	dca_encoder_t *enc = malloc(sizeof(dca_encoder_t));
	enc->dca = dca;
	enc->in_sample_fmt = in_sample_fmt;
	enc->in_sample_rate = in_sample_rate;
	enc->swr = swr_alloc_set_opts(NULL,
		av_get_default_channel_layout(dca->channels), AV_SAMPLE_FMT_S16, dca->sample_rate,
		av_get_default_channel_layout(dca->channels), in_sample_fmt, in_sample_rate,
		0, NULL
	);
	enc->opus = malloc(opus_encoder_get_size(dca->channels));
	enc->samples = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, dca->channels, dca->frame_size);
	enc->tmp_buf = NULL;
	enc->tmp_buf_size = 0;

	if ((err = swr_init(enc->swr)) < 0) {
		return NULL;
	}

	if ((err = opus_encoder_init(enc->opus, dca->sample_rate, dca->channels, dca->opus_mode)) != OPUS_OK) {
		return NULL;
	}
	opus_encoder_ctl(enc->opus, OPUS_SET_BITRATE(dca->bit_rate));

	return enc;
}

dca_encoder_t *dca_encoder_new_source(dca_t *dca, dca_source_t *src) {
	return dca_encoder_new(dca, src->ctx->sample_fmt, src->ctx->sample_rate);
}

void dca_encoder_free(dca_encoder_t *enc) {
	if (enc->tmp_buf != NULL) {
		free(enc->tmp_buf);
	}
	free(enc);
}

int dca_encoder_needs_more(dca_encoder_t *enc) {
	return av_audio_fifo_size(enc->samples) < enc->dca->frame_size;
}

int dca_encoder_feed(dca_encoder_t *enc, void *samples, int count) {
	int err;

	int expected = swr_get_out_samples(enc->swr, count);
	dca_encoder_reserve_samples(enc, expected);

	int converted;
	if ((converted = swr_convert(enc->swr, (uint8_t**)&(enc->tmp_buf), expected, (const uint8_t**)&samples, count)) < 0) {
		return converted;
	}

	if ((err = av_audio_fifo_write(enc->samples, &(enc->tmp_buf), converted)) < 0) {
		return err;
	}

	return converted;
}

int dca_encoder_feed_frame(dca_encoder_t *enc, AVFrame *frame) {
	return dca_encoder_feed(enc, frame->extended_data[0], frame->nb_samples);
}

int dca_encoder_emit(dca_encoder_t *enc, int16_t *len, void *buf, size_t buf_len) {
	if (av_audio_fifo_size(enc->samples) == 0) {
		*len = 0;
		return 0;
	}

	dca_encoder_reserve_samples(enc, enc->dca->frame_size);

	int samples;
	if ((samples = av_audio_fifo_read(enc->samples, &enc->tmp_buf, enc->dca->frame_size)) < 0) {
		return samples;
	}

	if ((*len = opus_encode(enc->opus, (opus_int16*)enc->tmp_buf, enc->dca->frame_size, buf, buf_len)) < 0) {
		return *len;
	}

	return samples;
}

void dca_encoder_reserve_samples(dca_encoder_t *enc, int count) {
	size_t new_size = sizeof(int16_t) * enc->dca->channels * count;
	if (new_size > enc->tmp_buf_size) {
		if (enc->tmp_buf != NULL) {
			free(enc->tmp_buf);
		}
		enc->tmp_buf = malloc(new_size);
		enc->tmp_buf_size = new_size;
	}
}
