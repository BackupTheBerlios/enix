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
 * $Id: scaler.c,v 1.4 2004/07/30 14:00:10 dooh Exp $
 *
 * a very simple scaler, to be improved in the future
 * (quality and speed)
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "scaler.h"
#include "zoom.h"


typedef struct {

  enix_stream_t        stream;

  /* parameters set by user */
  int                  clp_top, clp_btm;
  int                  clp_lft, clp_rgt;
  enix_stream_t       *src_stream;

  int                  dst_width, dst_height;
  int                  dst_aspect;

  xine_video_frame_t   src_frame;

  uint8_t             *y_data, *u_data, *v_data;
  int                  y_size, uv_size;

  zoom_image_t         zoom_image_src_y;
  zoom_image_t         zoom_image_src_uv;
  zoom_image_t         zoom_image_dst_y;
  zoom_image_t         zoom_image_dst_uv;

  zoomer_t            *zoomer_y;
  zoomer_t            *zoomer_uv;

} scaler_t;

static int scaler_play (enix_stream_t *stream_gen, int start_pos, int start_time) {
  
  scaler_t *scaler = (scaler_t *) stream_gen;

  return scaler->src_stream->play (scaler->src_stream, start_pos, start_time);
}
  
static int scaler_get_next_video_frame (enix_stream_t *stream_gen,
					xine_video_frame_t *frame) {
  scaler_t           *scaler = (scaler_t *) stream_gen;
  int                 ret, src_y_size, src_uv_size;

  ret = scaler->src_stream->get_next_video_frame (scaler->src_stream, 
						  &scaler->src_frame);
  if (!ret)
    return ret;

  if (scaler->src_frame.colorspace != XINE_IMGFMT_YV12) {
    printf ("scaler: can only handle yuv 4:2:0 images.\n");
    abort ();
  }

  frame->width         = scaler->dst_width;
  frame->height        = scaler->dst_height;
  frame->vpts          = scaler->src_frame.vpts;
  frame->duration      = scaler->src_frame.duration;
  frame->colorspace    = XINE_IMGFMT_YV12;
  frame->aspect_ratio  = scaler->src_frame.aspect_ratio;
  frame->pos_stream    = scaler->src_frame.pos_stream;
  frame->pos_time      = scaler->src_frame.pos_time;
  frame->data          = scaler->y_data;

  src_y_size  = scaler->src_frame.width * scaler->src_frame.height;
  src_uv_size = src_y_size / 4;

  scaler->zoom_image_src_y.data = scaler->src_frame.data + (scaler->zoom_image_src_y.span*scaler->clp_top) + (scaler->clp_lft);
  zoom_image_process (scaler->zoomer_y);

  scaler->zoom_image_src_uv.data = scaler->src_frame.data + src_y_size + (((scaler->zoom_image_src_y.span*scaler->clp_top)/2 + (scaler->clp_lft))/2);
  scaler->zoom_image_dst_uv.data = scaler->u_data;
  zoom_image_process (scaler->zoomer_uv);

  scaler->zoom_image_src_uv.data = scaler->src_frame.data + src_y_size + src_uv_size + (((scaler->zoom_image_src_y.span*scaler->clp_top)/2 + (scaler->clp_lft))/2);
  scaler->zoom_image_dst_uv.data = scaler->v_data;
  zoom_image_process (scaler->zoomer_uv);

  return 1;
}

static int scaler_get_next_audio_frame (enix_stream_t *stream_gen,
					xine_audio_frame_t *frame) {
  scaler_t *scaler = (scaler_t *) stream_gen;

  return scaler->src_stream->get_next_audio_frame (scaler->src_stream, frame);
}
  
static void scaler_dispose (enix_stream_t *stream_gen) {

  scaler_t *scaler = (scaler_t *) stream_gen;

  scaler->src_stream->dispose (scaler->src_stream);

  if (scaler->y_data)
    free (scaler->y_data);

  free (scaler);

}

static int scaler_get_property (enix_stream_t *stream_gen, int property) {
  scaler_t *scaler = (scaler_t *) stream_gen;

  switch (property) {
  case ENIX_STREAM_PROP_WIDTH:
    return scaler->dst_width;
  case ENIX_STREAM_PROP_HEIGHT:
    return scaler->dst_height;
  case ENIX_STREAM_PROP_ASPECT:
    return scaler->dst_aspect;
  default:
    if (scaler->src_stream) 
      return scaler->src_stream->get_property (scaler->src_stream, property);
    printf ("scaler: error get_property(%d) but no stream set\n", property);
  }
  return 0;
}

enix_stream_t *enix_scaler_new (enix_stream_t *src, int dst_w, int mode ,int mod_y,
				int clp_top, int clp_btm, int clp_lft, int clp_rgt) {

  scaler_t *scaler;
  int src_ratio=src->get_property (src, ENIX_STREAM_PROP_ASPECT);
  int clp_w, clp_h , src_w , src_h , dst_h;
  double src_aspect,clp_aspect;

  scaler = malloc (sizeof (scaler_t));

  scaler->clp_top       = clp_top;
  scaler->clp_btm       = clp_btm;
  scaler->clp_rgt       = clp_rgt;
  scaler->clp_lft       = clp_lft;

  scaler->src_stream  = src;

  src_w = src->get_property (src, ENIX_STREAM_PROP_WIDTH);
  src_h = src->get_property (src, ENIX_STREAM_PROP_HEIGHT);
  if (src_ratio) {
    src_aspect = (double) src_ratio / 10000.0;
  } else src_aspect = src_w / src_h;
  printf ("scaler: src     is '%d'x'%d' with aspect %f\n",src_w, src_h, src_aspect);

  /*clipping*/
  clp_w = src_w - clp_lft - clp_rgt;
  clp_h = src_h - clp_top - clp_btm;
  clp_aspect = src_aspect * clp_w * src_h / clp_h / src_w;
  printf ("scaler: clipped is '%d'x'%d' with aspect %f\n",clp_w, clp_h, clp_aspect);

  /*zooming*/
  switch (mode) {
  case ENIX_SCALER_MODE_AR_SQUARE:
    dst_h = dst_w / clp_aspect ;
    break;
  case ENIX_SCALER_MODE_AR_KEEP:
    dst_h = clp_h * dst_w / clp_w;
    break;
  default:
    printf ("scaler: scaler_calc_y was called with an unknown AR_MODE '%d'.\n",mode);
    abort();
  }
  /*readjust y so that y mod mod_y == 0*/
  scaler->dst_aspect = 10000 * clp_aspect;
  scaler->dst_width  = dst_w;
  scaler->dst_height = mod_y * (int) ( 0.5 + ( dst_h / mod_y ));
  printf ("scaler: zoomed  is '%d'x'%d' with aspect %f\n", scaler->dst_width, scaler->dst_height, scaler->dst_aspect / 10000 );

  /* setup images */

  scaler->y_size   = scaler->dst_width * scaler->dst_height;
  scaler->uv_size  = scaler->dst_width/2 * scaler->dst_height/2;
  scaler->y_data   = malloc (scaler->y_size + 2*scaler->uv_size);
  scaler->u_data   = scaler->y_data + scaler->y_size;
  scaler->v_data   = scaler->y_data + scaler->y_size + scaler->uv_size;

  zoom_setup_image (&scaler->zoom_image_src_y, 
		    src_w-clp_lft-clp_rgt, src_h-clp_top-clp_btm, 
		    1, NULL);
  scaler->zoom_image_src_y.span=src_w;

  zoom_setup_image (&scaler->zoom_image_src_uv, 
		    (src_w-clp_lft-clp_rgt)/2, (src_h-clp_top-clp_btm)/2, 
		    1, NULL);
  scaler->zoom_image_src_uv.span=src_w/2;

  zoom_setup_image (&scaler->zoom_image_dst_y, 
		    scaler->dst_width, scaler->dst_height,
		    1, scaler->y_data);
  zoom_setup_image (&scaler->zoom_image_dst_uv, 
		    scaler->dst_width/2, scaler->dst_height/2, 
		    1, NULL);

  scaler->zoomer_y  = zoom_image_init (&scaler->zoom_image_dst_y, 
				       &scaler->zoom_image_src_y,
				       Lanczos3_filter, Lanczos3_support);
  
  scaler->zoomer_uv = zoom_image_init (&scaler->zoom_image_dst_uv, 
				       &scaler->zoom_image_src_uv,
				       Lanczos3_filter, Lanczos3_support);

  /* public interface */

  scaler->stream.play                 = scaler_play;
  scaler->stream.get_next_video_frame = scaler_get_next_video_frame;
  scaler->stream.get_next_audio_frame = scaler_get_next_audio_frame;
  scaler->stream.dispose              = scaler_dispose;
  scaler->stream.get_property         = scaler_get_property;

  return &scaler->stream;
}
