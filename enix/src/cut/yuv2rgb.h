/*
 * Copyright (C) 2001 the viproc project
 * 
 * This file is part of viproc, a unix video player.
 * 
 * viproc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * viproc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: yuv2rgb.h,v 1.1 2003/02/23 22:56:27 guenter Exp $
 *
 * yuv2rgb, scaling
 */

#ifndef HAVE_YUV2RGB_H
#define HAVE_YUV2RGB_H

#include <glib.h>

void yuv2rgb (guchar *yuv, guchar *rgb,
	      gint width, gint height);

void yuv2rgb_init () ;

#endif
