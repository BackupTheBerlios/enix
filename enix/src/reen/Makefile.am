AM_CFLAGS = @GTK2_CFLAGS@ @GTHREAD2_CFLAGS@ $(XINE_CFLAGS)

bin_PROGRAMS = enix_reen

enix_reen_SOURCES = main.c

enix_reen_LDADD   = $(top_builddir)/src/enix/libenix.la 

# noinst_HEADERS = 

debug:
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

install-debug: debug
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
