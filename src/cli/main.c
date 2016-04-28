#include "config.h"
#include <dca/dca.h>
#include <string.h>
#include <stdio.h>

/**
 * Prints a usage message to stderr.
 */
void print_usage(const char *pname);

int main(int argc, char **argv) {
	config_t config;
	config_defaults(&config);
	if (parse_args(&config, argc, argv) || config.infile == NULL) {
		print_usage(argv[0]);
		return 1;
	}
	
	printf("OPUS Mode: %d\n", config.opus_mode);
	
	dca_t *dca = dca_new(DCA_VERSION);
	printf("DCA version: %d\n", dca_version(dca));
	dca_free(dca);
}

void print_usage(const char *pname) {
	printf("Usage: %s [flags] input\n", pname);
	printf("Flags:\n");
	printf("    -aa (audio|voip|lowdelay) [default: audio]\n");
	printf("        OPUS application\n");
	printf("    -ab ... [default: 64]\n");
	printf("        Audio bitrate, in kb/s\n");
	printf("    -ac ... [default: 2]\n");
	printf("        Audio channels\n");
	printf("    -ar ... [default: 48000]\n");
	printf("        Audio sample rate\n");
	printf("    -as ... [default: 960]\n");
	printf("        Frame size, can be 960 (20ms), 1920 (40ms) or 2880 (60ms)\n");
}
