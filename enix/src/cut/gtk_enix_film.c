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
 * $Id: gtk_enix_film.c,v 1.3 2004/07/30 14:00:10 dooh Exp $
 *
 * gtk enix film stripe display widget
 */

#include <stdlib.h>
#include <stdio.h>

#include "gtk_enix_film.h"
#include "scaler.h"
#include "yuv2rgb.h"

/*
#define LOG
*/

#define DEFAULT_WIDTH  64
#define DEFAULT_HEIGHT 48

struct gtk_enix_film_s {

  GtkWidget          *w;

  GtkWidget          *video;
  GtkWidget          *scale;
  GtkAdjustment      *seeker;

  int                 width, height;
  int                 zoom, offset;
  uint8_t            *rgb_data;

  enix_stream_t      *stream;
};

static void get_frame (gtk_enix_film_t *this) {

  xine_video_frame_t frame;

  if (!this->stream) {
    printf ("gtk_enix_film: warning: get_frame called but no stream set\n");
    return;
  }

#ifdef LOG
  printf ("gtk_enix_film: get_next_video_frame\n");
#endif

  if (!this->stream->get_next_video_frame (this->stream, &frame)) {
    printf ("gtk_enix_film: error, didn't get a frame!\n");
    return;
  }

#ifdef LOG
  printf ("gtk_enix_film: yuv2rgb\n");
#endif

  if ((this->width != frame.width) || (this->height != frame.height)) {
    if (this->rgb_data)
      free (this->rgb_data);
    this->rgb_data  = malloc (frame.width * frame.height * 3);
    this->width = frame.width;
    this->height = frame.height;
  }

  yuv2rgb (frame.data, this->rgb_data, frame.width, frame.height);

#ifdef LOG
  printf ("gtk_enix_film: video_expose_cb\n");
#endif

}

static gboolean video_expose_cb (GtkWidget *widget, 
				 GdkEventExpose *event, 
				 gpointer data) {

  gtk_enix_film_t *this = (gtk_enix_film_t *) data;
  int              time, len;
  int              w,h,x;


  if (!this->stream)
    return TRUE;

  w = widget->allocation.width;
  h = widget->allocation.height;

  gdk_draw_rectangle (widget->window,
		      widget->style->black_gc,
		      TRUE,
		      0, 0, w, h);

  for (x=0; x<w; x+=15) {
    gdk_draw_rectangle (widget->window,
			widget->style->white_gc,
			TRUE,
			x+2, 2, 7, 5);
    gdk_draw_rectangle (widget->window,
			widget->style->white_gc,
			TRUE,
			x+2, h-7, 7, 5);
  }

  time = this->offset; 
  len = this->stream->get_property (this->stream, ENIX_STREAM_PROP_LENGTH);
  x = 0;
  while ((time<len) && (x<w)) {


    this->stream->play (this->stream, 0, time);

    get_frame (this);

    gdk_draw_rgb_image (widget->window, 
			widget->style->fg_gc[widget->state],
			x, 10, this->width, this->height,
			GDK_RGB_DITHER_NORMAL,
			this->rgb_data, 3*this->width);
    time+=this->zoom;
    x+=this->width;
  }

  return TRUE;
}



static void scale_cb (GtkWidget* widget, gpointer data){

  gtk_enix_film_t *this = (gtk_enix_film_t *) data;

  this->offset = (gint) GTK_ADJUSTMENT (this->seeker)->value;

  video_expose_cb (this->video, NULL, data);

}

GtkWidget *gtk_enix_film_get_widget (gtk_enix_film_t *this) {

  return this->w;
}

void gtk_enix_film_set_stream (gtk_enix_film_t *this,
			       enix_stream_t *stream) {

  this->stream = enix_scaler_new (stream, DEFAULT_WIDTH, ENIX_SCALER_MODE_AR_SQUARE, 1, 0, 0, 0, 0);
  this->seeker->upper = stream->get_property (stream, ENIX_STREAM_PROP_LENGTH);
  this->seeker->value = 0.0;

  gtk_adjustment_changed (this->seeker);
}

gtk_enix_film_t *gtk_enix_film_new (void) {

  gtk_enix_film_t *this;

  this = malloc (sizeof (gtk_enix_film_t));

  this->stream       = NULL;

  /*
   * actual film widget starts here, its this vbox
   */

  this->w = gtk_vbox_new (0,0);

  /*
   * video stripe display 
   */

  this->width    = 0;
  this->height   = 0;
  this->zoom     = 500;
  this->offset   = 0;
  this->rgb_data = NULL;

  this->video = gtk_drawing_area_new ();
  gtk_widget_set_size_request (this->video, DEFAULT_WIDTH, DEFAULT_HEIGHT+20);

  gtk_signal_connect (GTK_OBJECT (this->video), 
		      "expose_event", 
		      GTK_SIGNAL_FUNC (video_expose_cb),
		      this);

  gtk_box_pack_start (GTK_BOX (this->w), this->video, TRUE, TRUE, 5);
  
  /*
   * scroller
   */

  this->seeker = GTK_ADJUSTMENT(gtk_adjustment_new (0.0, 0.0, 600.0, 1.0, 10.0, 1.0));
  this->scale = gtk_hscale_new (GTK_ADJUSTMENT (this->seeker));
  gtk_scale_set_draw_value (GTK_SCALE (this->scale), FALSE);
  gtk_box_pack_end (GTK_BOX(this->w), this->scale, TRUE, FALSE, 2);

  g_signal_connect (GTK_OBJECT (this->scale), "value-changed",
		    G_CALLBACK (scale_cb), this );


  return this;

}

