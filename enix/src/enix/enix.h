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
 * $Id: enix.h,v 1.1 2003/02/23 22:45:10 guenter Exp $
 *
 * enix.h: toplevel, basic data structures and utility functions
 */

#ifndef HAVE_ENIX_H
#define HAVE_ENIX_H

#define XINE_ENABLE_EXPERIMENTAL_FEATURES 

#include <xine.h>

/*
 * enix_stream_ts can be provided by various sources,
 * e.g. a wrapper around xine_stream_ts exists for 
 * opening video stream sources, but also filters
 * of various kinds can provide specialized enix_stream_ts
 */

typedef struct enix_stream_s enix_stream_t;

struct enix_stream_s {

  int  (*play) (enix_stream_t *stream, 
		int start_pos, 
		int start_time);
  
  int  (*get_next_video_frame) (enix_stream_t *stream,
				xine_video_frame_t *frame);
  
  int  (*get_next_audio_frame) (enix_stream_t *stream,
				xine_audio_frame_t *frame);
  
  void (*dispose) (enix_stream_t *stream);

  int (*get_property) (enix_stream_t *stream, int property); /* constants see below */

} ;

#define ENIX_STREAM_PROP_LENGTH         0 /* in msec */
#define ENIX_STREAM_PROP_WIDTH          1 
#define ENIX_STREAM_PROP_HEIGHT         2 
#define ENIX_STREAM_PROP_ASPECT         3
#define ENIX_STREAM_PROP_HAS_VIDEO      4 /* bool */
#define ENIX_STREAM_PROP_HAS_AUDIO      5 /* bool */
#define ENIX_STREAM_PROP_FRAME_DURATION 6 /* 1/90000 sec */
#define ENIX_STREAM_PROP_AUDIO_CHANNELS 7
#define ENIX_STREAM_PROP_SAMPLE_RATE    8
#define ENIX_STREAM_PROP_AUDIO_BITS     9

/*
 * option setting for codecs and other modules
 */

typedef struct enix_option_s enix_option_t;

struct enix_option_s {
  
  enix_option_t *next;
  char          *id;
  int            type;
  char          *str_value;
  int            num_value;
  int            min, max;

} ;

typedef struct enix_options_s enix_options_t;

struct enix_options_s {


  void (*new_num_option) (enix_options_t *this, char *option, int def_value);
  int  (*get_num_option) (enix_options_t *this, char *option);
  void (*set_num_option) (enix_options_t *this, char *option, int value);

#if 0 /* FIXME: later */
  void (*new_option) (enix_options_t *this, enix_option_t *option);
  void (*set_option) (enix_options_t *this, enix_option_t *option);
  void (*get_option) (enix_options_t *this, enix_option_t *option);
#endif

  void (*dispose) (enix_options_t *this);

  enix_option_t *options;
};

enix_options_t *enix_create_options (void);

/*
 * enix video and audio encoders provide this interface:
 */

typedef struct enix_venc_s enix_venc_t ;

struct enix_venc_s {

  void (*init) (enix_venc_t *this, enix_stream_t *stream);

  /* pass: 0 => single pass encoding, 1/2 => twopass encoding*/
  void (*set_pass) (enix_venc_t *this, int pass) ;

  /* returns number of bytes written to outbuf */
  void (*encode_frame)  (enix_venc_t *this, xine_video_frame_t *frame,
			 int *is_keyframe);
  void (*get_bitstream) (enix_venc_t *this, void *buf, int *num_bytes);

  void (*flush) (enix_venc_t *this);

  void (*dispose) (enix_venc_t *this);

  /* constants see below */
  int (*get_properties) (enix_venc_t *this);

  void* (*get_extra_info) (enix_venc_t *this, int info);

  enix_options_t *options;
};

typedef struct enix_aenc_s enix_aenc_t ;

struct enix_aenc_s {

  void (*init) (enix_aenc_t *this, enix_stream_t *stream);

  void (*encode_frame)  (enix_aenc_t *this, xine_audio_frame_t *frame);
  void (*get_bitstream) (enix_aenc_t *this, void *buf, int *num_bytes);

  void (*flush) (enix_aenc_t *this);

  void (*dispose) (enix_aenc_t *this);

  /* constants see below */
  int (*get_properties) (enix_aenc_t *this);
  
  void* (*get_extra_info) (enix_aenc_t *this, int info);

  enix_options_t *options;
};

#define ENIX_ENC_2PASS      1 /* encoder supports 2pass encoding */
#define ENIX_ENC_IS_VORBIS  2 /* needs special treatment         */

#define ENIX_INFO_FOURCC    1
#define ENIX_INFO_VORBISH_1 2
#define ENIX_INFO_VORBISH_2 3
#define ENIX_INFO_VORBISH_3 4

/*
 * this is the interface multiplexers provide in the enix world:
 */

typedef struct enix_mux_s enix_mux_t;

struct enix_mux_s {

  void (*init) (enix_mux_t *this, char *filename,
		enix_venc_t *video_encoder,
		enix_aenc_t *audio_encoder,
		enix_stream_t *stream);

  void (*run) (enix_mux_t *this);

  void (*dispose) (enix_mux_t *this);

  enix_options_t *options;
} ;

/*
 * this will probably soon be replaced by some dynamic plugin
 * loader/management mechanism, but for now, here are the
 * modules currently available in enix:
 */

enix_venc_t *create_xvid_enc (void) ;
enix_venc_t *create_ffmpeg_enc (void) ;
enix_aenc_t *create_vorbis_enc (void) ;
enix_mux_t  *create_ogm_mux (void) ;

/*
 * utility functions
 */

xine_video_frame_t *enix_video_frame_new (int w, int h) ;

/*
 * enix stream generators 
 */

enix_stream_t *enix_capture_stream_new (void) ;

#endif

