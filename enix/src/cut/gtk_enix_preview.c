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
 * $Id: gtk_enix_preview.c,v 1.3 2004/07/30 14:00:10 dooh Exp $
 *
 * gtk enix video preview widget
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "gtk_enix_preview.h"
#include "scaler.h"
#include "yuv2rgb.h"
#include "utils.h"

/*
#define LOG
*/


#if 0
#define DEFAULT_WIDTH  480
#define DEFAULT_HEIGHT 360
#endif

#define DEFAULT_WIDTH  320
#define DEFAULT_HEIGHT 240

struct gtk_enix_preview_s {

  GtkWidget          *w;

  GtkWidget          *video;
  GtkWidget          *scale;
  GtkAdjustment      *seeker;
  GtkWidget          *time_label;

  int                 width, height;
  uint8_t            *rgb_data;

  enix_stream_t      *stream;
};

static gboolean video_expose_cb (GtkWidget *widget, 
				 GdkEventExpose *event, 
				 gpointer data) {

  gtk_enix_preview_t *this = (gtk_enix_preview_t *) data;

  gdk_draw_rgb_image (widget->window, 
		      widget->style->fg_gc[widget->state],
		      0, 0, this->width, this->height,
		      GDK_RGB_DITHER_NORMAL,
		      this->rgb_data, 3*this->width);

  return TRUE;
}

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


static void get_frame (gtk_enix_preview_t *this) {

  xine_video_frame_t frame;

  if (!this->stream) {
    printf ("gtk_enix_preview: warning: get_frame called but no stream set\n");
    return;
  }

#ifdef LOG
  printf ("gtk_enix_preview: get_next_video_frame\n");
#endif

  if (!this->stream->get_next_video_frame (this->stream, &frame)) {
    printf ("gtk_enix_preview: error, didn't get a frame!\n");
    return;
  }

#ifdef LOG
  printf ("gtk_enix_preview: yuv2rgb\n");
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
  printf ("gtk_enix_preview: video_expose_cb\n");
#endif

  video_expose_cb (this->video, NULL, this);
}

static void goto_time (gtk_enix_preview_t *this, int pos_time) {

  char str[256];

  this->stream->play (this->stream, 0, pos_time);

  int_to_timestring (pos_time, str, 256);
  gtk_label_set_text (GTK_LABEL (this->time_label), str);

  get_frame (this);
}

static void scale_cb (GtkWidget* widget, gpointer data){

  gtk_enix_preview_t *this = (gtk_enix_preview_t *) data;

  int pos = (gint) this->seeker->value;

  goto_time (this, pos);
}

static void go_start_cb (GtkWidget* widget, gpointer data){

  gtk_enix_preview_t *this = (gtk_enix_preview_t *) data;

  goto_time (this, 0);
  this->seeker->upper = this->stream->get_property (this->stream, 
						    ENIX_STREAM_PROP_LENGTH);
  this->seeker->value = 0.0;

  gtk_adjustment_changed (this->seeker);
}

static void go_back_cb (GtkWidget* widget, gpointer data){

  gtk_enix_preview_t *this = (gtk_enix_preview_t *) data;
  int                 pos_time;

  pos_time = this->seeker->value - 10000.0;
  if (pos_time<0.9)
    pos_time=0.0;

  goto_time (this, pos_time);
  this->seeker->upper = this->stream->get_property (this->stream, 
						    ENIX_STREAM_PROP_LENGTH);
  this->seeker->value = pos_time;

  gtk_adjustment_changed (this->seeker);
}

static void step_back_cb (GtkWidget* widget, gpointer data){

  gtk_enix_preview_t *this = (gtk_enix_preview_t *) data;
  int                 pos_time;

  pos_time = this->seeker->value - 1000.0/25;
  if (pos_time<0.9)
    pos_time=0.0;

  goto_time (this, pos_time);
  this->seeker->upper = this->stream->get_property (this->stream, 
						    ENIX_STREAM_PROP_LENGTH);
  this->seeker->value = pos_time;

  gtk_adjustment_changed (this->seeker);
}

static void step_cb (GtkWidget* widget, gpointer data){

  gtk_enix_preview_t *this = (gtk_enix_preview_t *) data;
  int                 pos_time;

  pos_time = this->seeker->value + 1000.0/25;
  this->seeker->upper = this->stream->get_property (this->stream, 
						    ENIX_STREAM_PROP_LENGTH);
  if (pos_time>this->seeker->upper)
    pos_time=this->seeker->upper;

  goto_time (this, pos_time);
  this->seeker->value = pos_time;

  gtk_adjustment_changed (this->seeker);
}

static void go_forward_cb (GtkWidget* widget, gpointer data){

  gtk_enix_preview_t *this = (gtk_enix_preview_t *) data;
  int                 pos_time;

  pos_time = this->seeker->value + 10000.0;
  this->seeker->upper = this->stream->get_property (this->stream, 
						    ENIX_STREAM_PROP_LENGTH);
  if (pos_time>this->seeker->upper)
    pos_time=this->seeker->upper;

  goto_time (this, pos_time);
  this->seeker->value = pos_time;

  gtk_adjustment_changed (this->seeker);
}

static void go_end_cb (GtkWidget* widget, gpointer data){

  gtk_enix_preview_t *this = (gtk_enix_preview_t *) data;
  int                 pos_time;

  this->seeker->upper = this->stream->get_property (this->stream, 
						    ENIX_STREAM_PROP_LENGTH);
  pos_time = this->seeker->upper;
  goto_time (this, pos_time);
  this->seeker->value = pos_time;

  gtk_adjustment_changed (this->seeker);
}

GtkWidget *gtk_enix_preview_get_widget (gtk_enix_preview_t *this) {

  return this->w;
}

void gtk_enix_preview_set_stream (gtk_enix_preview_t *this,
				  enix_stream_t *stream) {

  this->stream = enix_scaler_new (stream, DEFAULT_WIDTH, ENIX_SCALER_MODE_AR_SQUARE, 1, 0, 0, 0, 0);

  stream->play (stream, 0, 0);

  get_frame (this);

  this->seeker->upper = stream->get_property (stream, ENIX_STREAM_PROP_LENGTH);
  this->seeker->value = 0.0;

  gtk_adjustment_changed (this->seeker);
}

gtk_enix_preview_t *gtk_enix_preview_new (void) {

  gtk_enix_preview_t *this;
  GtkWidget          *hbox, *button;

  this = malloc (sizeof (gtk_enix_preview_t));

  this->stream       = NULL;

  /*
   * actual preview widget starts here, its this vbox
   */

  this->w = gtk_vbox_new (0,0);

  /*
   * video frame display 
   */

  this->width    = 0;
  this->height   = 0;
  this->rgb_data = NULL;

  this->video = gtk_drawing_area_new ();
  gtk_widget_set_size_request (this->video, DEFAULT_WIDTH, DEFAULT_HEIGHT);

  gtk_signal_connect (GTK_OBJECT (this->video), 
		      "expose_event", 
		      GTK_SIGNAL_FUNC (video_expose_cb),
		      this);

  gtk_box_pack_start (GTK_BOX (this->w), this->video, TRUE, FALSE, 5);
  
  yuv2rgb_init () ;


  /*
   * button bar
   */

  hbox = gtk_hbox_new (0,2);
  
  button = gtk_button_new_with_label ("|<");
  g_signal_connect (GTK_OBJECT(button), "clicked", 
		    G_CALLBACK(go_start_cb), 
		    this);
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 2);
  button = gtk_button_new_with_label ("<<");
  g_signal_connect (GTK_OBJECT(button), "clicked", 
		    G_CALLBACK(go_back_cb), 
		    this);
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 2);
  button = gtk_button_new_with_label ("<");
  g_signal_connect (GTK_OBJECT(button), "clicked", 
		    G_CALLBACK(step_back_cb), 
		    this);
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 2);

  this->seeker = gtk_adjustment_new (0.0, 0.0, 65536.0, 1.0, 10.0, 1.0);
  this->scale = gtk_hscale_new (GTK_ADJUSTMENT (this->seeker));
  gtk_box_pack_start (GTK_BOX(hbox), this->scale, TRUE, TRUE, 2);

  gtk_scale_set_draw_value (GTK_SCALE (this->scale), FALSE);
  g_signal_connect (GTK_OBJECT (this->scale), "value-changed",
		    G_CALLBACK (scale_cb), this );

  this->time_label = gtk_label_new ("00:00:00:00");
  gtk_box_pack_start (GTK_BOX(hbox), this->time_label, FALSE, FALSE, 2);

  button = gtk_button_new_with_label (">");
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 2);
  g_signal_connect (GTK_OBJECT(button), "clicked", 
		    G_CALLBACK(step_cb), 
		    this);
  button = gtk_button_new_with_label (">>");
  g_signal_connect (GTK_OBJECT(button), "clicked", 
		    G_CALLBACK(go_forward_cb), 
		    this);
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 2);
  button = gtk_button_new_with_label (">|");
  g_signal_connect (GTK_OBJECT(button), "clicked", 
		    G_CALLBACK(go_end_cb), 
		    this);
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (this->w), hbox, FALSE, FALSE, 2);


  return this;

}

