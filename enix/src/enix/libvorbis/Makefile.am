## Process this file with automake to produce Makefile.in

SUBDIRS = modes books vorbis

noinst_LTLIBRARIES = libvorbis.la

libvorbis_la_SOURCES = mdct.c smallft.c block.c envelope.c window.c lsp.c \
			lpc.c analysis.c synthesis.c psy.c info.c \
			floor1.c floor0.c\
			res0.c mapping0.c registry.c codebook.c sharedbook.c\
			lookup.c bitrate.c vorbisenc.c vorbisfile.c

noinst_HEADERS = envelope.h lpc.h lsp.h codebook.h misc.h psy.h\
		 masking.h os.h mdct.h smallft.h highlevel.h\
		 registry.h scales.h window.h lookup.h lookup_data.h\
		 codec_internal.h backends.h bitrate.h 


debug:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) || exit;\
	done;
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

install-debug: debug
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in

