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
 * $Id: cut.c,v 1.1 2003/02/23 22:56:15 guenter Exp $
 *
 * nonlinear video editor - enix style test application
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "enix.h"
#include "gtk_enix_preview.h"
#include "gtk_enix_film.h"
#include "scaler.h"
#include "xine_stream.h"

#define DST_WIDTH  600
#define CLIP         0

int main(int argc, char* argv[]) {

  gtk_enix_preview_t *preview;
  gtk_enix_film_t    *film;
  enix_stream_t      *stream;
  GtkWidget          *win;
  char               *infile;
  GtkWidget          *hpane, *vpane;
  GtkWidget          *bbox, *vbox, *button;
  GtkWidget          *scrolled_window, *tree;
  GtkListStore       *clips_store;
  GtkCellRenderer    *cell;
  GtkTreeViewColumn  *column;

  /*
   * init glib/gdk/gtk thread safe/aware 
   */

  g_thread_init (NULL);
  gdk_threads_init ();
  gtk_init(&argc, &argv);

  /*
   * parse command line
   */

  if (argc != 2) {

    printf ("usage: enix <stream>\n");
    exit (1);
  }

  infile = argv[1];

  /*
   * main window
   */

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (win), "enix::cut!");
#if 0
  g_signal_connect( GTK_OBJECT (win), "delete_event",
		      G_CALLBACK (close_cb), win );
#endif

  vpane = gtk_vpaned_new();

  hpane = gtk_hpaned_new();

  /* preview widget + buttons */

  vbox = gtk_vbox_new (0,0);

  preview = gtk_enix_preview_new ();
  gtk_box_pack_start (GTK_BOX (vbox), gtk_enix_preview_get_widget (preview), 
		      TRUE, FALSE, 5);

  bbox = gtk_hbox_new (0,0);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 2);

  button = gtk_button_new_with_label ("clip start");
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, FALSE, 2);
  button = gtk_button_new_with_label ("clip end");
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, FALSE, 2);
  button = gtk_button_new_with_label ("take");
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, FALSE, 2);  

  gtk_paned_add1 (GTK_PANED (hpane), vbox);

  /*
   * tree list for clips
   */

  clips_store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(clips_store));  

  cell = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Clip",
						     cell,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree),
			       GTK_TREE_VIEW_COLUMN (column));

  column = gtk_tree_view_column_new_with_attributes ("Length",
						     cell,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree),
			       GTK_TREE_VIEW_COLUMN (column));
  
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_window, 180, 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), 
					 tree);

  gtk_paned_add2 (GTK_PANED (hpane), scrolled_window);

  gtk_paned_add1 (GTK_PANED (vpane), hpane);

  film = gtk_enix_film_new ();

  gtk_paned_add2 (GTK_PANED (vpane), gtk_enix_film_get_widget (film));

  gtk_container_add (GTK_CONTAINER (win), vpane);
  gtk_widget_show_all (win);

  stream = enix_xine_stream_new (infile, 0, 1);
  gtk_enix_preview_set_stream (preview, stream);

  stream = enix_xine_stream_new (infile, 0, 1);
  gtk_enix_film_set_stream (film, stream);

  gdk_threads_enter();  
  gtk_main();
  gdk_threads_leave();  

  return 0;
}
