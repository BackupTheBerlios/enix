AM_CFLAGS = @GTK2_CFLAGS@ @GTHREAD2_CFLAGS@ $(XINE_CFLAGS)

bin_PROGRAMS = enix_cut

enix_cut_SOURCES = cut.c gtk_enix_preview.c yuv2rgb.c gtk_enix_film.c utils.c

enix_cut_LDADD   = $(top_builddir)/src/enix/libenix.la \
		   @GTK2_LIBS@ @GTHREAD2_LIBS@

noinst_HEADERS = gtk_enix_film.h  gtk_enix_preview.h  utils.h  yuv2rgb.h

debug:
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS) @GTK2_CFLAGS@ @GTHREAD2_CFLAGS@"

install-debug: debug
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
