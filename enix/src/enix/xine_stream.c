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
 * $Id: xine_stream.c,v 1.4 2004/07/29 20:37:10 dooh Exp $
 *
 * wrapper around xine engine streams providing the enix_stream_t interface
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xine_stream.h"

/*
#define LOG
*/

#define DEFAULT_ZERO_SIZE 32768

extern int verbosity;

static xine_t *xine = NULL;

typedef struct {

  enix_stream_t       enix_stream;

  xine_audio_port_t  *audio_port;
  xine_video_port_t  *video_port;
  xine_stream_t      *stream;

  xine_video_frame_t  video_frame;
  xine_audio_frame_t  audio_frame;

  int64_t             video_vpts, audio_vpts;
  int64_t             vpts_offset;
  int64_t             frame_duration;

  char               *zero_space;
  int                 zero_size;
} enix_xine_stream_t ;

static  int enix_xine_play (enix_stream_t *stream, 
			    int start_pos, 
			    int start_time) {

  enix_xine_stream_t *this = (enix_xine_stream_t *) stream;
  int                 ret;
  int64_t             video_vpts, audio_vpts;

  /*
   * release frames, stop engine
   */

  if (this->video_frame.xine_frame) {
    xine_free_video_frame (this->video_port, &this->video_frame);
    this->video_frame.xine_frame = NULL;
  }

  if (this->audio_frame.xine_frame) {
    xine_free_audio_frame (this->audio_port, &this->audio_frame);
    this->audio_frame.xine_frame = NULL;
  }

  printf ("xine_stream: calling xine_stop...\n");

  xine_stop (this->stream);

  printf ("xine_stream: done. calling xine_play (%d %d)\n",
	  start_pos, start_time);

  ret = xine_play (this->stream, start_pos, start_time);

  if (!ret) {
    printf ("xine_stream: xine_play failed\n");
    return 0;
  }

  /*
   * get an audio and a video frame, calc vpts offset
   */

  if (this->audio_port && 
      !xine_get_next_audio_frame (this->audio_port, &this->audio_frame)) {
    printf ("xine_stream: failed to get an audio frame\n");
    return 0;
  }

#ifdef LOG
    printf ("xine_stream: got audio frame with vpts=%lld length = %d\n",
	    this->audio_frame.vpts, 
	    this->audio_frame.num_samples * 90000 / this->audio_frame.sample_rate);
#endif

  if (this->video_port && 
      !xine_get_next_video_frame (this->video_port, &this->video_frame)) {
    printf ("xine_stream: failed to get an audio frame\n");
    return 0;
  }
  
  if (this->video_port) {
    video_vpts = this->video_frame.vpts;
    if (this->audio_port) 
      audio_vpts = this->audio_frame.vpts;
    else
      audio_vpts = video_vpts;
  } else {
    audio_vpts = this->audio_frame.vpts;
    video_vpts = audio_vpts;
  }

  printf ("xine_stream: audio_vpts = %lld, video_vpts = %lld\n",
	  audio_vpts, video_vpts);

  if (audio_vpts<video_vpts)
    this->vpts_offset = audio_vpts;
  else
    this->vpts_offset = video_vpts;

  this->video_vpts = this->vpts_offset;
  this->audio_vpts = this->vpts_offset;

  this->frame_duration = this->video_frame.duration;

  return ret;
}

static int enix_xine_get_next_video_frame (enix_stream_t *stream,
					   xine_video_frame_t *frame) {
  enix_xine_stream_t *this = (enix_xine_stream_t *) stream;
  int64_t             diff;

#ifdef LOG
  printf ("xine_stream: get_next_video_frame\n");
#endif  

  if (!this->video_port) {
    printf ("xine_stream: get_next_video_frame called but no video port requested\n");
    abort ();
  }

  /* 
   * need to get next video frame ?
   */

  while ( (diff = this->video_vpts - this->video_frame.vpts) > this->frame_duration ) {
    
#ifdef LOG
    printf ("xine_stream: need to get next video frame\n");
#endif

    xine_free_video_frame (this->video_port, &this->video_frame);
    if (!xine_get_next_video_frame (this->video_port, &this->video_frame)) 
      return 0;
  }

  this->video_vpts += this->frame_duration;

  memcpy (frame, &this->video_frame, sizeof (xine_video_frame_t));
  
  return 1;
}

static int enix_xine_get_next_audio_frame (enix_stream_t *stream,
					   xine_audio_frame_t *frame) {
  enix_xine_stream_t *this = (enix_xine_stream_t *) stream;
  int64_t             diff, frame_end, frame_length;

#ifdef LOG
  printf ("xine_stream: get_next_audio_frame\n");
#endif

  if (!this->audio_port) {
    printf ("xine_stream: get_next_audio_frame called but no audio port requested\n");
    abort ();
  }

  /* throw away the current frame ? */

  while (1) {

    frame_length = this->audio_frame.num_samples * 90000 / this->audio_frame.sample_rate;

    frame_end = this->audio_frame.vpts + frame_length;

    diff = this->audio_vpts - frame_end;

    if (diff<=0)
      break;

#ifdef LOG
    printf ("xine_stream: getting next audio frame, diff=%lld\n",
	    diff);
#endif

    if (this->audio_frame.xine_frame) {
      xine_free_audio_frame (this->audio_port, &this->audio_frame);
      this->audio_frame.xine_frame = NULL;
    }

    if (!xine_get_next_audio_frame (this->audio_port, &this->audio_frame)) 
      return 0;

#ifdef LOG
    printf ("xine_stream: got audio frame with vpts=%lld length = %d\n",
	    this->audio_frame.vpts, 
	    this->audio_frame.num_samples * 90000 / this->audio_frame.sample_rate);
#endif
  }

#ifdef LOG
  printf ("xine_stream: delivering audio frame with vpts=%lld, length = %lld\n",
	  this->audio_frame.vpts, frame_length);
#endif


  memcpy (frame, &this->audio_frame, sizeof (xine_audio_frame_t));

  /* insert 0-samples if necessary */
  if (this->audio_vpts < this->audio_frame.vpts) {

    int64_t num_missing_samples;
    int64_t num_missing_bytes;

    num_missing_samples = (this->audio_frame.vpts - this->audio_vpts) * this->audio_frame.sample_rate / 90000;

#ifdef LOG
    printf ("xine_stream: missing %lld audio samples\n", num_missing_samples);
#endif

    num_missing_bytes = num_missing_samples * this->audio_frame.num_channels * this->audio_frame.bits_per_sample / 8;

    if (this->zero_size<num_missing_bytes) {
      this->zero_space = realloc (this->zero_space, num_missing_bytes);
      this->zero_size = num_missing_bytes;
      memset (this->zero_space, 0, this->zero_size);
    }

    frame->data = this->zero_space;
    frame->num_samples = num_missing_samples;
    frame_length = this->audio_frame.vpts - this->audio_vpts;

  }

  frame->vpts = this->audio_vpts;

  this->audio_vpts += frame_length;

  return 1;
}
  
static void enix_xine_dispose (enix_stream_t *stream) {
  enix_xine_stream_t *this = (enix_xine_stream_t *) stream;

  if (this->audio_frame.xine_frame)
    xine_free_audio_frame (this->audio_port, &this->audio_frame);

  if (this->video_frame.xine_frame)
    xine_free_video_frame (this->video_port, &this->video_frame);

  xine_dispose (this->stream);
  free (this);
}

int enix_xine_get_property (enix_stream_t *stream_gen, int property) {

  enix_xine_stream_t *this = (enix_xine_stream_t *) stream_gen;
  
  switch (property) {

  case ENIX_STREAM_PROP_LENGTH: 
    {
      int pos_stream, pos_time, length_time;
      
      xine_get_pos_length (this->stream,
			   &pos_stream, &pos_time, &length_time);
      
      return length_time;
    }
    break;

  case ENIX_STREAM_PROP_WIDTH:
    return xine_get_stream_info (this->stream, XINE_STREAM_INFO_VIDEO_WIDTH);
    break;

  case ENIX_STREAM_PROP_HEIGHT:
    return xine_get_stream_info (this->stream, XINE_STREAM_INFO_VIDEO_HEIGHT);
    break;

  case ENIX_STREAM_PROP_ASPECT:
    return xine_get_stream_info (this->stream, XINE_STREAM_INFO_VIDEO_RATIO);
    break;

  case ENIX_STREAM_PROP_HAS_VIDEO:
    return xine_get_stream_info (this->stream, XINE_STREAM_INFO_HAS_VIDEO);
    break;

  case ENIX_STREAM_PROP_HAS_AUDIO:
    return xine_get_stream_info (this->stream, XINE_STREAM_INFO_HAS_AUDIO);
    break;

  case ENIX_STREAM_PROP_FRAME_DURATION:
    return xine_get_stream_info (this->stream, XINE_STREAM_INFO_FRAME_DURATION);
    break;

  case ENIX_STREAM_PROP_AUDIO_CHANNELS:
    printf ("xine_stream: audio channels : %d\n",
	    xine_get_stream_info (this->stream, XINE_STREAM_INFO_AUDIO_CHANNELS));
    return xine_get_stream_info (this->stream, XINE_STREAM_INFO_AUDIO_CHANNELS);
    break;

  case ENIX_STREAM_PROP_AUDIO_BITS:
    return xine_get_stream_info (this->stream, XINE_STREAM_INFO_AUDIO_BITS);
    break;

  case ENIX_STREAM_PROP_SAMPLE_RATE:
    return xine_get_stream_info (this->stream, XINE_STREAM_INFO_AUDIO_SAMPLERATE);
    break;

  default:
    printf ("xine_stream: error, unknown property %d\n", property);
    abort ();
  }

  return 0;
}




enix_stream_t *enix_xine_stream_new (char *mrl, int want_audio, int want_video) {

  enix_xine_stream_t *this;
  char                configfile[256];

  this = malloc (sizeof (enix_xine_stream_t));

  if (!xine) {
    xine = xine_new ();

    snprintf (configfile, 255, "%s/.gxine/config", getenv ("HOME"));
    xine_config_load (xine, configfile);
    
    xine_init (xine);

    xine_engine_set_param (xine, XINE_ENGINE_PARAM_VERBOSITY, verbosity);
  }

  if (want_audio) 
    this->audio_port = xine_new_framegrab_audio_port (xine); 
  else
    this->audio_port = NULL;

  if (want_video)
    this->video_port = xine_new_framegrab_video_port (xine);
  else
    this->video_port = NULL;

  this->stream = xine_stream_new (xine, this->audio_port, this->video_port);

  this->audio_frame.xine_frame = NULL;
  this->video_frame.xine_frame = NULL;

  this->video_vpts  = 0;
  this->audio_vpts  = 0;
  this->vpts_offset = 0;

  if (!xine_open (this->stream, mrl)) {
    printf ("Unable to open '%s'\n",mrl);
    abort();
  } 

  this->enix_stream.play                 = enix_xine_play;
  this->enix_stream.get_next_video_frame = enix_xine_get_next_video_frame;
  this->enix_stream.get_next_audio_frame = enix_xine_get_next_audio_frame;
  this->enix_stream.dispose              = enix_xine_dispose;
  this->enix_stream.get_property         = enix_xine_get_property;

  this->zero_size = DEFAULT_ZERO_SIZE;
  this->zero_space = malloc (this->zero_size);
  memset (this->zero_space, 0, this->zero_size);

  /* workaround
     sometimes the properties like heigth,width and frame_duration are only set
     when a play has occured*/     
  enix_xine_play((enix_stream_t *)this,0,0);

  return (enix_stream_t *) this;
}

