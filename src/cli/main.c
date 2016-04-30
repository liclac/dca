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

	AVFormatContext *ifctx;
	if ((err = avformat_open_input(&ifctx, config->infile, NULL, NULL)) < 0) {
		fprintf(stderr, "Couldn't open input: %s\n", get_av_err_str(err));
		return err;
	}
	if ((err = avformat_find_stream_info(ifctx, NULL)) < 0) {
		fprintf(stderr, "Couldn't find stream info: %s\n", get_av_err_str(err));
		avformat_close_input(&ifctx);
		return err;
	}

	AVCodec *icodec;
	int stream_id = av_find_best_stream(ifctx, AVMEDIA_TYPE_AUDIO, -1, -1, &icodec, 0);
	if (stream_id == AVERROR_STREAM_NOT_FOUND) {
		fprintf(stderr, "Couldn't find an audio stream\n");
		avformat_close_input(&ifctx);
		return AVERROR_STREAM_NOT_FOUND;
	}
	if (stream_id == AVERROR_DECODER_NOT_FOUND) {
		fprintf(stderr, "Can't find a decoder for any audio stream\n");
		avformat_close_input(&ifctx);
		return AVERROR_DECODER_NOT_FOUND;
	}
	AVStream *stream = ifctx->streams[stream_id];
	AVCodecContext *ictx = stream->codec;

	if ((err = avcodec_open2(ictx, icodec, NULL)) < 0) {
		fprintf(stderr, "Can't open codec: %s\n", get_av_err_str(err));
		avformat_close_input(&ifctx);
		return err;
	}

	AVPacket pkt;
	AVFrame *frame = av_frame_alloc();
	while (1) {
		if ((err = av_read_frame(ifctx, &pkt)) < 0) {
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
		if ((err = avcodec_decode_audio4(ictx, frame, &got_frame, &pkt)) < 0) {
			fprintf(stderr, "Couldn't decode audio: %s\n", get_av_err_str(err));
			av_packet_unref(&pkt);
			break;
		}

		av_packet_unref(&pkt);

		if (got_frame) {
			int data_size = av_samples_get_buffer_size(NULL, ictx->channels, frame->nb_samples, ictx->sample_fmt, 1);
			fwrite(frame->data[0], 1, data_size, stdout);
		}
	}

	av_frame_free(&frame);

	return 0;
}
