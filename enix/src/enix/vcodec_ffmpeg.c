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
 * $Id: vcodec_ffmpeg.c,v 1.3 2003/04/18 00:52:55 guenter Exp $
 *
 * enix ffmpeg video codec wrapper
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

#include "avcodec.h"

#include "enix.h"

/*
#define LOG
*/

#define OUTBUF_SIZE 100000

typedef struct {

  enix_venc_t        encoder;

  int                codec_id;
  AVCodec           *codec;
  AVCodecContext    *c;
  AVFrame           *picture;

  int                pass;

  char              *outbuf;
  int                out_length;

  int                y_size, uv_size;

} ffmpeg_t;


static void ffmpeg_init_encoder (enix_venc_t *this_gen, enix_stream_t *stream) {
  
  ffmpeg_t          *this = (ffmpeg_t *) this_gen;
  int                width, height, frame_duration;
  int                quality, bitrate, max_bframes;
  enix_options_t    *options;

  width          = stream->get_property (stream, ENIX_STREAM_PROP_WIDTH);
  height         = stream->get_property (stream, ENIX_STREAM_PROP_HEIGHT);
  frame_duration = stream->get_property (stream, ENIX_STREAM_PROP_FRAME_DURATION);
  options        = this->encoder.options;

#ifdef LOG
  printf ("vcodec_ffmpeg: width=%02d, height=%02d, bitrate=%d\n", 
	  width, height, bitrate);
#endif

  quality                   = options->get_num_option (options,
						       "quality");
  bitrate                   = options->get_num_option (options,
						       "bitrate");
  max_bframes               = options->get_num_option (options,
						       "max_bframes");

  /*
   * ffmpeg specific stuff starts here
   */

  this->codec = avcodec_find_encoder (this->codec_id);
  if (!this->codec) {
    printf ("vcodec_ffmpeg: error, codec %d not found\n", this->codec_id);
    exit (1);
  }

  this->c       = avcodec_alloc_context();
  this->picture = avcodec_alloc_frame();

  this->c->bit_rate       = bitrate / 1000;
  this->c->width          = width;  
  this->c->height         = height;
  this->c->frame_rate     = frame_duration * FRAME_RATE_BASE / 90000;
  this->c->gop_size       = 50;

  this->c->flags          = CODEC_FLAG_4MV | CODEC_FLAG_QPEL |  CODEC_FLAG_GMC ;
  this->c->max_b_frames   = max_bframes;

  /* open it */
  if (avcodec_open (this->c, this->codec) < 0) {
    printf ("vcodec_ffmpeg: could not open codec\n");
  }

  this->picture->linesize[0] = width;
  this->picture->linesize[1] = width / 2;
  this->picture->linesize[2] = width / 2;

  this->y_size  = width * height;
  this->uv_size = width * height /4;

}

static void ffmpeg_encode_frame (enix_venc_t *this_gen, xine_video_frame_t *frame,
				 int *is_keyframe) {

  ffmpeg_t            *this = (ffmpeg_t *) this_gen; 

  this->picture->data[0] = frame->data;
  this->picture->data[2] = frame->data + this->y_size;
  this->picture->data[1] = frame->data + this->y_size + this->uv_size;

  this->out_length = avcodec_encode_video (this->c, this->outbuf, 
					   OUTBUF_SIZE, this->picture);

#ifdef LOG
  printf ("%d bytes keyframe: %d, type %d\n", 
	  this->out_length,
	  this->c->coded_frame->key_frame,
	  this->picture->pict_type);
#endif

  *is_keyframe = this->c->coded_frame->key_frame;
}

static void ffmpeg_get_bitstream (enix_venc_t *this_gen, void *buf, int *num_bytes) {

  ffmpeg_t         *this = (ffmpeg_t *) this_gen;

  memcpy (buf, this->outbuf, this->out_length);
  *num_bytes = this->out_length;
}


static void ffmpeg_set_pass (enix_venc_t *this_gen, int pass) {

  ffmpeg_t         *this = (ffmpeg_t *) this_gen;

  if (pass) {

    if (pass==1)
      this->c->flags = CODEC_FLAG_PASS1 | CODEC_FLAG_4MV | CODEC_FLAG_QPEL |  CODEC_FLAG_GMC ;
    else
      this->c->flags = CODEC_FLAG_PASS2 | CODEC_FLAG_4MV | CODEC_FLAG_QPEL |  CODEC_FLAG_GMC ;

  }

  this->pass            = pass;
}

static void ffmpeg_flush (enix_venc_t *this_gen) {

  /* ffmpeg_t  *this = (ffmpeg_t *) this_gen; */
}

static void ffmpeg_dispose (enix_venc_t *this_gen) {

  ffmpeg_t  *this = (ffmpeg_t *) this_gen;

  free (this);
}

static int ffmpeg_get_properties (enix_venc_t *this_gen) {

  /* ffmpeg_t *this = (ffmpeg_t *) this_gen; */

  return 0; /* ENIX_ENC_2PASS; */
}

static void *ffmpeg_get_extra_info (enix_venc_t *this_gen, int info) {

  /* ffmpeg_t *this = (ffmpeg_t *) this_gen;  */

  switch (info) {
  case ENIX_INFO_FOURCC:
    return "DIVX";
  default:
    printf ("ffmpeg: error: request for info %d not implemented\n",
	    info);
    abort ();
  }

  return NULL;
}

enix_venc_t *create_ffmpeg_enc (int codec_id) {

  ffmpeg_t *this;

  this = malloc (sizeof (ffmpeg_t));

  this->encoder.init           = ffmpeg_init_encoder;
  this->encoder.set_pass       = ffmpeg_set_pass;
  this->encoder.get_headers    = NULL;
  this->encoder.encode_frame   = ffmpeg_encode_frame;
  this->encoder.get_bitstream  = ffmpeg_get_bitstream;
  this->encoder.flush          = ffmpeg_flush;
  this->encoder.dispose        = ffmpeg_dispose;
  this->encoder.get_properties = ffmpeg_get_properties;
  this->encoder.get_extra_info = ffmpeg_get_extra_info;
  this->encoder.options        = enix_create_options ();

  this->encoder.options->new_num_option (this->encoder.options, 
					 "quality", 7);
  this->encoder.options->new_num_option (this->encoder.options, 
					 "max_bframes", -1);
  this->encoder.options->new_num_option (this->encoder.options, 
					 "bitrate", 500000);

  this->codec    = NULL;
  this->codec_id = codec_id;
  this->c        = NULL;

  this->pass  = 0;

  this->outbuf     = malloc (OUTBUF_SIZE);
  this->out_length = 0;
  this->y_size     = 0;
  this->uv_size    = 0;

  printf ("vcodec_ffmpeg: registering all codecs...\n");

  avcodec_init();

  avcodec_register_all();

  return &this->encoder;
}

