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
	config_t *config = malloc(sizeof(config_t));
	config_defaults(config);
	if ((err = parse_args(config, argc, argv) || config->infile == NULL)) {
		print_usage(argv[0]);
		return err;
	}

	avcodec_register_all();

	AVCodecContext *octx;
	if (make_output_context(config, &octx)) {
		return err;
	}

	return 0;
}
