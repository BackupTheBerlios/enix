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
 * $Id: ogm_xvid_vorbis.h,v 1.1 2003/02/23 22:45:22 guenter Exp $
 *
 * encoder for:
 *
 * system layer OGM
 *
 * video: xvid
 * audio: vorbis
 *
 */

#ifndef HAVE_OGM_XVID_VORBIS_H
#define HAVE_OGM_XVID_VORBIS_H

#include "enix.h"

encoder_t *create_ogm_xvid_vorbis_encoder (void);

#endif
