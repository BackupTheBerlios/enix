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
 * $Id: main.c,v 1.1 2003/02/23 22:56:30 guenter Exp $
 *
 * re-encoder main
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "enix.h"
#include "xine_stream.h"
#include "scaler.h"

#define DST_WIDTH  320
#define CLIP         0

void print_usage (void) {
  printf ("usage: enix_reen [ options ] inputfile\n\n");
  printf ("options:\n");
  printf ("  -h     show this info\n");
  printf ("  -A #   set audio bitrate [bits/sec]\n");
  printf ("  -V #   set video bitrate [bits/sec]\n");
  printf ("  -2     enable 2-pass encoding\n");
  printf ("  -b #   set number of b-frames\n");
  printf ("  -w #   set dest video width\n\n");
}

int main(int argc, char* argv[]) {

  enix_stream_t      *xine_stream, *stream;
  char               *infile;
  /* char                outfile[1024]; */
  enix_venc_t        *venc;
  enix_aenc_t        *aenc;
  enix_mux_t         *mux;

  int                 opt;

  int                 bitrate_audio;
  int                 bitrate_video;
  int                 twopass;
  int                 bframes;
  int                 width;

  /*
   * defaults
   */

  bitrate_audio = 64000;
  bitrate_video = 150000;
  twopass       = 0;
  bframes       = -1;
  width         = 320;

  /*
   * parse command line
   */

  while ((opt = getopt(argc, argv, "hA:V:2b:w:")) > 0) {
    switch (opt) {
    case 'h':
      print_usage ();
      exit (1);
      break;
    case '2':
      twopass = 1;
      break;
    case 'b':
      bframes = atoi (optarg);
      break;
    case 'V':
      bitrate_video = atoi (optarg);
      break;  
    case 'A':
      bitrate_audio = atoi (optarg);
      break;  
    case 'w':
      width = atoi (optarg);
      break;  
    default:
      print_usage ();
      exit (1);
    }
  }

  if (optind >= argc) {

    print_usage ();
    exit (1);
  }

  infile = argv[optind];

  /*
   * open an source stream chain
   */

  xine_stream   = enix_xine_stream_new (infile, 1, 1);
  stream        = enix_scaler_new (xine_stream, width, CLIP);

  /*
   * create an encoder
   */

  printf ("generating foo.ogm...\n");

  venc = create_ffmpeg_enc ();

  /* venc = create_xvid_enc (); */
  venc->options->set_num_option (venc->options, "bitrate", bitrate_video);
  venc->options->set_num_option (venc->options, "quality", 7);

  aenc = create_vorbis_enc ();
  aenc->options->set_num_option (aenc->options, "bitrate", bitrate_audio);
  
  mux = create_ogm_mux ();
  mux->options->set_num_option (mux->options, "2pass", twopass);

  mux->init (mux, "foo.ogm", venc, aenc, stream);

  mux->run (mux);

  printf ("done.\n");

  mux->dispose (mux);

  return 0;
}
