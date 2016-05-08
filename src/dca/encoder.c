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
	enc->samples = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, dca->channels, dca->frame_size);
	enc->tmp_buf = NULL;
	enc->tmp_buf_size = 0;

	if ((err = swr_init(enc->swr)) < 0) {
		return NULL;
	}

	return enc;
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
