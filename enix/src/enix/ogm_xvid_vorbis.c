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
 * $Id: ogm_xvid_vorbis.c,v 1.1 2003/02/23 22:45:22 guenter Exp $
 *
 * sample vorbis+xvid in ogg encoder
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
#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>

#include "ogm_xvid_vorbis.h"

/*
#define LOG
*/

/* according to http://tobias.everwicked.com/packfmt.htm */

#define PACKET_IS_SYNCPOINT      0x08
#define HINT_FILE                "/tmp/hints.mv"

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

typedef struct {

  encoder_t        encoder;

  int              fh;

  void            *xvid_handle;
  uint8_t         *xvid_buffer;
  uint8_t         *frame_buffer;
  int              width, height;
  int              frame_num;

  vorbis_info      vi; 
  vorbis_comment   vc; 
  vorbis_dsp_state vd; 
  vorbis_block     vb; 

  ogg_stream_state os_video, os_audio; 
  ogg_page         og_video, og_audio; 

  int              video_cnt;
  int              audio_cnt;

  int              quality;

  char             hints_buffer[1024*1024];
  FILE            *hints_file;
} oxv_t;


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
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | /* Q 6 */
	PMV_USESQUARES16 | PMV_EARLYSTOP8 | PMV_HALFPELREFINE8,
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EXTSEARCH16 | /* Q 6 */
	PMV_USESQUARES16 | PMV_EARLYSTOP8 | PMV_HALFPELREFINE8
};

static int const general_presets[7] = {
	XVID_H263QUANT,	                              /* Q 0 */
	XVID_MPEGQUANT,                               /* Q 1 */
	XVID_H263QUANT,                               /* Q 2 */
	XVID_H263QUANT | XVID_HALFPEL,                /* Q 3 */
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V, /* Q 4 */
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V, /* Q 5 */
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V  /* Q 6 */
};
		

static void oxv_open (encoder_t *this_gen, char *filename) {

  oxv_t             *this = (oxv_t *) this_gen;

  /*
   * open output file
   */
  this->fh = open (filename, O_CREAT|O_TRUNC|O_WRONLY, 0644);

  if (this->fh<0) {
    printf ("oxv: failed to create file '%s'\n", filename);
    abort ();
  }

  this->xvid_handle = 0;
}

static void init_encoder (oxv_t *this, xine_video_frame_t *frame) {

  XVID_INIT_PARAM    xinit;
  XVID_ENC_PARAM     xparam;
  int                xerr, ret;
  dsogg_header_t     dsoggh;
  ogg_packet         oph, opc; 
  int                width, height, frame_duration;

  /*
   * xvid init
   */

  xinit.cpu_flags = 0;

  xvid_init (NULL, 0, &xinit, NULL);

#ifdef LOG
  printf ("xvid: api version is 0x%08x\n", xinit.api_version);
#endif

  width          = frame->width;
  height         = frame->height;
  frame_duration = frame->duration;

#ifdef LOG
  printf ("xvid: width=%02d, height=%02d\n", width, height);
#endif

  this->xvid_buffer  = malloc (width * height * 2);
  this->frame_buffer = malloc (width * height * 2);
  this->width        = width;
  this->height       = height;

  xparam.width       = width;
  xparam.height      = height;

  xparam.fbase                    = 90000;
  xparam.fincr                    = frame_duration;

  xparam.rc_reaction_delay_factor = 16;
  xparam.rc_averaging_period      = 100;
  xparam.rc_buffer                = 10;
  xparam.rc_bitrate               = 120000;
  xparam.min_quantizer            = 1;
  xparam.max_quantizer            = 31;
  xparam.max_key_interval         = 250;
  xparam.bquant_ratio             = 120;
  xparam.bquant_offset            = 0;
  xparam.max_bframes              = 0;
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

  /*
   * create ogg stream
   */

  srand (time (NULL));
  ogg_stream_init (&this->os_video, rand ());

  this->video_cnt = 0;
  this->audio_cnt = 0;

  this->frame_num = 0;
  
  /* headers */

  this->xvid_buffer[0] = 0x01;

  dsoggh.streamtype[0]    = 'v';
  dsoggh.streamtype[1]    = 'i';
  dsoggh.streamtype[2]    = 'd';
  dsoggh.streamtype[3]    = 'e';
  dsoggh.streamtype[4]    = 'o';
  dsoggh.streamtype[5]    = ' ';
  dsoggh.streamtype[6]    = ' ';
  dsoggh.streamtype[7]    = ' ';
  dsoggh.subtype[0]       = 'X';
  dsoggh.subtype[1]       = 'V';
  dsoggh.subtype[2]       = 'I';
  dsoggh.subtype[3]       = 'D';
  dsoggh.size             = sizeof (dsogg_header_t);
  dsoggh.time_unit        = frame_duration * 1000 / 9;
  dsoggh.samples_per_unit = 1;
  dsoggh.default_len      = 0; /* ? */
  dsoggh.buffersize       = 2048000;
  dsoggh.bits_per_sample  = 0;
  dsoggh.hubba.video.width      = width;
  dsoggh.hubba.video.height     = height;

  memcpy (&this->xvid_buffer[1], &dsoggh, sizeof (dsogg_header_t));

  oph.packet     = this->xvid_buffer;
  oph.bytes      = sizeof (dsogg_header_t)+1;
  oph.b_o_s      = 1;
  oph.e_o_s      = 0;
  oph.granulepos = 0;
  oph.packetno   = this->video_cnt++;
  opc.packet     = "\003enix generated";
  opc.bytes      = 15;
  opc.b_o_s      = 0;
  opc.e_o_s      = 0;
  opc.granulepos = 0;
  opc.packetno   = this->video_cnt++;

  ogg_stream_packetin (&this->os_video, &oph);
  ogg_stream_packetin (&this->os_video, &opc);

  while (1) {
    int result = ogg_stream_flush (&this->os_video, &this->og_video);
    if (result==0)
      break;
    write (this->fh, this->og_video.header, this->og_video.header_len);
    write (this->fh, this->og_video.body, this->og_video.body_len);
  }


  /*
   * vorbis init
   */

  vorbis_info_init (&this->vi);

  /* ret=vorbis_encode_init_vbr (&this->vi, 2, 44100, .5); */
  /*
  ret = vorbis_encode_init (&this->vi, 2, 44100,
			    256000, 128000, 32000);
  */
  ret = vorbis_encode_init (&this->vi, 2, 44100,
			    64000, 50000, 24000);
  
  if (ret) {
    printf ("oxv: help, vorbis_encode_init failed.\n");
    exit(1);
  }

  /* add a comment */
  vorbis_comment_init (&this->vc);
  vorbis_comment_add_tag (&this->vc, "ENCODER", "xine|enix");

  /* set up the analysis state and auxiliary encoding storage */
  vorbis_analysis_init (&this->vd, &this->vi);
  vorbis_block_init (&this->vd, &this->vb);

  ogg_stream_init (&this->os_audio, rand());

  /* Vorbis streams begin with three headers; the initial header (with
     most of the codec setup parameters) which is mandated by the Ogg
     bitstream spec.  The second header holds any comment fields.  The
     third header holds the bitstream codebook.  We merely need to
     make the headers, then pass them to libvorbis one at a time;
     libvorbis handles the additional Ogg bitstream constraints */

  {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_headerout (&this->vd, &this->vc, &header, &header_comm, &header_code);
    ogg_stream_packetin (&this->os_audio, &header); /* automatically placed in its own
						       page */
    ogg_stream_packetin (&this->os_audio, &header_comm);
    ogg_stream_packetin (&this->os_audio, &header_code);

    /* This ensures the actual
     * audio data will start on a new page, as per spec
     */

    while (1) {
      int result=ogg_stream_flush (&this->os_audio, &this->og_audio);
      if (result==0)
	break;
      write (this->fh, this->og_audio.header, this->og_audio.header_len);
      write (this->fh, this->og_audio.body, this->og_audio.body_len);
    }
  }
  
}

#ifdef LOG

static void dump_frame (uint8_t *data, int w, int h) {

  int x, y;

  y = 0;
  while (y<h) {

    x = 0;

    while (x<w) {

      int pixel;

      pixel = *(data + y*w + x);

      if (pixel>100)
        printf ("*");
      else if (pixel>10)
        printf (".");
      else
        printf (" ");

      x+=15;

    }

    y+=15;
    printf ("\n");
  }

}
#endif

static void oxv_encode_video_frame (oxv_t *this, xine_video_frame_t *frame,
				    int pass) {

  XVID_ENC_FRAME     xframe;
  XVID_ENC_STATS     xstats;
  ogg_packet         op; 
  int                xerr;


#ifdef LOG
  printf ("xvid: encoding frame #%d (%dx%d)\n", 
	  this->frame_num, frame->width, frame->height);
#endif

  if (!this->xvid_handle) {
    init_encoder (this, frame);
  }

  if (!this->xvid_handle) 
    return;

  xframe.bitstream = this->xvid_buffer+1;
  xframe.length    = -1;
  xframe.image     = frame->data;

#if 0
  dump_frame (frame->data, frame->width, frame->height); 
#endif

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
  
  if (pass==1) {
    int hints_size;

    fread (&hints_size, 1, sizeof (long), this->hints_file);
    fread (this->hints_buffer, 1, hints_size, this->hints_file);
    xframe.hint.hintlength = hints_size;
    xframe.hint.rawhints   = 0;
    xframe.general |= XVID_HINTEDME_SET;
  }

  if (pass==0) {
    xframe.hint.rawhints = 0;
    xframe.general |= XVID_HINTEDME_GET;
  }

  xerr = xvid_encore (this->xvid_handle, XVID_ENC_ENCODE, &xframe, &xstats);

#ifdef LOG
  printf ("xvid: encore...done (%d bytes, err=%d, intra=%d)\n", 
	  xframe.length, xerr, xframe.intra);
#endif

  if (pass == 0) {
    int hints_size = xframe.hint.hintlength;

    fwrite (&hints_size, 1, sizeof(long), this->hints_file);
    fwrite (this->hints_buffer, 1, hints_size, this->hints_file);

  } else {

    op.packet      = this->xvid_buffer;
    if (xframe.intra)
      *op.packet = PACKET_IS_SYNCPOINT;
    else
      *op.packet = 0;
    op.bytes       = xframe.length+1;
    op.b_o_s       = 0;
    op.e_o_s       = 0;
    op.granulepos  = this->frame_num++;
    op.packetno    = this->video_cnt++;
    
    /* weld the packet into the bitstream */
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

static void oxv_encode_audio_frame (oxv_t *this, xine_audio_frame_t *frame) {

  int            bytes;
  int            i;
  ogg_packet     op;
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
  
  /* vorbis does some data preanalysis, then divvies up blocks for
     more involved (potentially parallel) processing.  Get a single
     block for encoding now */

  while (vorbis_analysis_blockout(&this->vd,&this->vb)==1){

    /* analysis, assume we want to use bitrate management */
    vorbis_analysis(&this->vb,NULL);
    vorbis_bitrate_addblock(&this->vb);
    
    while(vorbis_bitrate_flushpacket(&this->vd,&op)){

      op.packetno = this->audio_cnt++;

#ifdef LOG
      printf ("ogm_xvid_vorbis: added audio package, %ld bytes\n", op.bytes);
      printf ("ogm_xvid_vorbis: granulepos %lld, packetno %lld, audio_cnt=%d\n",
	      op.granulepos, op.packetno, this->audio_cnt);
#endif      

      /* op.granulepos = -1; */

      /* weld the packet into the bitstream */
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
}

static void show_progress (int pos, int length) {
  int stars, i;

  stars = pos * 70 / length;
  
  printf ("\r[");
  for (i=0; i<70; i++) {
    if (i<stars)
      printf ("*");
    else
      printf (" ");
  }
  printf ("] %3d%%", pos*100/length);

  fflush (stdout);
}

static void oxv_run (encoder_t *this_gen, enix_stream_t *stream) {

  oxv_t         *this = (oxv_t *) this_gen;
  int64_t        audio_pts, video_pts;
  int            cnt;
  int            length;
  int            pass;

  for (pass = 0; pass<2; pass++) {

    /* hints file */

    char *rights = "rb";

    if (pass==0)
      rights = "w+b";
    
    /* open the hint file */
    this->hints_file = fopen (HINT_FILE, rights);
    if (this->hints_file == NULL) {
      printf ("oxv: error opening hints file %s\n", HINT_FILE);
      exit (1);
    }
    
    stream->play (stream, 0, 0);
    
    audio_pts = 0;
    video_pts = 0;
    cnt       = 0;
    length    = stream->get_property (stream, ENIX_STREAM_PROP_LENGTH);
    
    printf ("oxv: pass # %d\n", pass);

    show_progress (0, length);
    
    while (1) {
      
#ifdef LOG
      printf ("oxv: audio_pts: %lld, video_pts: %lld\n", 
	      audio_pts, video_pts);
#endif
      
      if (audio_pts<video_pts) {
	
	xine_audio_frame_t frame;
	
	/* printf ("get next audio frame...\n"); */
	
	if (!stream->get_next_audio_frame (stream, &frame)) 
	  break;

	audio_pts = frame.vpts;

	if (cnt>10) {
	  show_progress (frame.pos_time, length);
	  cnt = 0;
	}
	  
	cnt ++;

	if (pass!=0) {

#ifdef LOG
	  printf ("oxv: got audio frame pts %lld\n", frame.vpts);
#endif
	  oxv_encode_audio_frame (this, &frame);
	}
      } else {
	
	xine_video_frame_t frame;
	
	/* printf ("get next video frame...\n"); */
	
	if (!stream->get_next_video_frame (stream, &frame)) 
	  break;
	
	video_pts = frame.vpts;
	
#ifdef LOG
	printf ("oxv: got video frame %dx%d, pts %lld\n", 
		frame.width, frame.height, frame.vpts);
#endif
	
	oxv_encode_video_frame (this, &frame, pass);
      }
    }
    fclose (this->hints_file);
  }
}

static void oxv_set_option (encoder_t *this_gen, const char *option, 
			    const char *value) {

  oxv_t *this = (oxv_t *) this_gen;

  if (!strcmp (option, "quality")) {

    sscanf (value, "%d", &this->quality);

    if (this->quality<0)
      this->quality = 0;
    if (this->quality>6)
      this->quality = 6;

    return;
  }

  printf ("oxv: set_option %s -> %s not implemented\n",
	  option, value);
}

static void oxv_close (encoder_t *this_gen) {

  oxv_t             *this = (oxv_t *) this_gen;
  int                xerr;

  /* 
   * flush audio and video
   */

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

  xerr = xvid_encore (this->xvid_handle, XVID_ENC_DESTROY, NULL, NULL);

  free (this->xvid_buffer);
  close (this->fh);
}

encoder_t *create_ogm_xvid_vorbis_encoder (void) {

  oxv_t *this;

  this = malloc (sizeof (oxv_t)) ;

  this->encoder.open                = oxv_open;
  this->encoder.run                 = oxv_run;
  this->encoder.set_option          = oxv_set_option;
  this->encoder.close               = oxv_close;

  this->quality = 6;

  return &this->encoder;
}
