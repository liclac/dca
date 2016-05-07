#include <dca/dca.h>
#include <stdlib.h>

dca_t* dca_new(uint8_t version) {
	dca_t *dca = malloc(sizeof(dca_t));
	dca->version = version;
	dca->bit_rate = DCA_DEFAULT_BIT_RATE;
	dca->sample_rate = DCA_DEFAULT_SAMPLE_RATE;
	dca->channels = DCA_DEFAULT_CHANNELS;
	dca->frame_size = DCA_DEFAULT_FRAME_SIZE;
	dca->opus_mode = DCA_DEFAULT_OPUS_MODE;
	return dca;
}

void dca_free(dca_t *dca) {
	free(dca);
}

uint8_t dca_version(dca_t *dca) {
	return dca->version;
}
