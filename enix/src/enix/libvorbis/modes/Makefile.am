## Process this file with automake to produce Makefile.in

EXTRA_DIST = floor_all.h psych_44.h residue_44.h setup_11.h setup_32.h \
	     setup_8.h psych_11.h psych_8.h residue_44u.h setup_16.h \
	     setup_44.h setup_X.h psych_16.h residue_16.h residue_8.h \
	     setup_22.h setup_44u.h

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
