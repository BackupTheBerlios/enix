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
 * $Id: frame.c,v 1.1 2003/02/23 22:45:09 guenter Exp $
 *
 * frame.c: frame allocation
 */

#include <stdlib.h>

#include "enix.h"

xine_video_frame_t *enix_video_frame_new (int w, int h) {

  xine_video_frame_t *frame;

  frame = malloc (sizeof (xine_video_frame_t));

  frame->vpts         = 0;
  frame->duration     = 0;
  
  frame->width        = w;
  frame->height       = h;
  frame->colorspace   = XINE_IMGFMT_YV12;
  frame->aspect_ratio = 1.0;

  frame->pos_stream   = 0;
  frame->pos_time     = 0;

  frame->data         = malloc (w*h*3/2);
  frame->xine_frame   = NULL;

  return frame;

}

