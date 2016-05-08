#include "config.h"
#include <dca/dca.h>
#include <dca/encoder.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include <opus/opus.h>
#include <string.h>
#include <stdio.h>

/**
 * Returns a static error string for a libav error.
 */
const char* get_av_err_str(const int err) {
	static char buf[1024];
	if (av_strerror(err, buf, sizeof(buf))) {
		strcpy(buf, "[COULDN'T GET A PROPER ERROR]");
	}
	return buf;
}

/**
 * Prints a usage message to stderr.
 */
void print_usage(const char *pname) {
	fprintf(stderr, "Usage: %s [flags] input\n", pname);
	fprintf(stderr, "Flags:\n");
	fprintf(stderr, "    -aa (audio|voip|lowdelay) [default: audio]\n");
	fprintf(stderr, "        OPUS application\n");
	fprintf(stderr, "    -ab ... [default: 64]\n");
	fprintf(stderr, "        Audio bitrate, in kb/s\n");
	fprintf(stderr, "    -ac ... [default: 2]\n");
	fprintf(stderr, "        Audio channels\n");
	fprintf(stderr, "    -ar ... [default: 48000]\n");
	fprintf(stderr, "        Audio sample rate\n");
	fprintf(stderr, "    -as ... [default: 960]\n");
	fprintf(stderr, "        Frame size, can be 960 (20ms), 1920 (40ms) or 2880 (60ms)\n");
}

int main(int argc, char **argv) {
	int err;

	// Parse commandline flags! This should probably be made less home-cooked...
	config_t *config = malloc(sizeof(config_t));
	config_defaults(config);
	if ((err = parse_args(config, argc, argv) || config->infile == NULL)) {
		print_usage(argv[0]);
		return err;
	}

	// Load every imaginable container format and codec
	avcodec_register_all();
	av_register_all();

	// Open the input file, and just kinda... stare at it, to figure out what it is
	AVFormatContext *fctx = NULL;
	if ((err = avformat_open_input(&fctx, config->infile, NULL, NULL)) < 0) {
		fprintf(stderr, "Couldn't open input: %s\n", get_av_err_str(err));
		return err;
	}
	if ((err = avformat_find_stream_info(fctx, NULL)) < 0) {
		fprintf(stderr, "Couldn't find stream info: %s\n", get_av_err_str(err));
		avformat_close_input(&fctx);
		return err;
	}

	// Find an audio stream (we may have been given a video file) and the codec for good measure
	AVCodec *codec = NULL;
	int stream_id = av_find_best_stream(fctx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
	if (stream_id == AVERROR_STREAM_NOT_FOUND) {
		fprintf(stderr, "Couldn't find an audio stream\n");
		avformat_close_input(&fctx);
		return AVERROR_STREAM_NOT_FOUND;
	}
	if (stream_id == AVERROR_DECODER_NOT_FOUND) {
		fprintf(stderr, "Can't find a decoder for any audio stream\n");
		avformat_close_input(&fctx);
		return AVERROR_DECODER_NOT_FOUND;
	}
	AVStream *stream = fctx->streams[stream_id];

	// Grab a decoder for that codec, also we need to explicitly open it
	AVCodecContext *ctx = stream->codec;
	ctx->channel_layout = av_get_default_channel_layout(ctx->channels);
	if ((err = avcodec_open2(ctx, codec, NULL)) < 0) {
		fprintf(stderr, "Can't open codec: %s\n", get_av_err_str(err));
		avformat_close_input(&fctx);
		return err;
	}

	dca_t *dca = dca_new(0);
	dca->bit_rate = config->bit_rate;
	dca->sample_rate = config->sample_rate;
	dca->channels = config->channels;
	dca->frame_size = config->frame_size;
	dca->opus_mode = config->opus_mode;

	dca_encoder_t *enc = dca_encoder_new(dca, ctx->sample_fmt, ctx->sample_rate);

	// Fancypants encoding loop
	AVPacket pkt;
	AVFrame *frame = av_frame_alloc();
	while (1) {
		int stop = 0;
		while (dca_encoder_needs_more(enc)) {
			if ((err = av_read_frame(fctx, &pkt)) < 0) {
				if (err != AVERROR_EOF) {
					fprintf(stderr, "%s\n", get_av_err_str(err));
					stop = 1;
				}
				av_packet_unref(&pkt);
				break;
			}

			if (pkt.stream_index != stream_id) {
				av_packet_unref(&pkt);
				continue;
			}

			int got_frame;
			if ((err = avcodec_decode_audio4(ctx, frame, &got_frame, &pkt)) < 0) {
				fprintf(stderr, "Couldn't decode audio: %s\n", get_av_err_str(err));
				av_packet_unref(&pkt);
				stop = 1;
				break;
			}

			av_packet_unref(&pkt);

			if (got_frame) {
				if ((dca_encoder_feed_frame(enc, frame)) < 0) {
					fprintf(stderr, "Couldn't feed encoder: %s\n", get_av_err_str(err));
					stop = 1;
					av_frame_unref(frame);
					break;
				}

				av_frame_unref(frame);
			}
		}

		if (stop) {
			break;
		}

		int samples;
		int16_t len;
		unsigned char obuf[4000];
		if ((samples = dca_encoder_emit(enc, &len, obuf, sizeof(obuf))) < 0) {
			err = samples;
			fprintf(stderr, "Couldn't encode frame: %s\n", opus_strerror(err));
			break;
		}

		if (samples == 0) {
			break;
		}

		fprintf(stderr, "%d samples -> %d byte\n", samples, len);
		fwrite(&len, 1, sizeof(len), stdout);
		fwrite(obuf, 1, len, stdout);
	}

	av_frame_free(&frame);
	avformat_close_input(&fctx);

	return 0;
}
