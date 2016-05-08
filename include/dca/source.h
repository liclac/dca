/**
 * @file dca/source.h
 *
 * Convenient wrappers around ffmepg's demuxer and decoder libraries for reading audio frames.
 * Intended to be used together with a dca_encoder_t, but you're free to use your own sample source
 * with an encoder if you prefer.
 */
#ifndef DCA_SOURCE_H
#define DCA_SOURCE_H

#include <dca/dca.h>
#include <libavformat/avformat.h>

/**
 * A DCA input source.
 */
typedef struct {
	dca_t *dca;					///< DCA metadata.

	AVFormatContext *fctx;		///< Demuxer.
	AVCodecContext *ctx;		///< Decoder.
	int stream_id;				///< ID of the audio stream to work with.
} dca_source_t;

/**
 * Creates a new audio source.
 *
 * @param  dca DCA structure for metadata.
 * @return     A newly allocated source, NULL on error.
 */
dca_source_t* dca_source_new(dca_t *dca);

/**
 * Opens an input file.
 *
 * @param  src      Source.
 * @param  filename Filename (can be a path, URL or pipe:x)
 * @return          < 0 on error.
 */
int dca_source_open(dca_source_t *src, const char *filename);

/**
 * Frees a source and all associated data.
 *
 * @param src Source.
 */
void dca_source_free(dca_source_t *src);

/**
 * Reads an audio frame.
 *
 * Note that the error returned can simply be AVERROR_EOF, which obviously isn't fatal.
 *
 * @param  src       Source.
 * @param  out_frame Frame to write to.
 * @return           < 0 on error.
 */
int dca_source_read_frame(dca_source_t *src, AVFrame *out_frame);

#endif
