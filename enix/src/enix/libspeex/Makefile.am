## Process this file with automake to produce Makefile.in. -*-Makefile-*-

noinst_LTLIBRARIES = libspeex.la

# Sources for compilation in the library
libspeex_la_SOURCES = nb_celp.c \
	sb_celp.c \
	lpc.c \
	ltp.c \
	lsp.c \
	quant_lsp.c \
	lsp_tables_nb.c \
	gain_table.c \
	gain_table_lbr.c \
	cb_search.c \
	filters.c \
	bits.c \
	modes.c \
	vq.c \
	high_lsp_tables.c \
	vbr.c \
	hexc_table.c \
	exc_5_256_table.c \
	exc_5_64_table.c \
	exc_8_128_table.c \
	exc_10_32_table.c \
	exc_10_16_table.c \
	exc_20_32_table.c \
	hexc_10_32_table.c \
	misc.c \
	speex_header.c \
	speex_callbacks.c \
	math_approx.c \
	stereo.c


noinst_HEADERS =  speex.h \
	speex_bits.h \
	speex_header.h \
	speex_callbacks.h \
	speex_stereo.h\
	lsp.h \
	nb_celp.h \
	lpc.h \
	ltp.h \
	quant_lsp.h \
	cb_search.h \
	filters.h \
	stack_alloc.h \
	vq.h \
	modes.h \
	sb_celp.h \
	vbr.h \
	misc.h \
	ltp_sse.h \
	filters_sse.h \
	math_approx.h

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

