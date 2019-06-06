TEST_LOG_DRIVER       = env AM_TAP_AWK='$(AWK)' $(SHELL) \
                          $(top_srcdir)/build-aux/tap-driver.sh
TEST_LOG_DRIVER_FLAGS = '--comments --ignore-exit'

TESTS          = cppDetectorBank cppNoteDetector

check_PROGRAMS = cppDetectorBank cppNoteDetector

cppDetectorBank_SOURCES = c++tests-detectorbank.cpp \
                          tap++.cpp tap++.h

cppNoteDetector_SOURCES = c++tests-notedetector.cpp \
                          tap++.cpp tap++.h

LDADD          = -L../src/.libs
AM_CPPFLAGS    = -I$(top_srcdir)/src
AM_LDFLAGS     = -ldetectorbank -lfftw3f -pthread

if DO_SWIG_BINDINGS
TESTS += Pytests
endif

EXTRA_DIST     = $(TESTS) Pytests.in Pytests.py Py_absZtest.py
