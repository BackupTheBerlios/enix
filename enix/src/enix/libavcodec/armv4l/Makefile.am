##
## Process this file with automake to produce Makefile.in
##

AM_CFLAGS = -O2 $(LIBAVCODEC_CFLAGS)
ASFLAGS =

noinst_LTLIBRARIES = libavcodec_armv4l.la

libavcodec_armv4l_src = dsputil_arm.c jrevdct_arm.S mpegvideo_arm.c
EXTRA_DIST =  $(libavcodec_armv4l_src) dummy.c

if BUILD_ARM
armv4l_modules = $(libavcodec_armv4l_src)
else
armv4l_modules = dummy.c
endif

libavcodec_armv4l_la_SOURCES = $(armv4l_modules)

noinst_HEADERS = 

.s.lo:
	$(ASCOMPILE) -o $@ `test -f $< || echo '$(srcdir)/'`$<

debug:
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS) $(LIBFFMPEG_CFLAGS)"

install-debug: debug
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
