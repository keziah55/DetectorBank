# Release version
m4_define([package_version_major],[1])
m4_define([package_version_minor],[0])
m4_define([package_version_micro],[0])

# API version (not used yet)
m4_define([api_version],[1])

# Boilerplate
AC_INIT([detectorbank],
        [package_version_major.package_version_minor.package_version_micro],
        [nick@n-ism.org])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CXX
#AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])
AC_CONFIG_HEADERS([config.h])

# Doxyfile and the python build script need massaging
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 test/Makefile
 Doxyfile
])
AC_CONFIG_FILES([setup.py],[chmod +x setup.py])
AC_CONFIG_FILES([test/Pytests],[chmod +x test/Pytests])

# Require the unit test driver
AC_REQUIRE_AUX_FILE([tap-driver.sh])

# Check the availability of necessary executables
AM_PROG_AR
AC_PROG_MKDIR_P
AC_PROG_INSTALL
AC_PROG_AWK
AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN"; then
   AC_MSG_ERROR([Doxygen not found.])
fi
AC_CHECK_PROGS([DOT], [dot])
if test -z "$DOT"; then
   AC_MSG_ERROR([Doxygen needs dot, please install dot first.])
fi


# Python is needed to build some C++ source files, even if SWIG
# bindings aren't required
AM_PATH_PYTHON([3])

LT_INIT

# Dependencies
PKG_PROG_PKG_CONFIG
PKG_INSTALLDIR

# The hilbert transformer uses single precision fftw.
# Don't chance the first argument. It gets used in setup.py.in.
AX_PKG_CHECK_MODULES([FFTW], [fftw3f], [],
                     [],
                     AC_MSG_ERROR([Single-precision FFTW3 library not found.]))

# Conditionally deal with SWIG bindings (yes by default)
AC_ARG_WITH([swig],
    AS_HELP_STRING([--without-swig], [Don't build/install Python3 SWIG bindings]),
    ,
    [with_swig=yes])

AS_IF([test "x$with_swig" != "xno"],
      [AX_PKG_SWIG([3.0],
       ,
       AC_MSG_ERROR([Building/installing python binings requires SWIG >= 3.0.]))
      ])

AS_IF([test "x$with_swig" = "xyes"],
      [
      AC_MSG_NOTICE([Building python bindings with SWIG])
      ],
      [AC_MSG_NOTICE([SWIG interface generation of python bindings is disabled])])

AM_CONDITIONAL([DO_SWIG_BINDINGS], [test "x$with_swig" = "xyes"])

AC_ARG_VAR([python],
    [Python executable to use]
)

AC_SUBST(PYTHON, $(realpath -s $PYTHON))

AC_SUBST([PACKAGE_VERSION_MAJOR],package_version_major)
AC_SUBST([PACKAGE_VERSION_MINOR],package_version_minor)
AC_SUBST([PACKAGE_VERSION_MICRO],package_version_micro)

AC_SUBST([API_VERSION],api_version)

# Output files
# Rename the template .pc file to include the API version on configure
AC_CONFIG_FILES([detectorbank.pc:detectorbank.pc.in],
#        or...  [detectorbank-$API_VERSION.pc:detectorbank.pc.in],
[],
[])
# or... [API_VERSION='$API_VERSION'])  if we're using API versions

# Doxygen macros
DX_XML_FEATURE(ON)
DX_PDF_FEATURE(ON)
DX_HTML_FEATURE(ON)
DX_PS_FEATURE(OFF)
DX_DOT_FEATURE(ON)
DX_INIT_DOXYGEN(Detectorbank, ${builddir}/Doxyfile)

# Tell (PDF)LaTeX the directory where external graphics live
AC_SUBST([docgraphics],${srcdir}/graphics/)

# Get abspath of top srcdir (assuming we're calling this script from top_srcdir/build)
ABS_TOP_DIR="$(pwd)/.."
RXML_CXXFLAGS="-I${ABS_TOP_DIR}/include"
AC_SUBST(AM_CXXFLAGS, $RXML_CXXFLAGS)

AC_OUTPUT
