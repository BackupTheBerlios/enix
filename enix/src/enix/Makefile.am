SUBDIRS = libavcodec

AM_CFLAGS = $(XINE_CFLAGS)

noinst_LTLIBRARIES = libenix.la

libenix_la_SOURCES = scaler.c frame.c xine_stream.c options.c \
	 	     vcodec_xvid.c acodec_vorbis.c mux_ogm.c zoom.c \
		     vcodec_ffmpeg.c acodec_speex.c vcodec_theora.c

libenix_la_LDFLAGS = $(THREAD_LIBS) -avoid-version -module

libenix_la_LIBADD   = @XINE_LIBS@ $(VORBIS_LIBS) $(VORBISENC_LIBS) \
	  	      $(OGG_LIBS) -lxvidcore -lspeex -ltheora \
		      $(top_builddir)/src/enix/libavcodec/libavcodec.la


noinst_HEADERS = enix.h  scaler.h  xine_stream.h zoom.h

debug:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) || exit;\
	done;
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS) $(XINE_CFLAGS)"

install-debug: debug
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
