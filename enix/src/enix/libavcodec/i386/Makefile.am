##
## Process this file with automake to produce Makefile.in
##

AM_CFLAGS = $(LIBAVCODEC_CFLAGS)

ASFLAGS =

#LIBTOOL = $(SHELL) $(top_builddir)/libtool-nofpic

noinst_LTLIBRARIES = libavcodec_mmx.la

libavcodec_mmx_src = \
	cputest.c \
	dsputil_mmx.c \
	fdct_mmx.c \
	fft_sse.c \
	idct_mmx.c \
	motion_est_mmx.c \
	mpegvideo_mmx.c \
	simple_idct_mmx.c

libavcodec_mmx_dummy = libavcodec_mmx_dummy.c

EXTRA_DIST = \
	$(libavcodec_mmx_dummy) \
	$(libavcodec_mmx_src) \
	mpegvideo_mmx_template.c

if BUILD_MMX
mmx_modules = $(libavcodec_mmx_src)
endif


libavcodec_mmx_la_SOURCES = $(mmx_modules) $(libavcodec_mmx_dummy)

noinst_HEADERS = dsputil_mmx_avg.h dsputil_mmx_rnd.h mmx.h

.s.lo:
	$(ASCOMPILE) -o $@ `test -f $< || echo '$(srcdir)/'`$<

debug:
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS) $(LIBAVCODEC_CFLAGS) -pipe -fomit-frame-pointer -mcpu=pentiumpro"

install-debug: debug
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
