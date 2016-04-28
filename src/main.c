#include <dca/dca.h>
#include <stdio.h>

int main(int argc, char **argv) {
	dca_t *dca = dca_new(DCA_VERSION);
	printf("DCA version: %d\n", dca_version(dca));
	dca_free(dca);
}
