#include "config.h"
#include <dca/dca.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
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

	// Find an audio stream (we may have been given a video file) and the codec for good measure
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

	// Grab a decoder for that codec, also we need to explicitly open it
	AVCodecContext *ctx = stream->codec;
	if ((err = avcodec_open2(ctx, codec, NULL)) < 0) {
		fprintf(stderr, "Can't open codec: %s\n", get_av_err_str(err));
		avformat_close_input(&fctx);
		return err;
	}

	ctx->channel_layout = av_get_default_channel_layout(ctx->channels);

	// We want to spit out signed 16-bit little endian PCM (s16le), it's not actually possible for
	// this to not be available, unless someone was to remove av_register_all() above
	AVCodec *ocodec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
	if (!codec) {
		fprintf(stderr, "The s16le codec doesn't exist!?\n");
		return 1;
	}

	// Make an encoder context for this codec, and set it to the configured parameters
	AVCodecContext *octx = avcodec_alloc_context3(ocodec);
	octx->sample_fmt = AV_SAMPLE_FMT_S16;
	octx->bit_rate = config->bit_rate;
	octx->sample_rate = config->sample_rate;
	octx->channel_layout = av_get_default_channel_layout(config->channels);

	if ((err = avcodec_open2(octx, ocodec, NULL)) < 0) {
		fprintf(stderr, "Couldn't open codec: %s\n", get_av_err_str(err));
		return 1;
	}

	// The encoder doesn't actually resample anything, huh
	SwrContext *swr = swr_alloc_set_opts(NULL,
		octx->channel_layout, octx->sample_fmt, octx->sample_rate,
		ctx->channel_layout, ctx->sample_fmt, ctx->sample_rate,
		0, NULL
	);

	// Also make an Opus encoder
	OpusEncoder *opus = malloc(opus_encoder_get_size(octx->channels));
	if ((err = opus_encoder_init(opus, octx->sample_rate, octx->channels, config->opus_mode)) != OPUS_OK) {
		fprintf(stderr, "Couldn't init OPUS: %s\n", opus_strerror(err));
		return err;
	}

	// Fancypants encoding loop
	AVPacket pkt;
	AVFrame *frame = av_frame_alloc();
	AVFrame *oframe = av_frame_alloc();
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
			oframe->channels = octx->channels;
			oframe->channel_layout = octx->channel_layout;
			oframe->sample_rate = octx->sample_rate;
			oframe->format = octx->sample_fmt;

			if ((err = swr_convert_frame(swr, oframe, frame)) < 0) {
				fprintf(stderr, "Couldn't resample audio: %s\n", get_av_err_str(err));
				av_frame_unref(frame);
				break;
			}

			av_frame_unref(frame);

			int got_pkt;
			if ((err = avcodec_encode_audio2(octx, &pkt, oframe, &got_pkt)) < 0) {
				fprintf(stderr, "Couldn't encode audio: %s\n", get_av_err_str(err));
				av_frame_unref(oframe);
				break;
			}

			av_frame_unref(oframe);

			if (got_pkt) {
				// unsigned char data[1024*1024];
				// int16_t len;
				// if ((len = opus_encode(opus, (opus_int16*)(pkt.data), config->frame_size, data, sizeof(data))) < 0) {
				// 	fprintf(stderr, "Couldn't encode OPUS: %s\n", opus_strerror(len));
				// 	av_packet_unref(&pkt);
				// 	break;
				// }

				// fwrite(&len, 1, sizeof(len), stdout);
				// fwrite(data, 1, len, stdout);

				fwrite(pkt.data, 1, pkt.size, stdout);

				av_packet_unref(&pkt);
			}
		}
	}

	av_frame_free(&frame);
	avformat_close_input(&fctx);

	return 0;
}
