dnl
dnl Configure.in for xine|enix
dnl

AC_INIT
AC_CONFIG_SRCDIR([src/enix/xine_stream.c])

dnl
dnl Require autoconf version 2.53
dnl
AC_PREREQ(2.53)

ENIX_MAJOR=0
ENIX_MINOR=1
ENIX_SUB=3

AC_SUBST(ENIX_MAJOR)
AC_SUBST(ENIX_MINOR)
AC_SUBST(ENIX_SUB)

##
AC_CANONICAL_TARGET

##
AM_INIT_AUTOMAKE("enix", $ENIX_MAJOR.$ENIX_MINOR.$ENIX_SUB)

AM_CONFIG_HEADER(config.h)

dnl ---------------------------------------------
dnl Check for programs.
dnl ---------------------------------------------
dnl Save CFLAGS, AC_ISC_POSIX set some unwanted default CFLAGS
saved_CFLAGS="$CFLAGS"
AC_ISC_POSIX
CFLAGS="$saved_CFLAGS"
AC_PROG_CC
AC_HEADER_STDC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_AS

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

dnl ---------------------------------------------
dnl Libtool
dnl ---------------------------------------------
AC_LIBTOOL_DLOPEN
dnl AC_DISABLE_STATIC
AC_PROG_LIBTOOL
dnl AC_PROG_LIBTOOL_SANITYCHECK

dnl
dnl Check for xine-lib
dnl
AM_PATH_XINE(1.0.0,, AC_MSG_ERROR(*** You should install xine-lib first ***))

dnl ---------------------------------------------
dnl check for gtk 2
dnl ---------------------------------------------
PKG_CHECK_MODULES(GLIB2, glib-2.0 >= 2.0.0)
PKG_CHECK_MODULES(GTK2, gtk+-2.0 >= 2.0.0)
PKG_CHECK_MODULES(GTHREAD2, gthread-2.0 >= 2.0.0)

dnl
dnl threads: xine-config tell us what should be used, but
dnl          xitk need to be linked to thread lib, so the follow AC_SUBST() 
dnl          are only used in src/xitk/xine-toolkit/Makefile.am
dnl
case "$host" in
  *-*-freebsd*)
    THREAD_LIBS="-L/usr/local/lib -pthread"
    THREAD_CFLAGS="-I/usr/local/include -D_THREAD_SAFE"
    CFLAGS="$CFLAGS -L/usr/local/lib $THREAD_CFLAGS"
    CPPFLAGS="$CPPFLAGS -I/usr/local/include -L/usr/local/lib"
    ;;
  *)
    AC_CHECK_LIB(pthread, pthread_create,
  	     THREAD_LIBS="-lpthread",
 	     AC_MSG_ERROR(pthread needed))
    ;;
esac
AC_SUBST(THREAD_LIBS)
AC_SUBST(THREAD_CFLAGS)


dnl
dnl Checks for X11
dnl
AC_PATH_XTRA
if test x"$no_x" != x"yes"; then
    AC_DEFINE(HAVE_X11,,[Define this if you have X11R6 installed])
fi

AC_HEADER_STAT
AC_HEADER_DIRENT

dnl ---------------------------------------------
dnl Ogg/Vorbis libs.
dnl ---------------------------------------------
AM_PATH_OGG(
           [ AM_PATH_VORBIS([], 
	                    AC_MSG_ERROR("ogg/vorbis libraries (plus headers) are needed.)) 
	   ], 
	   AC_MSG_ERROR("ogg/vorbis libraries (plus headers) are needed.)) 


dnl
dnl includes
dnl
INCLUDES='-I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src/enix -I$(top_builddir)/src/enix'
AC_SUBST(INCLUDES)

dnl ---------------------------------------------
dnl speex lib/headers
dnl ---------------------------------------------

AC_CHECK_LIB(speex, speex_encoder_init, HAVE_SPEEX="maybe", AC_MSG_ERROR("speex libraries (plus headers) needed"), [-lm -logg])
AC_CHECK_HEADER(speex.h, , AC_MSG_ERROR("(required) speex headers not found."))


dnl ---------------------------------------------
dnl CFLAGS
dnl ---------------------------------------------
AC_SUBST(DEBUG_CFLAGS)
DEBUG_CFLAGS="$CFLAGS -g -DDEBUG -D_FILE_OFFSET_BITS=64 -Wall -O"
CFLAGS="-D_FILE_OFFSET_BITS=64 -Wall -O3"

dnl ---------------------------------------------
dnl libavcodec (ffmpeg) cflags
dnl ---------------------------------------------

AC_SUBST(LIBAVCODEC_CFLAGS)
LIBAVCODEC_CFLAGS="-DCONFIG_ENCODERS -DCONFIG_DECODERS -DSIMPLE_IDCT -DHAVE_AV_CONFIG_H -DRUNTIME_CPUDETECT"

dnl ---------------------------------------------
dnl host / architecture specific stuff
dnl ---------------------------------------------

enable_arm="no"
enable_mmx="no"
enable_ppc="no"
enable_alpha="no"
enable_sparc="yes"

host_or_hostalias="$host"
if test "$host_or_hostalias" = ""; then
    host_or_hostalias="$host_alias"
fi

case "$host_or_hostalias" in

  i386-*-freebsd*)
    CFLAGS="$CFLAGS -pipe -fomit-frame-pointer -fexpensive-optimizations -ffast-math -funroll-loops -finline-functions -mcpu=pentiumpro"
    DEBUG_CFLAGS="$X_CFLAGS $DEBUG_CFLAGS"
    AC_DEFINE(__i386__,1,[Define this if you're running x86 architecture])
    AC_DEFINE_UNQUOTED(ARCH_X86,,[Define this if you're running x86 architecture])

    enable_mmx="yes"
    ;;

  alpha-*)
    enable_alpha="yes"
    ;;

  sparc-*)
    enable_sparc="yes"
    ;;

  ppc-*-linux* | powerpc-*) 
    CFLAGS="$CFLAGS -fomit-frame-pointer"
    dnl avoid ppc compilation crash
    AS="$CC"
    AC_DEFINE_UNQUOTED(ARCH_PPC,,[Define this if you're running PowerPC architecture])
    enable_ppc="yes"
    ;;

  armv4l-*-linux*)
    CFLAGS="$CFLAGS -fsigned-char -mcpu=strongarm1100 -fthread-jumps -fregmove"
    enable_arm="yes"
    ;;

  i?86-*-linux* | i386-*-solaris* | i?86-* | k?-* | athlon-* | pentium*)
    AC_DEFINE_UNQUOTED(ARCH_X86,,[Define this if you're running x86 architecture])
    enable_mmx="yes"
    AC_DEFINE_UNQUOTED(HAVE_MMX,,[Define this if you're running on an mmx capable architecture (x86)])
    CFLAGS="$CFLAGS -pipe -fomit-frame-pointer -fexpensive-optimizations -ffast-math -funroll-loops -finline-functions -mcpu=pentiumpro"
    ;;

  *)
    echo "unknown architecture, only default optimizations (-O3) enabled"
    ;;

esac

AM_CONDITIONAL(BUILD_ALPHA, test x"$enable_alpha" = "xyes")
AM_CONDITIONAL(BUILD_SPARC, test x"$enable_sparc" = "xyes")
AM_CONDITIONAL(BUILD_PPC,   test x"$enable_ppc" = "xyes")
AM_CONDITIONAL(BUILD_ARM,   test x"$enable_arm" = "xyes")
AM_CONDITIONAL(BUILD_MMX,   test x"$enable_mmx" = "xyes")

dnl ---------------------------------------------
dnl set include paths
dnl ---------------------------------------------

INCLUDES='-I$(top_srcdir) -I$(top_srcdir)/src/enix -I$(top_builddir)/src/enix -I$(top_srcdir)/src/enix/libavcodec -I$(top_builddir)/src/enix/libavcodec'
AC_SUBST(INCLUDES)


AC_CONFIG_FILES([
Makefile
src/Makefile
src/enix/Makefile
src/enix/libavcodec/Makefile
src/enix/libavcodec/armv4l/Makefile
src/enix/libavcodec/alpha/Makefile
src/enix/libavcodec/i386/Makefile
src/cut/Makefile
src/reen/Makefile
src/phone/Makefile
m4/Makefile
])

AC_OUTPUT

