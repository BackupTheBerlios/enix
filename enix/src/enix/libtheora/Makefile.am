noinst_LTLIBRARIES = libtheora.la

libtheora_la_SOURCES = encode.c \
	idct.c reconstruct.c \
	mcomp.c scan.c blockmap.c misc_common.c \
	dct.c frarray.c pb.c dct_decode.c frinit.c pp.c dct_encode.c \
	huffman.c toplevel.c decode.c huffman.h quant.c 

noinst_HEADERS = hufftables.h quant_lookup.h encoder_internal.h block_inline.h \
		toplevel_lookup.h pp.h encoder_lookup.h 

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
