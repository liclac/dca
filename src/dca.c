#include <dca/dca.h>
#include <stdlib.h>

dca_t* dca_new(uint8_t version) {
	dca_t *dca = malloc(sizeof(dca_t));
	dca->version = version;
	return dca;
}

void dca_free(dca_t *dca) {
	free(dca);
}

uint8_t dca_version(dca_t *dca) {
	return dca->version;
}
