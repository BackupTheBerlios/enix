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
 * $Id: capture.c,v 1.1 2003/02/23 22:45:22 guenter Exp $
 *
 * low-latency v4l/oss stream capture
 */

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>

#include "enix.h"

#define WIDTH                    384
#define HEIGHT                   288
#define SAMPLE_RATE            11025
#define AUDIO_FRAME_BYTES       4000
#define AUDIO_DEVICE      "/dev/dsp"

typedef struct {

  enix_stream_t       enix_stream;

  int                 audio_fd;

  int64_t             start_time;
  int                 samples_read;

  char               *audio_buf;
  char               *video_buf;

} enix_capture_stream_t ;

static int64_t get_time() {
  struct timeval tv;
  gettimeofday (&tv, NULL);

  return (int64_t) tv.tv_sec * 90000 + (int64_t) tv.tv_usec * 9 / 100;
}

static  int enix_capture_play (enix_stream_t *stream, 
			       int start_pos, 
			       int start_time) {

  enix_capture_stream_t *this = (enix_capture_stream_t *) stream;
  int                    err, tmp;

  if (this->audio_fd>=0) {
    close (this->audio_fd);
    this->audio_fd = -1;
  }

  this->audio_fd = open (AUDIO_DEVICE, O_RDONLY);

  tmp = AFMT_S16_LE;
  err=ioctl(this->audio_fd, SNDCTL_DSP_SETFMT, &tmp);
  if (err < 0) {
    perror("SNDCTL_DSP_SETFMT");
    exit (1);
  }
      
  tmp = 0;
  err = ioctl(this->audio_fd, SNDCTL_DSP_STEREO, &tmp);
  if (err < 0) {
    perror("SNDCTL_DSP_STEREO");
    exit (1);
  }
      
  tmp = 11025;
  err = ioctl(this->audio_fd, SNDCTL_DSP_SPEED, &tmp);
  if (err < 0) {
    perror("SNDCTL_DSP_SPEED");
    exit (1);
  }

  this->start_time   = get_time ();

  this->samples_read = 0;

  return 1;
}

static int enix_capture_get_next_video_frame (enix_stream_t *stream,
					      xine_video_frame_t *frame) {
  enix_capture_stream_t *this = (enix_capture_stream_t *) stream;

  frame->vpts         = get_time () - this->start_time;     
  frame->duration     = 3600;
  frame->width        = WIDTH;
  frame->height       = HEIGHT;
  frame->colorspace   = XINE_IMGFMT_YV12; 
  frame->aspect_ratio = 4.0/3.0;
  frame->pos_stream   = 0; 
  frame->pos_time     = 0;   
  frame->data         = this->video_buf;
  frame->xine_frame   = NULL; 

  return 0;
}
  
static int enix_capture_get_next_audio_frame (enix_stream_t *stream,
					      xine_audio_frame_t *frame) {
  enix_capture_stream_t *this = (enix_capture_stream_t *) stream;
  int64_t                t, pts;
  int                    num_bytes, new_samples, delay;
  count_info             info;

  if (this->audio_fd<0) {
    printf ("capture: get_next_audio_frame called but audio fd not there\n");
    abort ();
  }

  num_bytes = read (this->audio_fd, this->audio_buf, AUDIO_FRAME_BYTES);
  new_samples = num_bytes / 2 ;

  ioctl (this->audio_fd, SNDCTL_DSP_GETIPTR, &info);
  delay = (info.bytes / 2 ) - this->samples_read;

  this->samples_read += new_samples;

  /*
    printf ("info.bytes: %d, delay: %lld, samples_read: %lld\n",
    info.bytes, delay, samples_read);
  */
  
  t = get_time() - this->start_time;
  pts = t - delay * 90000 / SAMPLE_RATE ;

  frame->vpts             = pts;
  frame->num_samples      = new_samples;
  frame->sample_rate      = SAMPLE_RATE;
  frame->num_channels     = 1;
  frame->bits_per_sample  = 16;
  frame->pos_stream       = this->samples_read * 2;
  frame->pos_time         = 0;
  frame->data             = this->audio_buf;
  frame->xine_frame       = NULL;
    
  return 0;
}
  
static void enix_capture_dispose (enix_stream_t *stream) {
  enix_capture_stream_t *this = (enix_capture_stream_t *) stream;

  free (this);
}

int enix_capture_get_property (enix_stream_t *stream_gen, int property) {

  /* enix_capture_stream_t *this = (enix_capture_stream_t *) stream_gen; */
  
  switch (property) {

  case ENIX_STREAM_PROP_LENGTH: 
    return 0; /* live */

  case ENIX_STREAM_PROP_WIDTH:
    return WIDTH;

  case ENIX_STREAM_PROP_HEIGHT:
    return HEIGHT;

  case ENIX_STREAM_PROP_HAS_VIDEO:
    return 1;

  case ENIX_STREAM_PROP_HAS_AUDIO:
    return 1;

  case ENIX_STREAM_PROP_FRAME_DURATION:
    return 3600;

  default:
    printf ("capture: error, unknown property %d\n", property);
  }

  return 0;
}

enix_stream_t *enix_capture_stream_new (void) {

  enix_capture_stream_t *this;

  this = malloc (sizeof (enix_capture_stream_t));

  this->audio_fd = -1;

  this->audio_buf = malloc (AUDIO_FRAME_BYTES);
  this->video_buf = malloc (WIDTH * HEIGHT * 2);

  this->enix_stream.play                 = enix_capture_play;
  this->enix_stream.get_next_video_frame = enix_capture_get_next_video_frame;
  this->enix_stream.get_next_audio_frame = enix_capture_get_next_audio_frame;
  this->enix_stream.dispose              = enix_capture_dispose;
  this->enix_stream.get_property         = enix_capture_get_property;

  return (enix_stream_t *) this;
}


