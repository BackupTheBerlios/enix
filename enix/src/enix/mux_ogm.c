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
 * $Id: mux_ogm.c,v 1.8 2004/08/01 21:48:01 dooh Exp $
 *
 * enix ogm multiplexer
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

#include "enix.h"

#include <ogg/ogg.h>

/*
#define LOG
*/

/* according to http://tobias.everwicked.com/packfmt.htm */

#define PACKET_IS_SYNCPOINT      0x08

typedef struct dsogg_video_header_s {
  int32_t width;
  int32_t height;
} dsogg_video_header_t;

typedef struct dsogg_audio_header_s {
  int16_t channels;
  int16_t blockalign;
  int32_t avgbytespersec;
} dsogg_audio_header_t;

typedef struct {

  char streamtype[8];
  char subtype[4];

  int32_t size; /* size of the structure */

  int64_t time_unit; /* in reference time */
  int64_t samples_per_unit;
  int32_t default_len; /* in media time */

  int32_t buffersize;
  int16_t bits_per_sample;

  union {
    /* video specific */
    struct dsogg_video_header_s video;
    /* audio specific */
    struct dsogg_audio_header_s audio;
  } hubba;
} dsogg_header_t;

#define BUFSIZE 100 * 1024

typedef struct {

  enix_mux_t       mux;

  int              fh;

  enix_stream_t   *stream;
  enix_venc_t     *video_encoder;
  int64_t          video_bytes;
  enix_aenc_t     *audio_encoder;
  int64_t          audio_bytes;

  int              frame_num;
  int              video_cnt, audio_cnt;

  ogg_stream_state os_video, os_audio; 
  ogg_page         og_video, og_audio; 

  char             data[BUFSIZE];

} ogm_t;


static void ogm_init (enix_mux_t *this_gen, char *filename,
		      enix_venc_t *video_encoder,
		      enix_aenc_t *audio_encoder,
		      enix_stream_t *stream) {

  ogm_t             *this = (ogm_t *) this_gen;
  dsogg_header_t     dsoggh;
  ogg_packet         oph, opc; 
  ogg_packet        *ogghp;
  int                width, height, frame_duration;
  char              *fourcc;

  /*
   * open output file
   *
   * FIXME: use output plugins here
   */

  this->fh = open (filename, O_CREAT|O_TRUNC|O_WRONLY, 0644);

  if (this->fh<0) {
    printf ("mux_ogm failed to create file '%s'\n", filename);
    abort ();
  }

  /*
   * create ogg stream
   */

  srand (time (NULL));
  ogg_stream_init (&this->os_video, rand ());

  this->video_encoder = video_encoder;
  this->audio_encoder = audio_encoder;
  this->stream        = stream;

  this->video_cnt = 0;
  this->audio_cnt = 0;

  /* 
   * headers 
   */

  /* video */

  video_encoder->init (video_encoder, stream);

  if (video_encoder->get_headers) {
    int pagenumber=0;
    ogg_packet *op;

    while (video_encoder->get_headers (video_encoder, (void **) &op)) {
      pagenumber++;
      ogg_stream_packetin (&this->os_video, op);

      if (pagenumber==1) {
        /*The initial packet must be writen alone*/
        while (1) {
          int result = ogg_stream_flush (&this->os_video, &this->og_video);
          if (result==0)
            break;
          write (this->fh, this->og_video.header, this->og_video.header_len);
          write (this->fh, this->og_video.body, this->og_video.body_len);
        }      
      }

    }

  } else {

    fourcc = video_encoder->get_extra_info (video_encoder, ENIX_INFO_FOURCC);
    frame_duration = stream->get_property (stream, ENIX_STREAM_PROP_FRAME_DURATION);
    width  = stream->get_property (stream, ENIX_STREAM_PROP_WIDTH);
    height = stream->get_property (stream, ENIX_STREAM_PROP_HEIGHT);
    
    printf ("mux:mux_ogm frame_duration = %d\n", frame_duration);
    
    dsoggh.streamtype[0]    = 'v';
    dsoggh.streamtype[1]    = 'i';
    dsoggh.streamtype[2]    = 'd';
    dsoggh.streamtype[3]    = 'e';
    dsoggh.streamtype[4]    = 'o';
    dsoggh.streamtype[5]    = ' ';
    dsoggh.streamtype[6]    = ' ';
    dsoggh.streamtype[7]    = ' ';
    dsoggh.subtype[0]       = fourcc[0];
    dsoggh.subtype[1]       = fourcc[1];
    dsoggh.subtype[2]       = fourcc[2];
    dsoggh.subtype[3]       = fourcc[3];
    dsoggh.size             = sizeof (dsogg_header_t);
    dsoggh.time_unit        = frame_duration * 1000 / 9;
    dsoggh.samples_per_unit = 1;
    dsoggh.default_len      = 0; /* ? */
    dsoggh.buffersize       = 2048000;
    dsoggh.bits_per_sample  = 0;
    dsoggh.hubba.video.width      = width;
    dsoggh.hubba.video.height     = height;

    *this->data = 0x01;
    memcpy (this->data+1, &dsoggh, sizeof (dsogg_header_t));
    
    oph.packet     = this->data;
    oph.bytes      = sizeof (dsogg_header_t) + 1;
    oph.b_o_s      = 1;
    oph.e_o_s      = 0;
    oph.granulepos = 0;
    oph.packetno   = this->video_cnt++;
    
    ogg_stream_packetin (&this->os_video, &oph);
    
    opc.packet     = "\003enix generated";
    opc.bytes      = 15;
    opc.b_o_s      = 0;
    opc.e_o_s      = 0;
    opc.granulepos = 0;
    opc.packetno   = this->video_cnt++;
    
    ogg_stream_packetin (&this->os_video, &opc);
  }


  /* audio */

  audio_encoder->init (audio_encoder, stream);

  if (!(audio_encoder->get_properties(audio_encoder) & ENIX_ENC_IS_VORBIS)) {
    printf ("mux_ogm: error, audio is not vorbis\n");
    abort (); /*FIXME: support other audio types */
  }

  ogg_stream_init (&this->os_audio, rand());

  ogghp = audio_encoder->get_extra_info (audio_encoder, ENIX_INFO_VORBISH_1);
  ogg_stream_packetin (&this->os_audio, ogghp);
  ogghp = audio_encoder->get_extra_info (audio_encoder, ENIX_INFO_VORBISH_2);
  ogg_stream_packetin (&this->os_audio, ogghp);
  ogghp = audio_encoder->get_extra_info (audio_encoder, ENIX_INFO_VORBISH_3);
  ogg_stream_packetin (&this->os_audio, ogghp);

  while (1) {
    int result=ogg_stream_flush (&this->os_audio, &this->og_audio);
    if (result==0)
      break;
    write (this->fh, this->og_audio.header, this->og_audio.header_len);
    write (this->fh, this->og_audio.body, this->og_audio.body_len);
  }

  /*dump the remaining video headers*/
  while (1) {
    int result = ogg_stream_flush (&this->os_video, &this->og_video);
    if (result==0)
      break;
    write (this->fh, this->og_video.header, this->og_video.header_len);
    write (this->fh, this->og_video.body, this->og_video.body_len);
  }
}

static void ogm_encode_video_frame (ogm_t *this, xine_video_frame_t *frame,
				    int hints_only) {

  int         len, keyframe;
  ogg_packet  op;

  this->video_encoder->encode_frame (this->video_encoder, frame, &keyframe);

  if (!hints_only) {

    if (this->video_encoder->get_properties (this->video_encoder) & ENIX_ENC_OGG_PACKETS) {

      this->video_encoder->get_bitstream (this->video_encoder, 
					  &op, &len);

      this->frame_num++;
      this->video_cnt++;

    } else {

      this->video_encoder->get_bitstream (this->video_encoder, 
					  this->data+1, &len);

      op.packet      = this->data;
      if (keyframe) {
	this->data[0] = PACKET_IS_SYNCPOINT;
      } else {
	this->data[0] = 0;
      }
      op.bytes       = len+1;
      op.b_o_s       = 0;
      op.e_o_s       = 0;
      op.granulepos  = this->frame_num++;
      op.packetno    = this->video_cnt++;
    }
      
#ifdef LOG
    printf ("mux_ogm: video granulepos %lld\n", op.granulepos);
#endif    

    /* weld the packet into the bitstream */
    this->video_bytes=this->video_bytes+op.bytes;
    ogg_stream_packetin (&this->os_video, &op);

    /* write out pages (if any) */
    while (1){
      int result=ogg_stream_pageout (&this->os_video, &this->og_video);
      if (result==0)
	break;
      write (this->fh, this->og_video.header, this->og_video.header_len);
      write (this->fh, this->og_video.body, this->og_video.body_len);
    }
  }
}

static void ogm_write_audio_bitstream (ogm_t *this) {

  while (1) {
    ogg_packet     op;
    int            num_bytes;

    this->audio_encoder->get_bitstream (this->audio_encoder, &op, &num_bytes);

    if (!num_bytes)
      break;

    op.packetno = this->audio_cnt++;

#ifdef LOG
    printf ("mux_ogm: added audio package, %ld bytes\n", op.bytes);
    printf ("mux_ogm: granulepos %lld, packetno %lld, audio_cnt=%d\n",
	    op.granulepos, op.packetno, this->audio_cnt);
#endif      

    /* weld the packet into the bitstream */
    this->audio_bytes = this->audio_bytes + op.bytes;
    ogg_stream_packetin (&this->os_audio, &op);
      
    /* write out pages (if any) */
    while (1){
      int result=ogg_stream_pageout (&this->os_audio, &this->og_audio);
      if (result==0)
	break;

      write (this->fh, this->og_audio.header, this->og_audio.header_len);
      write (this->fh, this->og_audio.body, this->og_audio.body_len);
    }
  }
}

static void ogm_encode_audio_frame (ogm_t *this, xine_audio_frame_t *frame) {

  this->audio_encoder->encode_frame (this->audio_encoder, frame);

  ogm_write_audio_bitstream (this);
}

#define TERM_WIDTH 50

static void show_progress (int pos, int length, int64_t vbytes, int64_t abytes) {
  int stars, i;

  stars = pos * TERM_WIDTH / length;
  
  printf ("\r[");
  for (i=0; i<TERM_WIDTH; i++) {
    if (i<stars)
      printf ("*");
    else
      printf (" ");
  }

  i = 1024 * pos;
  if (i)
    printf ("] %3d%% (%d / %d) V-Bitrate %d A-Bitrate %d  ", pos*100/length, pos, length, (int) (vbytes*8000 / (1024*pos)), (int) (abytes*8000 / (1024*pos)) );
  else
    printf ("] %3d%% (%d / %d) V-Bitrate 0  ", pos*100/length, pos, length);

  fflush (stdout);
}

static void ogm_run (enix_mux_t *this_gen) {

  ogm_t         *this = (ogm_t *) this_gen;
  int64_t        audio_pts, video_pts;
  int            cnt;
  int            length;
  int            pass, passes;
  enix_stream_t *stream;

  stream = this->stream;

  if ( this->mux.options->get_num_option (this->mux.options, "2pass") )
    passes = 2;
  else
    passes = 1;

  length    = stream->get_property (stream, ENIX_STREAM_PROP_LENGTH);

  for (pass=0; pass<passes; pass++) {

    stream->play (stream, 0, 0);

    if (passes==2)
      this->video_encoder->set_pass (this->video_encoder, pass+1);
    else
      this->video_encoder->set_pass (this->video_encoder, 0);
    
    audio_pts = 0;
    video_pts = 0;
    cnt       = 0;

    show_progress (0, length,0,0);
    
    while (1) {
      
#ifdef LOG
      printf ("mux_ogm audio_pts: %lld, video_pts: %lld\n", 
	      audio_pts, video_pts);
#endif
      
      if (audio_pts<video_pts) {
	
	xine_audio_frame_t frame;
	
	if (!stream->get_next_audio_frame (stream, &frame)) 
	  break;
	
	audio_pts = frame.vpts;
	
	if (frame.pos_time) {
	  if (cnt>10) {
	    show_progress (frame.pos_time, length, this->video_bytes, this->audio_bytes);
	    cnt = 0;
	  }
	
	  cnt ++;
	}
	
#ifdef LOG
	printf ("mux_ogm got audio frame pts %lld\n", frame.vpts);
#endif
	if ( (passes==1) || (pass==1) )
	  ogm_encode_audio_frame (this, &frame);

      } else {
	
	xine_video_frame_t frame;
	
	if (!stream->get_next_video_frame (stream, &frame)) 
	  break;
	
	video_pts = frame.vpts;
	
#ifdef LOG
	printf ("mux_ogm got video frame %dx%d, pts %lld\n", 
		frame.width, frame.height, frame.vpts);
#endif
	if (frame.pos_time) {
	  if (cnt>10) {
	    show_progress (frame.pos_time, length, this->video_bytes, this->audio_bytes);
	    cnt = 0;
	  }
	
	  cnt ++;
	}

	if ( (passes==1) || (pass==1) )
	  ogm_encode_video_frame (this, &frame, 0);
	else
	  ogm_encode_video_frame (this, &frame, 1);
      }
    }
  }
}

static void ogm_dispose (enix_mux_t *this_gen) {

  ogm_t             *this = (ogm_t *) this_gen;

  /* 
   * flush audio and video
   */

  this->audio_encoder->flush (this->audio_encoder);

  ogm_write_audio_bitstream (this);

  while (1) {
    int result=ogg_stream_flush (&this->os_audio, &this->og_audio);
    if (result==0)
      break;
    write (this->fh, this->og_audio.header, this->og_audio.header_len);
    write (this->fh, this->og_audio.body, this->og_audio.body_len);
  }

  while (1) {
    int result = ogg_stream_flush (&this->os_video, &this->og_video);
    if (result==0)
      break;
    write (this->fh, this->og_video.header, this->og_video.header_len);
    write (this->fh, this->og_video.body, this->og_video.body_len);
  }

  close (this->fh);

  free (this);
}

enix_mux_t *create_ogm_mux (void) {

  ogm_t *this;

  this = malloc (sizeof (ogm_t)) ;

  this->mux.init           = ogm_init;
  this->mux.run            = ogm_run;
  this->mux.dispose        = ogm_dispose;
  this->mux.options        = enix_create_options ();

  this->mux.options->new_num_option (this->mux.options, "2pass", 0);

  this->fh            = -1;
  this->audio_cnt     = 0;
  this->video_cnt     = 0;
  this->audio_bytes   = 0;
  this->video_bytes   = 0;
  this->frame_num     = 0;

  return &this->mux;
}
