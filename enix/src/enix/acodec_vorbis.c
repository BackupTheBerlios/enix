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
 * $Id: acodec_vorbis.c,v 1.2 2003/03/12 16:16:04 guenter Exp $
 *
 * enix vorbis audio codec wrapper
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

#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>

#include "enix.h"

#define HINT_FILE "/tmp/hints.enix.vorbis"

typedef struct {

  enix_aenc_t        encoder;

  vorbis_info        vi; 
  vorbis_comment     vc; 
  vorbis_dsp_state   vd; 
  vorbis_block       vb; 

  ogg_packet         header;
  ogg_packet         header_comm;
  ogg_packet         header_code;

  ogg_packet         op;
  int                audio_cnt;
} vorbis_t;


		
static void vorbis_init_encoder (enix_aenc_t *this_gen, enix_stream_t *stream) {
  
  vorbis_t          *this = (vorbis_t *) this_gen;
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

  printf ("acodec_vorbis: %d audio channels, %d samples/sec\n",
	  channels, sample_rate);

  /*
   * vorbis init
   */

  vorbis_info_init (&this->vi);

#if 0
  vorbis_encode_init (&this->vi, channels, sample_rate,
		      bitrate*2, bitrate, bitrate/2);
#endif

  ret = vorbis_encode_setup_vbr (&this->vi, channels, sample_rate, 1);

  if (ret) {
    printf ("oxv: help, vorbis_encode_init failed.\n");
    exit(1);
  }

  vorbis_encode_setup_init (&this->vi);

  /* add a comment */
  vorbis_comment_init (&this->vc);
  vorbis_comment_add_tag (&this->vc, "ENCODER", "xine|enix");

  /* set up the analysis state and auxiliary encoding storage */
  vorbis_analysis_init (&this->vd, &this->vi);
  vorbis_block_init (&this->vd, &this->vb);


  vorbis_analysis_headerout (&this->vd, &this->vc, &this->header, 
			     &this->header_comm, &this->header_code);


  this->audio_cnt = 0;
}

static void vorbis_encode_frame (enix_aenc_t *this_gen, xine_audio_frame_t *frame) {

  vorbis_t      *this = (vorbis_t *) this_gen;
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

  buffer = vorbis_analysis_buffer (&this->vd, bytes);

  readbuffer = frame->data;
      
  /* uninterleave samples */
  for (i=0;i<bytes/4;i++) {
    buffer[0][i] = ((readbuffer[i*4+1]<<8)|
		    (0x00ff&(int)readbuffer[i*4]))/32768.f;
    buffer[1][i] = ((readbuffer[i*4+3]<<8)|
		    (0x00ff&(int)readbuffer[i*4+2]))/32768.f;
  }
  
  /* tell the library how much we actually submitted */
  vorbis_analysis_wrote (&this->vd, i);
}

static void vorbis_get_bitstream (enix_aenc_t *this_gen, void *buf, int *num_bytes) {

  vorbis_t      *this = (vorbis_t *) this_gen;

  *num_bytes = 0;

  if (!vorbis_bitrate_flushpacket(&this->vd, &this->op)) {

    if (vorbis_analysis_blockout(&this->vd, &this->vb)!=1) 
      return ;

    vorbis_analysis (&this->vb, NULL);
    vorbis_bitrate_addblock (&this->vb);
    
    if (vorbis_bitrate_flushpacket(&this->vd, &this->op)) {

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
}


static void vorbis_flush (enix_aenc_t *this_gen) {

  vorbis_t  *this = (vorbis_t *) this_gen; 

  vorbis_analysis_wrote (&this->vd,0);
}


static void vorbis_dispose (enix_aenc_t *this_gen) {

  vorbis_t *this = (vorbis_t *) this_gen;

  free (this);
}

static int vorbis_get_properties (enix_aenc_t *this_gen) {

  /* vorbis_t *this = (vorbis_t *) this_gen; */

  return ENIX_ENC_IS_VORBIS;
}

static void *vorbis_get_extra_info (enix_aenc_t *this_gen, int info) {

  vorbis_t *this = (vorbis_t *) this_gen; 

  switch (info) {
  case ENIX_INFO_FOURCC:
    return "VORB";
  case ENIX_INFO_VORBISH_1:
    return &this->header;
  case ENIX_INFO_VORBISH_2:
    return &this->header_comm;
  case ENIX_INFO_VORBISH_3:
    return &this->header_code;
  default:
    printf ("vorbis: error: request for info %d not implemented\n",
	    info);
    abort ();
  }

  return NULL;
}

enix_aenc_t *create_vorbis_enc (void) {

  vorbis_t *this;

  this = malloc (sizeof (vorbis_t));

  this->encoder.init           = vorbis_init_encoder;
  this->encoder.encode_frame   = vorbis_encode_frame;
  this->encoder.get_bitstream  = vorbis_get_bitstream;
  this->encoder.flush          = vorbis_flush;
  this->encoder.dispose        = vorbis_dispose;
  this->encoder.get_properties = vorbis_get_properties;
  this->encoder.get_extra_info = vorbis_get_extra_info;
  this->encoder.options        = enix_create_options ();

  this->encoder.options->new_num_option (this->encoder.options, "bitrate", 128000);

  return &this->encoder;
}

