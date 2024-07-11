# Detectorbank!

C++/Python software to detect note events using Hopf Bifurcations.

The full documentation, including [examples](https://keziah55.github.io/DetectorBank/PythonExamples.html) 
can be found [here](https://keziah55.github.io/DetectorBank/).

A simple GUI application for basic DetectorBank usage can be found [here](https://github.com/keziah55/detectorbank-gui).

Materials relating to the development of the DetectorBank and the 
OnsetDetector can be found [here](https://github.com/keziah55/ExtraThesisMaterial).

This software is released under the terms of the 
[GNU General Public License](https://www.gnu.org/licenses/gpl-3.0.en.html);
if you wish to use it under different terms, get in touch and we'll sort 
something out.

## Requirements

On a Debian system, the following packages should be installed:

* python3-dev
* python3-build
* python3-virtualenv
* python3-numpy
* swig (to build python3 bindings)
* build-essential
* autoconf-archive
* libtool
* libcereal-dev
* librapidxml-dev
* libfftw3-dev
* doxygen (for the documentation)
* python3 (for the documentation as well as the SWIG bindings)
* graphviz (for enhanced documentation)
* python3-tap (for the unit tests)

### Debian/Ubuntu/Mint

```
apt install python3-dev python3-build python3-virtualenv python3-numpy swig build-essential autoconf-archive pkg-config libtool libcereal-dev librapidxml-dev libfftw3-dev doxygen python3 graphviz python3-tap
```

### Fedora

```
dnf install make automake gcc gcc-c++ kernel-devel python3-devel python3-build python3-numpy swig autoconf-archive libtool fftw-devel rapidxml-devel cereal-devel doxygen python3 graphviz
```

## Installation

You need to `apt install autoconf-archive` for lovely
extra macros if you want this to build on debian.

Set up the build system if required

```
autoreconf --install --verbose
```

(or -iv).

We recommend you build the software to and install it from
a new directory.

```
mkdir -p build
cd build
```

The Python extension requires [NumPy](https://numpy.org/); your system numpy version
will be used by default. However, this may lead to issues if your system NumPy version
is different from the version you use in a virtual environment. If you want to build
the `detectorbank` extension to work with a specifc NumPy version, we recommend that
you build it in a venv with your required NumPy version installed, for example:
```bash
python -m venv .venv
source .venv/bin/activate
python -m pip install "numpy<2" build wheel setuptools
```

Next configure the build system
```bash
../configure
```
Note that if you're using a virtual environment, pass the path to the python executable
to `configure`, for example
```bash
../configure PYTHON=.venv/bin/python
```

Then build `DetectorBank`
```bash
make
```

Optionally build and run the unit tests. Note that the Python tests execute in their own
virtual environment, so before running `make check`, you may need to `deactivate` your
build venv.
```bash
make check
```

Note that to run a single test, you can look in `tests/Makefile.am` then supply a replacement for TESTS naming
only those you wish to run, e.g.
```bash
make check TESTS='Pytests'
```

The results of the checks are written in test/test-suite.log
```bash
sudo make install
```

On some platforms you may have to add `/usr/local/lib/` to your `LD_LIBRARY_PATH`
```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
```
and/or refresh the shared object cache:
```bash
sudo ldconfig
```

System-wide installation of Python packages via pip is deprecated, but you 
can install `detectorbank` in a Python virtual environment via `pip` from 
the wheel in `build/dist`. 
For example, to create a new venv and install `detectorbank`, starting from the `build` dir:
```bash
# note that the exact name of the wheel depends on the python version you are using
DETBANK_WHEEL=`readlink -f dist/detectorbank-1.0.0-cp312-cp312-linux_x86_64.whl`
cd ~
mkdir detbank
cd detbank
python3 -m venv .venv
.venv/bin/python -m pip install $DETBANK_WHEEL
```

That should be sufficient to install DetectorBank; the following steps are 
optional/only required on some platforms.

Perhaps test that the library's been installed properly by asking
for the compilation flags
```bash
( cd ; pkg-config detectorbank --cflags --libs )
```

Documentation is built in the doxygen-doc subdirectory and installed
in $(docdir)/html. The default top page for the documentation is
```bash
/usr/local/share/doc/detectorbank/html/index.html
```

If you don't want/need Python3 bindings generated by SWIG, you can
pass the `--without-swig` to the configure command. Note that Python3
is still required to build the library.

You can build for debugging by defining the C preprocesor macro DEBUG when
compiling. For example
```bash
../configure CPPFLAGS=-DDEBUG=2
```

in the example above.

DEBUG levels (can be |ed):

    1: Report thread start and end + general usage
    2: Write description of detector normalisation to /tmp/z.dat
       for search normalisation of stdout for chirp normalisation
    4: Log SlidingBuffer memory debugging info (LONG!)

Note that stuff gets printed when DEBUG == 0 (or simply exists).
For no debugging output, don't define it at all.

## Note Detection

Work-in-progress note detector using the DetectorBank can be found [here](https://github.com/keziah55/NoteDetector).
