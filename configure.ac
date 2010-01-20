# Autoconf script. Process with autoconf to produce a configure script
#

AC_PREREQ(2.59)
AC_INIT([spiceweasel], [1.1], [bd512@york.ac.uk])

AM_INIT_AUTOMAKE([1.9 foreign])

######### Configure options

AC_ARG_ENABLE(threads,
	AS_HELP_STRING([--enable-threads],[Enable multi-threading support],
,
[enable_threads="no"]))

######### Programs

AC_PROG_CC

######### Libraries

AS_IF([test "x$enable_threads" = "xyes"], [
   # Need pthreads for multi-threading
   AC_CHECK_LIB([pthread], [pthread_create], , [
      echo "Disabling multi-threading"
      enable_threads="no"
   ])
])

AS_IF([test "x$enable_threads" != "xyes"], [
   # Disable multi-threading
   CFLAGS="$CFLAGS -DSINGLE_THREAD"
   echo "Multi-threading disabled"
], [
   echo "Enabling multi-threading using pthreads"
])

AC_CHECK_LIB([png], [png_read_info], ,[
   # Not found PNG library
   echo "ERROR: PNG library required."
   echo "    Make sure it's on your LD_LIBRARY_PATH"
   exit
])

AC_CHECK_LIB([openjpeg], [opj_decode], ,[
   # Not found
   echo "ERROR: Openjpeg library required."
   echo "    Make sure it's on your LD_LIBRARY_PATH"
   exit])

######### Headers

AC_CHECK_HEADERS([pthread.h time.h openjpeg.h png.h])

######### Output

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
