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
 * $Id: vcodec_xvid.c,v 1.2 2003/04/18 00:52:55 guenter Exp $
 *
 * enix xvid video codec wrapper
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

#include <xvid.h>

#include "enix.h"

/*
#define LOG
*/

#define HINT_FILE "/tmp/hints.enix.xvid"

#define OUTBUF_SIZE  100*1024
#define HINTBUF_SIZE 100*1024

typedef struct {

  enix_venc_t        encoder;

  void              *xvid_handle;

  int                frame_num;

  int                bitrate, quality;
  char               outbuf[OUTBUF_SIZE];
  int                out_length;

  int                pass;
  char               hints_buffer[HINTBUF_SIZE];
  FILE              *hints_file;

  XVID_INIT_PARAM    xinit;
} xvid_t;


/*
 * xvid quality presets
 */

static int const motion_presets[8] = {
  0,                                                        /* Q 0 */
  PMV_EARLYSTOP16,                                          /* Q 1 */
  PMV_EARLYSTOP16,                                          /* Q 2 */
  PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,                    /* Q 3 */
  PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,                    /* Q 4 */
  PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8 |  /* Q 5 */
  PMV_HALFPELREFINE8,
  PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 |                   /* Q 6 */
  PMV_USESQUARES16 | PMV_EARLYSTOP8 | PMV_HALFPELREFINE8,

  PMV_HALFPELREFINE16 | PMV_EXTSEARCH16 | PMV_USESQUARES16 /* Q 7 */
#if 0
  PMV_QUARTERPELREFINE16 | PMV_EXTSEARCH16 | PMV_USESQUARES16 /* Q 7 */
#endif
};

static int const general_presets[8] = {
  XVID_H263QUANT,	                        /* Q 0 */
  XVID_MPEGQUANT,                               /* Q 1 */
  XVID_H263QUANT,                               /* Q 2 */
  XVID_H263QUANT | XVID_HALFPEL,                /* Q 3 */
  XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V, /* Q 4 */
  XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V, /* Q 5 */
  XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V, /* Q 6 */
  XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V | XVID_ME_FULLSEARCH | XVID_CHROMAOPT | XVID_GMC /* Q 7 */
};
		
static void xvid_init_encoder (enix_venc_t *this_gen, enix_stream_t *stream) {
  
  xvid_t            *this = (xvid_t *) this_gen;
  XVID_ENC_PARAM     xparam;
  int                xerr;
  int                width, height, frame_duration;
  enix_options_t    *options;

  width          = stream->get_property (stream, ENIX_STREAM_PROP_WIDTH);
  height         = stream->get_property (stream, ENIX_STREAM_PROP_HEIGHT);
  frame_duration = stream->get_property (stream, ENIX_STREAM_PROP_FRAME_DURATION);
  options        = this->encoder.options;

#ifdef LOG
  printf ("xvid: width=%02d, height=%02d\n", width, height);
#endif

  this->quality                   = options->get_num_option (options,
							     "quality");
  this->bitrate                   = options->get_num_option (options,
							     "bitrate");

  xparam.width                    = width;
  xparam.height                   = height;

  xparam.fbase                    = 90000;
  xparam.fincr                    = frame_duration;

  xparam.rc_reaction_delay_factor = 16;
  xparam.rc_averaging_period      = 100;
  xparam.rc_buffer                = 10;
  xparam.rc_bitrate               = this->bitrate;
  xparam.min_quantizer            = 1;
  xparam.max_quantizer            = 31;
  xparam.max_key_interval         = 250;
  xparam.bquant_ratio             = 120;
  xparam.bquant_offset            = 0;
  xparam.max_bframes              = options->get_num_option (options,
							     "max_bframes");;
  xparam.frame_drop_ratio         = 0;
  xparam.global                   = 0;

  xerr = xvid_encore (NULL, XVID_ENC_CREATE, &xparam, NULL);

#ifdef LOG
  printf ("xvid: enc_create done, xerr = %d\n", xerr);
#endif

  if (xerr) {

    printf ("xvid: XVID_ENC_CREATE failed.\n");
    exit (1);

  }

  this->xvid_handle = xparam.handle;
}

static void xvid_encode_frame (enix_venc_t *this_gen, xine_video_frame_t *frame,
			       int *is_keyframe) {

  xvid_t            *this = (xvid_t *) this_gen;
  XVID_ENC_FRAME     xframe;
  XVID_ENC_STATS     xstats;
  int                xerr;

#ifdef LOG
  printf ("xvid: encoding frame #%d (%dx%d)\n", 
	  this->frame_num, frame->width, frame->height);
#endif

  xframe.bitstream = this->outbuf;
  xframe.length    = -1;
  xframe.image     = frame->data;

  if (frame->colorspace == XINE_IMGFMT_YV12)
    xframe.colorspace = XVID_CSP_YV12;
  else
    xframe.colorspace = XVID_CSP_YUY2;
  xframe.intra              = -1;
  xframe.quant              = 0;
  xframe.bquant             = 0;
  xframe.motion             = motion_presets [this->quality];
  xframe.general            = general_presets [this->quality];
  xframe.quant_intra_matrix = xframe.quant_inter_matrix = NULL;
  xframe.stride             = frame->width;
  xframe.hint.hintstream    = this->hints_buffer;
  
  if (this->pass==2) {
    int hints_size;

    fread (&hints_size, 1, sizeof (long), this->hints_file);
    fread (this->hints_buffer, 1, hints_size, this->hints_file);
    xframe.hint.hintlength = hints_size;
    xframe.hint.rawhints   = 0;
    xframe.general |= XVID_HINTEDME_SET;
  }

  if (this->pass==1) {
    xframe.hint.rawhints = 0;
    xframe.general |= XVID_HINTEDME_GET;
  }

  xerr = xvid_encore (this->xvid_handle, XVID_ENC_ENCODE, &xframe, &xstats);

#ifdef LOG
  printf ("xvid: encore...done (%d bytes, err=%d, intra=%d)\n", 
	  xframe.length, xerr, xframe.intra);
#endif

  if (this->pass == 1) {
    int hints_size = xframe.hint.hintlength;

    fwrite (&hints_size, 1, sizeof(long), this->hints_file);
    fwrite (this->hints_buffer, 1, hints_size, this->hints_file);

  } 

  *is_keyframe = xframe.intra;
  this->out_length = xframe.length;
}

static void xvid_get_bitstream (enix_venc_t *this_gen, void *buf, int *num_bytes) {
  xvid_t         *this = (xvid_t *) this_gen;

  memcpy (buf, this->outbuf, this->out_length);
  *num_bytes = this->out_length;
}


static void xvid_set_pass (enix_venc_t *this_gen, int pass) {

  xvid_t         *this = (xvid_t *) this_gen;

  if (pass) {

    /* hints file */

    char *rights = "rb";

    if (this->hints_file) {
      fclose (this->hints_file);
      this->hints_file = NULL;
    }

    if (pass==1)
      rights = "w+b";
    
    /* open the hint file */
    this->hints_file = fopen (HINT_FILE, rights);
    if (this->hints_file == NULL) {
      printf ("oxv: error opening hints file %s\n", HINT_FILE);
      exit (1);
    }
  }

  this->frame_num       = 0;
  this->pass            = pass;
}

static void xvid_flush (enix_venc_t *this_gen) {

  /* xvid_t  *this = (xvid_t *) this_gen; */
}

static void xvid_dispose (enix_venc_t *this_gen) {

  xvid_t  *this = (xvid_t *) this_gen;
  int      xerr;

  xerr = xvid_encore (this->xvid_handle, XVID_ENC_DESTROY, NULL, NULL);

  this->xvid_handle = 0;

  if (this->hints_file) {
    fclose (this->hints_file);
    this->hints_file = NULL;
  }

  free (this);
}

static int xvid_get_properties (enix_venc_t *this_gen) {

  /* xvid_t *this = (xvid_t *) this_gen; */

  return ENIX_ENC_2PASS;
}

static void *xvid_get_extra_info (enix_venc_t *this_gen, int info) {

  /* xvid_t *this = (xvid_t *) this_gen;  */

  switch (info) {
  case ENIX_INFO_FOURCC:
    return "XVID";
  default:
    printf ("xvid: error: request for info %d not implemented\n",
	    info);
    abort ();
  }

  return NULL;
}

enix_venc_t *create_xvid_enc (void) {

  xvid_t *this;

  this = malloc (sizeof (xvid_t));

  this->encoder.init           = xvid_init_encoder;
  this->encoder.set_pass       = xvid_set_pass;
  this->encoder.get_headers    = NULL;
  this->encoder.encode_frame   = xvid_encode_frame;
  this->encoder.get_bitstream  = xvid_get_bitstream;
  this->encoder.flush          = xvid_flush;
  this->encoder.dispose        = xvid_dispose;
  this->encoder.get_properties = xvid_get_properties;
  this->encoder.get_extra_info = xvid_get_extra_info;
  this->encoder.options        = enix_create_options ();

  this->xvid_handle     = 0;
  this->xinit.cpu_flags = 0;
  this->hints_file      = NULL;
  this->pass            = 0;

  this->encoder.options->new_num_option (this->encoder.options, "quality", 7);
  this->encoder.options->new_num_option (this->encoder.options, "max_bframes", -1);
  this->encoder.options->new_num_option (this->encoder.options, "bitrate", 500000);

  xvid_init (NULL, 0, &this->xinit, NULL);

#ifdef LOG
  printf ("xvid: api version is 0x%08x\n", this->xinit.api_version);
#endif

  return &this->encoder;
}

