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
 * $Id: gtk_enix_film.h,v 1.1 2003/02/23 22:56:18 guenter Exp $
 *
 * gtk enix video film widget
 */

#ifndef HAVE_GTK_FILM_H
#define HAVE_GTK_FILM_H

#include <gtk/gtk.h>
#include "enix.h"

typedef struct gtk_enix_film_s gtk_enix_film_t;

gtk_enix_film_t *gtk_enix_film_new (void);

GtkWidget *gtk_enix_film_get_widget (gtk_enix_film_t *this);

void gtk_enix_film_set_stream (gtk_enix_film_t *this,
			       enix_stream_t *stream) ;

#endif

