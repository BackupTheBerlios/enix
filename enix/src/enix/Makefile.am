SUBDIRS = libavcodec libogg libvorbis libtheora libspeex

AM_CFLAGS = $(XINE_CFLAGS)

noinst_LTLIBRARIES = libenix.la

libenix_la_SOURCES = scaler.c frame.c xine_stream.c options.c \
	 	     acodec_vorbis.c mux_ogm.c zoom.c \
		     vcodec_ffmpeg.c acodec_speex.c vcodec_theora.c

#vcodec_xvid.c

libenix_la_LDFLAGS = $(THREAD_LIBS) -avoid-version -module

libenix_la_LIBADD   = @XINE_LIBS@ \
		      $(top_builddir)/src/enix/libavcodec/libavcodec.la \
		      $(top_builddir)/src/enix/libogg/libogg.la \
		      $(top_builddir)/src/enix/libvorbis/libvorbis.la \
		      $(top_builddir)/src/enix/libtheora/libtheora.la \
		      $(top_builddir)/src/enix/libspeex/libspeex.la

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
