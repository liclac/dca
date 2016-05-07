#include <dca/encoder.h>
#include <stdlib.h>

dca_encoder_t* dca_encoder_new(dca_t *dca) {
	dca_encoder_t *enc = malloc(sizeof(dca_encoder_t));
	enc->dca = dca;
	return enc;
}

void dca_encoder_free(dca_t *dca) {
	free(dca);
}
