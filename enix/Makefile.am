##
## Process this file with automake to produce Makefile.in
##

SUBDIRS = src m4

EXTRA_DIST = autogen.sh

debug:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) || exit;\
	done;

install-debug: debug
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) || exit; \
	done;

prune-cache:
	-rm -f config.cache

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#* 
	-rm -f $(PACKAGE)_$(VERSION).tar.gz
	-rm -f $(distdir).tar.gz $(PACKAGE).tgz package_descriptions
	-rm -rf $(distdir)

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in configure acinclude.m4 aclocal.m4
	-rm -f ltconfig ltmain.sh
	-rm -f config.guess config.sub install-sh missing mkinstalldirs
	-rm -f libtool-nofpic depcomp config.log

maintainer-clean-generic-hook: 
	rm -f config.status


