#include "config.h"
#include <dca/dca.h>
#include <libavformat/avformat.h>
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

/**
 * Makes an output context.
 *
 * @param  config  Program configuration
 * @param  out_ctx Populated with a context on success
 * @return         0 on success
 */
int make_output_context(config_t *config, AVCodecContext **out_ctx) {
	int err = 0;

	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
	if (!codec) {
		fprintf(stderr, "The s16le codec doesn't exist!?\n");
		return 1;
	}

	AVCodecContext *ctx = avcodec_alloc_context3(codec);
	ctx->sample_fmt = AV_SAMPLE_FMT_S16;
	ctx->bit_rate = config->bit_rate * 1000;
	ctx->sample_rate = config->sample_rate;
	ctx->channel_layout = av_get_default_channel_layout(config->channels);

	if ((err = avcodec_open2(ctx, codec, NULL)) < 0) {
		fprintf(stderr, "Couldn't open codec: %s\n", get_av_err_str(err));
		return 1;
	}

	*out_ctx = ctx;
	return 0;
}

int main(int argc, char **argv) {
	int err;

	config_t *config = malloc(sizeof(config_t));
	config_defaults(config);
	if ((err = parse_args(config, argc, argv) || config->infile == NULL)) {
		print_usage(argv[0]);
		return err;
	}

	avcodec_register_all();
	av_register_all();

	AVCodecContext *octx;
	if (make_output_context(config, &octx)) {
		return err;
	}

	AVFormatContext *fctx;
	if ((err = avformat_open_input(&fctx, config->infile, NULL, NULL)) < 0) {
		fprintf(stderr, "Couldn't open input: %s\n", get_av_err_str(err));
		return err;
	}
	if ((err = avformat_find_stream_info(fctx, NULL)) < 0) {
		fprintf(stderr, "Couldn't find stream info: %s\n", get_av_err_str(err));
		avformat_close_input(&fctx);
		return err;
	}

	AVCodec *codec;
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
	AVCodecContext *ctx = stream->codec;

	if ((err = avcodec_open2(ctx, codec, NULL)) < 0) {
		fprintf(stderr, "Can't open codec: %s\n", get_av_err_str(err));
		avformat_close_input(&fctx);
		return err;
	}

	AVPacket pkt;
	AVFrame *frame = av_frame_alloc();
	while (1) {
		if ((err = av_read_frame(fctx, &pkt)) < 0) {
			if (err != AVERROR_EOF) {
				fprintf(stderr, "%s\n", get_av_err_str(err));
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
			break;
		}

		av_packet_unref(&pkt);

		if (got_frame) {
			int data_size = av_samples_get_buffer_size(NULL, ctx->channels, frame->nb_samples, ctx->sample_fmt, 1);
			fwrite(frame->data[0], 1, data_size, stdout);

			// int got_pkt;
			// if ((err = avcodec_encode_audio2(octx, &pkt, frame, &got_pkt)) < 0) {
			// 	fprintf(stderr, "Couldn't encode audio: %s\n", get_av_err_str(err));
			// 	av_frame_unref(frame);
			// 	break;
			// }

			// if (got_pkt) {

			// }
		}
	}

	av_frame_free(&frame);
	avformat_close_input(&fctx);

	return 0;
}
