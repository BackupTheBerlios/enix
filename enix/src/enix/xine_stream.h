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
 * $Id: xine_stream.h,v 1.1 2003/02/23 22:45:12 guenter Exp $
 *
 * wrapper around xine engine streams providing the enix_stream_t interface
 */

#ifndef HAVE_XINE_STREAM_H
#define HAVE_XINE_STREAM_H

#include "enix.h"

/*
 * open stream <mrl> using the xine engine and
 * build an enix wrapper around it
 *
 * want_audio: set up an audio port so this
 *             stream can and must be used to retrieve
 *             decoded audio samples
 *
 * want_video: set up an video port so this
 *             stream can and must be used to retrieve
 *             decoded video frames
 *
 * note: if want_audio/want_video is set,
 *       audio/video frames must be retrieved, otherwise
 *       the xine engine will get stuck
 */

enix_stream_t *enix_xine_stream_new (char *mrl,
				     int want_audio,
				     int want_video);

#endif
