##
## Process this file with automake to produce Makefile.in
##

SUBDIRS = armv4l i386 alpha liba52

## we include ppc/* files since auto* doesn't dive into.
EXTRA_DIST = fdctref.c imgresample.c \
	ppc/dsputil_ppc.c   ppc/mpegvideo_altivec.c \
	ppc/dsputil_altivec.c  ppc/mpegvideo_ppc.c \
	ppc/dsputil_altivec.h  ppc/idct_altivec.c

AM_CFLAGS = $(RELEASE_CFLAGS) $(LIBAVCODEC_CFLAGS)
ASFLAGS =

noinst_LTLIBRARIES = libavcodec.la

libavcodec_la_SOURCES = \
	ac3enc.c \
	a52dec.c \
	adpcm.c \
	allcodecs.c \
	avcodec.c \
	common.c \
	dsputil.c \
	dv.c \
	error_resilience.c \
	eval.c \
	fdctref.c \
	fft.c \
	h263.c \
	h263dec.c \
	huffyuv.c \
	imgconvert.c \
	imgresample.c \
	jfdctfst.c \
	jfdctint.c \
	jrevdct.c \
	mace.c \
	mdct.c \
	mem.c \
	mjpeg.c \
	motion_est.c \
	mpeg12.c \
	mpegaudio.c \
	mpegaudiodec.c \
	mpegvideo.c \
	msmpeg4.c \
	oggvorbis.c \
	pcm.c \
	ratecontrol.c \
	resample.c \
	rv10.c \
	simple_idct.c \
	svq1.c \
	utils.c \
	wmadec.c 

libavcodec_la_LDFLAGS = \
	$(top_builddir)/src/enix/libavcodec/armv4l/libavcodec_armv4l.la \
	$(top_builddir)/src/enix/libavcodec/i386/libavcodec_mmx.la \
	-avoid-version -module


noinst_HEADERS = \
	ac3.h \
	ac3tab.h \
	avcodec.h \
	bswap.h \
	common.h \
	dsputil.h \
	dvdata.h \
	fastmemcpy.h \
	h263data.h \
	mpeg12data.h \
	mpeg4data.h \
	mpegaudio.h \
	mpegaudiodectab.h \
	mpegaudiotab.h \
	mpegvideo.h \
	msmpeg4data.h \
	oggvorbis.h \
	simple_idct.h \
	svq1_cb.h \
	wmadata.h 

debug:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) || exit;\
	done;
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS) $(LIBAVCODEC_CFLAGS)"

install-debug: debug
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
