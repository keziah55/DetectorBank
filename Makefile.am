SUBDIRS = src test

EXTRA_DIST =  detectorbank.i python_docstrings.i numpy.i \
              doc_fix.py doxy2swig.py \
              Docs dotfiles graphics \
              setup.py.in Doxyfile.in detectorbank.pc.in \
              examples

ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
BUILT_SOURCES = README

# Install the pkg-config file; the directory is set using
# PKG_INSTALLDIR in configure.ac.
pkgconfig_DATA = detectorbank.pc

# We use README.md, but a README file is required by automake
PANDOC = pandoc
README: README.md
	if command -v  $(PANDOC) ; then \
		$(PANDOC) -f markdown -t plain --wrap=none $< -o $@ ; \
	else \
		echo "See $<" > $@ ; \
	fi

# For API versioning use:
# pkgconfig_DATA = detectorbank-$(API_VERSION).pc

# Additional rules so we can install the doxygen tree to docdir
DOXYGENLS = doxygen_file_list

# Setup doxygen build rules
@DX_RULES@

DOCSTAMP = $(DX_DOCDIR)/$(PACKAGE).tag
DOXYSRCSTAMP = stamp-dx

# Make graphics files visible to LaTeX (path set in configure.ac) 
doxygen-doc: export TEXINPUTS=:../../@docgraphics@

# Doxygen source files
DOXYFILES = $(srcdir)/Docs/detectorcache-design.doxy \
            $(srcdir)/Docs/mainpage.doxy \
            $(srcdir)/Docs/samplerates.doxy \
            $(srcdir)/Docs/features.doxy \
            $(srcdir)/Docs/python-examples.doxy \
            $(srcdir)/Docs/slidingbuffer-example.doxy \
            $(srcdir)/Docs/frequencyshifter-operation.doxy \
            $(srcdir)/Docs/python-usage.doxy


# Kick doxygen into action if any of its input files are newer.
$(DOXYSRCSTAMP): $(DOXYFILES)
	touch Doxyfile
	echo Timestamp for Doxygen Sources > $@

# Build a list of doxygen html files so they can be installed to the doc target
$(DOXYGENLS): $(DOCSTAMP) $(DOXYSRCSTAMP)
	echo doxygen_files = \\ > $(DOXYGENLS)
	( cd $(DX_DOCDIR) && find html -type f | paste -s -d ' ' >> ../$(DOXYGENLS) )

MOSTLYCLEANFILES = $(DX_CLEANFILES) $(DOXYGENLS) $(DOXYSRCSTAMP)
CLEANFILES = $(top_srcdir)/README

.PHONY: install-docs uninstall-docs

install-docs: $(DOCSTAMP)
	$(MKDIR_P) $(docdir)/html
	@echo Installing html documentation
	@( cd $(DX_DOCDIR) && $(install_sh) $(doxygen_files) $(docdir)/html )

install-data-local: install-docs

uninstall-docs-local:
	rm -rf $(docdir)/html


if DO_SWIG_BINDINGS
SWIGINPUTS = documentation.i detectorbank.i
SWIGOUTPUTS = detectorbank_wrap.cpp detectorbank.py
SWIGFILES = $(SWIGINPUTS) $(SWIGOUTPUTS)
MOSTLYCLEANFILES += documentation.i $(SWIGOUTPUTS) stamp-py stamp-lib

documentation.i: $(DOCSTAMP)
	$(PYTHON) setup.py docs

all-local:  $(SWIGOUTPUTS)

# If stamp-lib (touched by library build) doesn't exist yet, don't try to make it
stamp-lib:

# All of the swig targets get build with a sinlge command,
# so this is a grouped target.
$(SWIGOUTPUTS) &: $(SWIGINPUTS) stamp-lib
	$(PYTHON) setup.py buildDB

# installation requires SWIG to be complete.
install-exec-local: $(SWIGOUTPUTS)
	$(PYTHON) setup.py install

clean-local:
	$(PYTHON) setup.py clean --all
	$(RM) -r __pycache__
	$(RM) $(MOSTLYCLEANFILES)

uninstall-local: uninstall-docs # put setup.py uninstall target here
else
uninstall-local: uninstall-docs


endif

.PHONY: link-rapidxml

link-rapidxml:
	$(srcdir)/link_rapidxml.sh $(srcdir)
AM_CXXFLAGS = $(RXML_CPPFLAGS)

all: link-rapidxml

include $(DOXYGENLS)
