#ifndef HAVE_SCALER_H
#define HAVE_SCALER_H

#include <enix.h>

enix_stream_t *enix_scaler_new (enix_stream_t *src,
				int dst_w, int clip) ;

#endif
