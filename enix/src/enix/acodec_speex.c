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
 * $Id: acodec_speex.c,v 1.1 2003/04/27 16:59:15 guenter Exp $
 *
 * enix speex audio codec wrapper
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

#include <speex.h>

#include "enix.h"

#define MAX_SAMPLES 32768

typedef struct {

  enix_aenc_t        encoder;

  SpeexBits          bits; 

  void              *enc_state; 

  int                frame_size;

  float              samples[MAX_SAMPLES];
  int                num_samples;

} speex_t;


		
static void speex_init_encoder (enix_aenc_t *this_gen, enix_stream_t *stream) {
  
  speex_t          *this = (speex_t *) this_gen;
  int                bitrate, channels, sample_rate;
  enix_options_t    *options;
  int                ret;

  /* 
   * gather parameters
   */

  options      = this->encoder.options;
  bitrate      = options->get_num_option (options, "bitrate");
  channels     = stream->get_property (stream, ENIX_STREAM_PROP_AUDIO_CHANNELS);
  sample_rate  = stream->get_property (stream, ENIX_STREAM_PROP_SAMPLE_RATE);

  printf ("acodec_speex: %d audio channels, %d samples/sec\n",
	  channels, sample_rate);

  /*
   * speex init
   */

  speex_bits_init (&this->bits);

  this->enc_state = speex_encoder_init (&speex_nb_mode);   

  speex_encoder_ctl (this->enc_state, SPEEX_GET_FRAME_SIZE, &this->frame_size);
  printf ("acodec_speex: codec init done. frame size is %d\n", this->frame_size);

  this->num_samples = 0;
}

static void speex_encode_frame (enix_aenc_t *this_gen, xine_audio_frame_t *frame) {

#if 0
  speex_t      *this = (speex_t *) this_gen;
  int            bytes, i;
  float        **buffer;
  signed char   *readbuffer;

  /* data to encode */

  /* expose the buffer to submit data */

  bytes  = frame->num_samples * frame->num_channels * frame->bits_per_sample/8;

#ifdef LOG
  printf ("oxv: %d samples (%d bytes), %d channels\n", 
	  frame->num_samples, bytes, frame->num_channels);
#endif

  buffer = speex_analysis_buffer (&this->vd, bytes);

  readbuffer = frame->data;
      
  /* uninterleave samples */
  for (i=0;i<bytes/4;i++) {
    buffer[0][i] = ((readbuffer[i*4+1]<<8)|
		    (0x00ff&(int)readbuffer[i*4]))/32768.f;
    buffer[1][i] = ((readbuffer[i*4+3]<<8)|
		    (0x00ff&(int)readbuffer[i*4+2]))/32768.f;
  }
  
  /* tell the library how much we actually submitted */
  speex_analysis_wrote (&this->vd, i);
#endif
}

static void speex_get_bitstream (enix_aenc_t *this_gen, void *buf, int *num_bytes) {

  speex_t      *this = (speex_t *) this_gen;

  *num_bytes = 0;

#if 0
  if (!speex_bitrate_flushpacket(&this->vd, &this->op)) {

    if (speex_analysis_blockout(&this->vd, &this->vb)!=1) 
      return ;

    speex_analysis (&this->vb, NULL);
    speex_bitrate_addblock (&this->vb);
    
    if (speex_bitrate_flushpacket(&this->vd, &this->op)) {

      this->op.packetno = this->audio_cnt++;

      memcpy (buf, &this->op, sizeof (ogg_packet));
      *num_bytes = sizeof (ogg_packet);
    } else
      return;
  } else {
    this->op.packetno = this->audio_cnt++;

    memcpy (buf, &this->op, sizeof (ogg_packet));
    *num_bytes = sizeof (ogg_packet);
  }
#endif
}


static void speex_flush (enix_aenc_t *this_gen) {

#if 0
  speex_t  *this = (speex_t *) this_gen; 
#endif
}


static void speex_dispose (enix_aenc_t *this_gen) {

  speex_t *this = (speex_t *) this_gen;

  free (this);
}

static int speex_get_properties (enix_aenc_t *this_gen) {

  /* speex_t *this = (speex_t *) this_gen; */

  return 0;
}

static void *speex_get_extra_info (enix_aenc_t *this_gen, int info) {

  speex_t *this = (speex_t *) this_gen; 

  switch (info) {
  case ENIX_INFO_FOURCC:
    return "SPEX";
  default:
    printf ("speex: error: request for info %d not implemented\n",
	    info);
    abort ();
  }

  return NULL;
}

enix_aenc_t *create_speex_enc (void) {

  speex_t *this;

  this = malloc (sizeof (speex_t));

  this->encoder.init           = speex_init_encoder;
  this->encoder.encode_frame   = speex_encode_frame;
  this->encoder.get_bitstream  = speex_get_bitstream;
  this->encoder.flush          = speex_flush;
  this->encoder.dispose        = speex_dispose;
  this->encoder.get_properties = speex_get_properties;
  this->encoder.get_extra_info = speex_get_extra_info;
  this->encoder.options        = enix_create_options ();

  this->encoder.options->new_num_option (this->encoder.options, "bitrate", 8000);

  return &this->encoder;
}

