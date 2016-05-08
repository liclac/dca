#include "config.h"
#include <dca/dca.h>
#include <dca/encoder.h>
#include <dca/source.h>
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

	dca_t *dca = dca_new(0);
	dca->bit_rate = config->bit_rate;
	dca->sample_rate = config->sample_rate;
	dca->channels = config->channels;
	dca->frame_size = config->frame_size;
	dca->opus_mode = config->opus_mode;

	dca_source_t *src = dca_source_new(dca);
	if ((err = dca_source_open(src, config->infile)) < 0) {
		fprintf(stderr, "Couldn't open input: %s\n", get_av_err_str(err));
		return err;
	}

	dca_encoder_t *enc = dca_encoder_new_source(dca, src);

	// Fancypants encoding loop
	AVFrame *frame = av_frame_alloc();
	while (1) {
		int stop = 0;
		while (dca_encoder_needs_more(enc)) {
			if ((err = dca_source_read_frame(src, frame)) < 0) {
				if (err != AVERROR_EOF) {
					fprintf(stderr, "%s\n", get_av_err_str(err));
					stop = 1;
				}
				break;
			}

			if ((dca_encoder_feed_frame(enc, frame)) < 0) {
				fprintf(stderr, "Couldn't feed encoder: %s\n", get_av_err_str(err));
				stop = 1;
				av_frame_unref(frame);
				break;
			}

			av_frame_unref(frame);
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

	return 0;
}
