/// \file
#ifndef DCA_DCA_H
#define DCA_DCA_H

#include <stdint.h>

/**
 * Latest DCA version the library supports.
 */
const static uint8_t DCA_VERSION = 0;

/**
 * A DCA file.
 */
typedef struct {
	uint8_t version;	///< DCA version.
} dca_t;

/**
 * Creates a new DCA structure.
 * 
 * The caller takes ownership of the newly allocated structure, and must free it using dca_free().
 * 
 * @param  version DCA version
 * @return         A newly allocated DCA structure.
 */
dca_t* dca_new(uint8_t version);

/**
 * Frees a DCA structure.
 * 
 * @param dca DCA structure to free.
 */
void dca_free(dca_t *dca);

/**
 * Returns a DCA structure's version.
 */
uint8_t dca_version(dca_t *dca);

#endif
