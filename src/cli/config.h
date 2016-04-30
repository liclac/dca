#ifndef DCA_CLI_CONFIG_H
#define DCA_CLI_CONFIG_H

/**
 * Configuration values.
 */
typedef struct {
	const char *infile;		///< Input filename

	int opus_mode;			///< OPUS mode/application
	int bit_rate;			///< Audio bitrate
	int sample_rate;		///< Audio sample rate
	int channels;			///< Audio channels
	int frame_size;			///< Audio frame size
} config_t;

/**
 * Errors encountered while parsing parameters.
 */
enum {
	ERR_ARG_UNKNOWN,		///< There's an unknown argument somewhere
	ERR_ARG_NO_VALUE,		///< There should be a value, but there is none
	ERR_ARG_INVALID,		///< There's a value, but it's not valid
};

void config_defaults(config_t *config);

/**
 * Parses a list of commandline arguments.
 *
 * @param  argc Number of arguments
 * @param  argv Arguments
 * @return      0 on success, anything else on failure
 */
int parse_args(config_t *config, int argc, char **argv);

#endif
