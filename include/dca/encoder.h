#ifndef DCA_ENCODER_H
#define DCA_ENCODER_H

#include <dca/dca.h>

/**
 * A DCA encoder.
 */
typedef struct {
	dca_t *dca;			///< DCA metadata.
} dca_encoder_t;

/**
 * Creates a new DCA encoder.
 *
 * The caller takes ownership of it, and must free it using dca_encoder_free().
 *
 * @param  dca DCA structure for metadata.
 * @return     A newly allocated encoder.
 */
dca_encoder_t* dca_encoder_new(dca_t *dca);

/**
 * Frees a DCA encoder.
 *
 * @param dca DCA encoder to free.
 */
void dca_encoder_free(dca_t *dca);

#endif
