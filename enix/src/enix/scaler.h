#ifndef HAVE_SCALER_H
#define HAVE_SCALER_H

#include <enix.h>


#define ENIX_SCALER_MODE_AR_SQUARE 1
#define ENIX_SCALER_MODE_AR_KEEP 2


enix_stream_t *enix_scaler_new (enix_stream_t *src,
				int dst_w, int mode, int mod_y) ;

#endif
