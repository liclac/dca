#include "config.h"
#include <opus/opus_defines.h>
#include <string.h>
#include <stdlib.h>

void config_defaults(config_t *config) {
	config->opus_mode = OPUS_APPLICATION_AUDIO;
	config->bit_rate = 64000;
	config->channels = 2;
	config->sample_rate = 48000;
	config->frame_size = 960;
}

int parse_args(config_t *config, int argc, char **argv) {
	// Count the number of encountered positional arguments
	int positionals = 0;

	for (int i = 1; i < argc; i++) {
		char *arg = argv[i];
		size_t arglen = strlen(arg);

		// You never know...
		if (arglen == 0) {
			continue;
		}

		if (arg[0] == '-' && arglen != 1) {
			if (strcmp(arg, "-aa") == 0) {
				if (i == argc-2) {
					return ERR_ARG_NO_VALUE;
				}
				char *val = argv[++i];

				if (strcmp(val, "voip") == 0) {
					config->opus_mode = OPUS_APPLICATION_VOIP;
				} else if (strcmp(val, "audio") == 0) {
					config->opus_mode = OPUS_APPLICATION_AUDIO;
				} else if (strcmp(val, "lowdelay") == 0) {
					config->opus_mode = OPUS_APPLICATION_RESTRICTED_LOWDELAY;
				} else {
					return ERR_ARG_INVALID;
				}
			} else if (strcmp(arg, "-ab") == 0) {
				if (i == argc-2) {
					return ERR_ARG_NO_VALUE;
				}

				int val = atoi(argv[++i]);
				if (val == 0) {
					return ERR_ARG_INVALID;
				}
				config->bit_rate = val * 1000;
			} else if (strcmp(arg, "-ac") == 0) {
				if (i == argc-2) {
					return ERR_ARG_NO_VALUE;
				}

				int val = atoi(argv[++i]);
				if (val == 0) {
					return ERR_ARG_INVALID;
				}
				config->channels = val;
			} else if (strcmp(arg, "-ar") == 0) {
				if (i == argc-2) {
					return ERR_ARG_NO_VALUE;
				}

				int val = atoi(argv[++i]);
				if (val == 0) {
					return ERR_ARG_INVALID;
				}
				config->sample_rate = val;
			} else if (strcmp(arg, "-as") == 0) {
				if (i == argc-2) {
					return ERR_ARG_NO_VALUE;
				}

				int val = atoi(argv[++i]);
				if (val == 0) {
					return ERR_ARG_INVALID;
				}
				config->frame_size = val;
			} else {
				return ERR_ARG_UNKNOWN;
			}
		} else {
			if (positionals == 0) {
				config->infile = arg;
			} else {
				return ERR_ARG_UNKNOWN;
			}
			++positionals;
		}
	}

	return 0;
}
