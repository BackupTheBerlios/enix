/*
 * Copyright (C) 2003 the xine project
 * 
 * This file is part of xine|enix, a free video processor.
 * 
 * xine|enix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine|enix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: vcodec_theora.c,v 1.1 2003/04/18 00:52:55 guenter Exp $
 *
 * enix theora video codec wrapper
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <theora/theora.h>


#include "enix.h"

/*
#define LOG
*/

#define OUTBUF_SIZE 100000

typedef struct {

  enix_venc_t        encoder;

  theora_state       td;
  theora_info        ti;

  ogg_packet         header;
  int                header_cnt;

  int                y_size, uv_size;

  ogg_packet         op;
  int                got_bits;

} theora_t;


static void theora_init_encoder (enix_venc_t *this_gen, enix_stream_t *stream) {
  
  theora_t          *this = (theora_t *) this_gen;
  int                width, height, frame_duration;
  int                quality, bitrate, max_bframes;
  enix_options_t    *options;

  width          = stream->get_property (stream, ENIX_STREAM_PROP_WIDTH);
  height         = stream->get_property (stream, ENIX_STREAM_PROP_HEIGHT);
  frame_duration = stream->get_property (stream, ENIX_STREAM_PROP_FRAME_DURATION);
  options        = this->encoder.options;

#ifdef LOG
  printf ("vcodec_theora: width=%02d, height=%02d, bitrate=%d\n", 
	  width, height, bitrate);
#endif

  quality                   = options->get_num_option (options,
						       "quality");
  bitrate                   = options->get_num_option (options,
						       "bitrate");
  max_bframes               = options->get_num_option (options,
						       "max_bframes");

  this->y_size  = width * height;
  this->uv_size = width * height /4;

  /*
   * theora specific stuff starts here
   */

  printf ("frame_duration: %d\n", frame_duration);

  this->ti.width              = width;
  this->ti.height             = height;
  this->ti.fps_numerator      = 90000;
  this->ti.fps_denominator    = frame_duration;
  this->ti.aspect_numerator   = width/height * 1000;
  this->ti.aspect_denominator = 1000;
  this->ti.target_bitrate     = bitrate;
  this->ti.target_bitrate     = -1;
  this->ti.quality            = quality;
  this->ti.quality            = 16;
  
  this->ti.dropframes_p                 = 0;
  this->ti.quick_p                      = 1;
  this->ti.keyframe_auto_p              = 1;
  this->ti.keyframe_frequency           = 64;
  this->ti.keyframe_frequency_force     = 64;
  /* this->ti.keyframe_data_target_bitrate = bitrate*1.5; */
  this->ti.keyframe_data_target_bitrate = -1;
  this->ti.keyframe_auto_threshold      = 80;
  this->ti.keyframe_mindistance         = 8;
  this->ti.noise_sensitivity            = 1;

  theora_encode_init (&this->td, &this->ti);

  theora_encode_header (&this->td, &this->header);
  this->header_cnt = 0;

}

static void theora_encode_frame (enix_venc_t *this_gen, xine_video_frame_t *frame,
				 int *is_keyframe) {

  theora_t            *this = (theora_t *) this_gen; 
  yuv_buffer          yuv;
  int                 ret;

  yuv.y = frame->data;
  yuv.v = frame->data + this->y_size;
  yuv.u = frame->data + this->y_size + this->uv_size;

  yuv.y_width   = frame->width;
  yuv.y_height  = frame->height;
  yuv.y_stride  = frame->width;
        
  yuv.uv_width  = frame->width / 2;
  yuv.uv_height = frame->height / 2;
  yuv.uv_stride = frame->width / 2;
        
  ret = theora_encode_YUVin (&this->td, &yuv);

  printf ("encoded frame %d x %d, ret=%d\n", frame->width, frame->height, ret);

  this->got_bits = 0;


#if 0 /* FIXME */
#ifdef LOG
  printf ("%d bytes keyframe: %d, type %d\n", 
	  this->out_length,
	  this->c->coded_frame->key_frame,
	  this->picture->pict_type);
#endif

  *is_keyframe = this->c->coded_frame->key_frame;
#endif
}

static int theora_get_headers (enix_venc_t *this_gen, void **header) {

  theora_t         *this = (theora_t *) this_gen;

  if (this->header_cnt)
    return 0;
  this->header_cnt++;

  *header = &this->header;
  return 1;
}

static void theora_get_bitstream (enix_venc_t *this_gen, void *buf, int *num_bytes) {

  theora_t         *this = (theora_t *) this_gen;


  if (this->got_bits) {
    *num_bytes = 0;
    return;
  }

  theora_encode_packetout (&this->td, 0, &this->op);
  this->got_bits = 1;

  *num_bytes = this->op.bytes;
  memcpy (buf, this->op.packet, this->op.bytes);

  printf ("%d bytes\n", this->op.bytes);


#if 0
  /* if there's only one frame, it's the last in the stream */
  if(state<2)
    theora_encode_packetout(td,1,&op);
  else
    theora_encode_packetout(td,0,&op);
#endif
}


static void theora_set_pass (enix_venc_t *this_gen, int pass) {

  /* theora_t         *this = (theora_t *) this_gen; */

}

static void theora_flush (enix_venc_t *this_gen) {

  /* theora_t  *this = (theora_t *) this_gen; */
}

static void theora_dispose (enix_venc_t *this_gen) {

  theora_t  *this = (theora_t *) this_gen;

  /* ogg_stream_clear (&this->to); */
  theora_clear (&this->td);

  free (this);
}

static int theora_get_properties (enix_venc_t *this_gen) {

  /* theora_t *this = (theora_t *) this_gen; */

  return 0; /* ENIX_ENC_2PASS; */
}

static void *theora_get_extra_info (enix_venc_t *this_gen, int info) {

  /* theora_t *this = (theora_t *) this_gen;  */

  switch (info) {
  case ENIX_INFO_FOURCC:
    return "DIVX";
  default:
    printf ("theora: error: request for info %d not implemented\n",
	    info);
    abort ();
  }

  return NULL;
}

enix_venc_t *create_theora_enc (void) {

  theora_t *this;

  this = malloc (sizeof (theora_t));

  this->encoder.init           = theora_init_encoder;
  this->encoder.set_pass       = theora_set_pass;
  this->encoder.get_headers    = theora_get_headers;
  this->encoder.encode_frame   = theora_encode_frame;
  this->encoder.get_bitstream  = theora_get_bitstream;
  this->encoder.flush          = theora_flush;
  this->encoder.dispose        = theora_dispose;
  this->encoder.get_properties = theora_get_properties;
  this->encoder.get_extra_info = theora_get_extra_info;
  this->encoder.options        = enix_create_options ();

  this->encoder.options->new_num_option (this->encoder.options, 
					 "quality", 7);
  this->encoder.options->new_num_option (this->encoder.options, 
					 "max_bframes", -1);
  this->encoder.options->new_num_option (this->encoder.options, 
					 "bitrate", 500000);

  return &this->encoder;
}

