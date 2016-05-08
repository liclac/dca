#include <dca/source.h>

dca_source_t* dca_source_new(dca_t *dca) {
	dca_source_t *src = malloc(sizeof(dca_source_t));
	src->dca = dca;
	src->fctx = NULL;
	src->ctx = NULL;
	src->stream_id = -1;

	return src;
}

int dca_source_open(dca_source_t *src, const char *filename) {
	int err;

	if ((err = avformat_open_input(&(src->fctx), filename, NULL, NULL)) < 0) {
		return err;
	}

	if ((err = avformat_find_stream_info(src->fctx, NULL)) < 0) {
		avformat_close_input(&(src->fctx));
		return err;
	}

	AVCodec *codec = NULL;
	if ((src->stream_id = av_find_best_stream(src->fctx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0)) < 0) {
		return src->stream_id;
	}

	AVStream *stream = src->fctx->streams[src->stream_id];
	src->ctx = stream->codec;
	src->ctx->channel_layout = av_get_default_channel_layout(src->ctx->channels);
	if ((err = avcodec_open2(src->ctx, codec, NULL)) < 0) {
		avformat_close_input(&(src->fctx));
		return err;
	}

	return 0;
}

void dca_source_free(dca_source_t *src) {
	avcodec_close(src->ctx);
	avformat_close_input(&(src->fctx));
	free(src);
}

int dca_source_read_frame(dca_source_t *src, AVFrame *out_frame) {
	int err;
	AVPacket pkt;
	int got_frame = 0;
	do {
		do {
			if ((err = av_read_frame(src->fctx, &pkt)) < 0) {
				return err;
			}
		} while (pkt.stream_index != src->stream_id);

		if ((err = avcodec_decode_audio4(src->ctx, out_frame, &got_frame, &pkt)) < 0) {
			return err;
		}
	} while(!got_frame);

	return 0;
}
