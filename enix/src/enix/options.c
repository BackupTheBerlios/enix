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
 * $Id: options.c,v 1.1 2003/02/23 22:45:36 guenter Exp $
 *
 * options handling
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "enix.h"

/*
#define LOG
*/

static enix_option_t *options_find_option (enix_options_t *this, char *id) {

  enix_option_t *o;

  o = this->options;
  while (o) {

#ifdef LOG
    printf ("options: find %s =? %s\n", o->id, id);
#endif

    if (!strcmp (o->id, id))
      break;

    o = o->next;
  }
  
  return o;
}

static void options_new_num_option (enix_options_t *this, char *id, int def_value) {

  enix_option_t *o;

#ifdef LOG
  printf ("options: new option %s\n", id);
#endif

  o = options_find_option (this, id);

  if (!o) {

    o = malloc (sizeof (enix_option_t));

    o->next = this->options;
    this->options = o;

    o->id        = strdup (id);
    o->num_value = def_value;
  } else {
    printf ("options: error, option '%s' registered more than once\n",
	    id);
    abort ();
  }
}

static void options_set_num_option (enix_options_t *this, char *id, int value) {

  enix_option_t *o;

#ifdef LOG
  printf ("options: set option %s to %d\n", id, value);
#endif

  o = options_find_option (this, id);

  if (!o) {
    printf ("options: error, trying to set unknown option '%s'\n", id);
    abort ();
  }

  o->num_value = value;
}

static int  options_get_num_option (enix_options_t *this, char *id) {

  enix_option_t *o;

  o = options_find_option (this, id);

  if (!o) {
    printf ("options: error, option '%s' not registered\n",
	    id);
    abort ();
  }

  printf ("options: '%s' => %d\n", id, o->num_value);

  return o->num_value;
}

static void options_dispose (enix_options_t *this) {

  enix_option_t *o;

  o = this->options;
  while (o) {
    enix_option_t *n;

    n = o->next;
    free (o);
    o = n;
  }

  free (this);
}

enix_options_t *enix_create_options (void) {

  enix_options_t *this;

  this = malloc (sizeof (enix_options_t));

  this->new_num_option = options_new_num_option;
  this->get_num_option = options_get_num_option;
  this->set_num_option = options_set_num_option;
  this->dispose        = options_dispose;

  this->options = NULL;

  return this;
}
