#!/bin/sh
# run this to generate all the initial makefiles, etc.

PROG=xine-ui

# Check how echo works in this /bin/sh
case `echo -n` in
-n)     _echo_n=   _echo_c='\c';;
*)      _echo_n=-n _echo_c=;;
esac

detect_configure_ac() {

  srcdir=`dirname $0`
  test -z "$srcdir" && srcdir=.

  (test -f $srcdir/configure.ac) || {
    echo $_echo_n "*** Error ***: Directory "\`$srcdir\`" does not look like the"
    echo " top-level directory"
    exit 1
  }
}


#--------------------
# AUTOCONF
#-------------------
detect_autoconf() {
  (autoconf --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`autoconf' installed to compile gxine."
    echo "Download the appropriate package for your distribution,"
    echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
    exit 1
  }
}

run_autoheader () {
  echo $_echo_n " + Running autoheader: $_echo_c";
    autoheader;
  echo "done."
}

run_autoconf () {
  echo $_echo_n " + Running autoconf: $_echo_c";
    autoconf;
  echo "done."
}

#--------------------
# LIBTOOL
#-------------------
detect_libtool() {
  (libtool --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed to compile gxine."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool-1.4.tar.gz"
    echo "(or a newer version if it is available)"
    exit 1
  }
}

run_libtoolize() {
  echo $_echo_n " + Running libtoolize: $_echo_c";
    libtoolize --force --copy >/dev/null 2>&1;
  echo "done."
}

#--------------------
# AUTOMAKE
#--------------------
detect_automake() {
  if [ x != x`which automake-1.6` ]; then
    AUTOMAKE_CMD="automake-1.6"
  else
    if [ x != x`which automake` ]; then
      AUTOMAKE_CMD="automake"
    else
      echo
      echo "You must have automake installed to compile $PROG."
      echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.6.tar.gz"
      echo "(or a newer version if it is available)"
      exit 1
    fi
  fi
}

run_automake () {
  if test x"$automake_1_6x" = x"no"; then
    echo "Warning: automake < 1.6. Some warning message might occur from automake"
    echo
  fi

  echo $_echo_n " + Running automake: $_echo_c";

  $AUTOMAKE_CMD --gnu --add-missing --copy

  echo "done."
}

#--------------------
# ACLOCAL
#-------------------
detect_aclocal() {

  # if no automake, don't bother testing for aclocal
  if [ x != x`which aclocal-1.6` ]; then
    ACLOCAL_CMD="aclocal-1.6"
  else
    if [ x != x`which aclocal` ]; then
      ACLOCAL_CMD="aclocal"
    else
      echo
      echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
      echo "installed doesn't appear recent enough."
      echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.6.tar.gz"
      echo "(or a newer version if it is available)"
      exit 1
    fi
  fi
}

run_aclocal () {
  echo $_echo_n " + Running aclocal: $_echo_c"
  if [ -z "$XINE_CONFIG" ]; then
    aclocalinclude=`xine-config --acflags`
  else
    aclocalinclude=`${XINE_CONFIG} --acflags`
  fi
  $ACLOCAL_CMD $aclocalinclude -I m4
  echo "done." 
}

#--------------------
# CONFIGURE
#-------------------
run_configure () {
  rm -f config.cache
  echo " + Running 'configure $@':"
  if [ -z "$*" ]; then
    echo "   ** If you wish to pass arguments to ./configure, please"
    echo "   ** specify them on the command line."
  fi
  ./configure "$@" 
}


#---------------
# MAIN
#---------------
detect_configure_ac
detect_autoconf
detect_libtool
detect_automake
detect_aclocal


#   help: print out usage message
#   *) run aclocal, autoheader, automake, autoconf, configure
case "$1" in
  aclocal)
    run_aclocal
    ;;
  autoheader)
    run_autoheader
    ;;
  automake)
    run_automake
    ;;
  autoconf)
    run_aclocal
    run_autoconf
    ;;
  libtoolize)
    run_libtoolize
    ;;
  noconfig)
    run_aclocal
    run_libtoolize
    run_autoheader
    run_automake
    run_autoconf
    ;;
  *)
    run_aclocal
    run_libtoolize
    run_autoheader
    run_automake
    run_autoconf
    run_configure $@
    ;;
esac
