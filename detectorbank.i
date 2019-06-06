%module detectorbank

%inline %{
#define SWIG_FILE_WITH_INIT

#include <sstream>

//#include "slidingbuffer.h"
#include "detectorbank.h"
#include "detectorcache.h"
#include "onsetdetector.h"
#include "detectortypes.h"
#include "notedetector.h"
#include "frequencyshifter.h"

// Make SWIG happy with std::size_t and size_t being the same
typedef unsigned long int size_t;
%}


%include "documentation.i"
%include "python_docstrings.i"

%include "numpy.i"

//%include <std_deque.i>
%include <std_vector.i>
%include <std_except.i>
%include <std_string.i>
%include <std_map.i>
%include <typemaps.i>

%init %{
import_array();
%}

// Explicitly ignore nested classes
%ignore DetectorBank::detector_components;
%ignore DetectorBank::GetZ_params;
%ignore DetectorBank::AbsZ_params;
%ignore NoteDetector::Analyse_params;

%fragment("NumPy_Fragments");

// pass std::invalid_argument and std::runtime_error exceptions to Python
// SlidingBufferExceptions are std::runtime_errors in C++, but should be
// IndexErrors in Python
%exception {
    try {
        $action
    } catch (std::invalid_argument &e) {
        PyErr_SetString(PyExc_ValueError, const_cast<char*>(e.what()));
        SWIG_fail;
    } catch (slidingbuffer::SlidingBufferException &e) {
        PyErr_SetString(PyExc_IndexError, const_cast<char*>(e.what()));
        SWIG_fail;
    } catch (std::runtime_error &e) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char*>(e.what()));
        SWIG_fail;
    }
}


%extend DetectorBank {
    %pythoncode %{
       ## NB this is pasted directly to the NoteDetector bit of this file ##
       ## If changing anything, make sure both bits are changed ##
       def _checkBufferType(self, buf):
            # Check that the data type is float32
            # Also, flatten arrays of more than 1 dimension

            from numpy import array, dtype, mean

            # Check data type
            if buf.dtype is not dtype('float32'):
                if buf.dtype is dtype('int16') :
                    buf = array(buf, dtype=dtype('float32'))/(2**15)
                else:
                    buf = array(buf, dtype=dtype('float32'))

            # Check number of dimensions
            if buf.ndim > 1 :
                buf = mean(buf, axis=1)

            del array, dtype, mean

            return buf

       def _checkNumericArgs(self, args):
           # typecheck/cast the sample rate, num threads, damping and gain
           # as there are some weird errors where NumPy types cause errors

            # args[0,2,5,6] should be of the following types
            types = {0:float, 2:int, 5:float, 6:float}

            for idx in types.keys():
                try:
                    args[idx] = types[idx](args[idx])
                except IndexError:
                    # args 5 and 6 may not be there, which is no big deal
                    pass
                except ValueError:
                    raise ValueError('Arg {} should be of type {}'.format(idx, types[idx]))

            return args

       def _checkDetCharType(self, det_char):
           # cast det_char array (frequencies and bandwidths) to float
           return det_char.astype(float)
    %}
};

## NB this is pasted directly to the NoteDetector bit of this file ##
## If changing anything, make sure both bits are changed ##
%pythonprepend DetectorBank::DetectorBank %{
    # Make sure all the DetectorBank arguments coming from Python are the
    # right type etc.

    args = list(args)

    # The c++ code only deals with mono audio but we'd like to deal with
    # more channels on demand
    # Fortunately, the input buffer is the second argument in all forms
    # of the constructor, so no need to check len(args) here.
    # This also makes sure the data type is float32
    b = self._checkBufferType(args[1])

    if b is not args[1]:
        args[1] = b

    # Keep a local copy so it doesn't get garbage-collected
    self._ibuf = args[1]

    # If the DetectorBank has been given parameters to construct
    # a new bank (as opposed to loading a saved profile), check that
    # all the args are of the right type
    if len(args) > 2:

        from numpy import transpose

        # Typecheck/cast the sample rate, num threads, damping and gain
        # as there are some weird errors where NumPy types cause errors
        args = self._checkNumericArgs(args)

        # Cast frequencies and bandwidths to float so they can
        # be reliably cast to parameter_t
        # It doesn't complain if we don't do this (probably because
        # the typemap is only looking for a pointer), but it seems
        # like a nasty bug waiting to happen
        args[3] = self._checkDetCharType(args[3])

        # Transpose det_char so that it can be SWIGed
        args[3] = transpose(args[3])
        # Keep a local copy
        self._transposed_array = args[3]

        del transpose
%}
%pythonprepend DetectorBank::setInputBuffer %{
    inputBuffer = self._checkBufferType(inputBuffer)

    # Keep a local copy so it doesn't get garbage-collected
    self._ibuf = inputBuffer
%}
%pythonprepend DetectorBank::getFreq_in %{
    ch = int(ch)
%}

/*! Change numpy typemap for 2D arrays so we can split a
*   2D array into two 1D arrays.
*   Used when given 2D array of frequencies and bandwidths.
*   Takes a pointer and two sizes.
*   Returns a pointer to the first array, pointer to second
*   array and length of both arrays.
*
*   The following three methods are taken directly from
*   numpy.i (Typemap suite for (DATA_TYPE* IN_ARRAY2, DIM_TYPE DIM1,
*   DIM_TYPE DIM2)) and the second one has been modified.
*/
%typecheck(SWIG_TYPECHECK_DOUBLE_ARRAY,
           fragment="NumPy_Macros")
  (parameter_t* IN_ARRAY2, DIM_TYPE DIM1, DIM_TYPE DIM2)
{
  $1 = is_array($input) || PySequence_Check($input);
}
%typemap(in,
         fragment="NumPy_Fragments")
  (parameter_t* IN_ARRAY2, DIM_TYPE DIM1, DIM_TYPE DIM2)
  (PyArrayObject* array=NULL, int is_new_object=0)
{
    npy_intp size[2] = { -1, -1 };
    array = obj_to_array_contiguous_allow_conversion($input,
      NPY_DOUBLE,
      &is_new_object);

    if (!array || !require_dimensions(array, 2) ||
      !require_size(array, size, 2)) SWIG_fail;

    std::size_t fb_size = array_size(array, 1);

    // pointer to first array
    $1 = (parameter_t*) array_data(array);
    // pointer to second array
    $2 = (parameter_t*) (array_data(array5)) + fb_size;
    // size of array
    $3 = (std::size_t) fb_size;
}
%typemap(freearg)
  (parameter_t* IN_ARRAY2, DIM_TYPE DIM1, DIM_TYPE DIM2)
{
  if (is_new_object$argnum && array$argnum)
    { Py_DECREF(array$argnum); }
}

// for some reason, all numpy integers will throw a TypeError
// because C++ doesn't understand them, so we have to
// cast these ints as ints before we can use them. Yep.
%typemap(in) int {
    $1 = static_cast<int>(PyLong_AsLong($input));
}

%typemap(in) std::size_t {
    $1 = static_cast<std::size_t>(PyLong_AsLong($input));
}

%apply (float* IN_ARRAY1, int DIM1) {(const inputSample_t* inputBuffer,
                                      const std::size_t inputBufferSize)};
// apply the typemap we created, which takes a pointer and two sizes and returns
// two pointers and one size
%apply (parameter_t* IN_ARRAY2, DIM_TYPE DIM1, DIM_TYPE DIM2) {(const parameter_t* freqs,
                                                                parameter_t* bw,
                                                                const std::size_t numDetectors)};
%apply (std::complex<double>* INPLACE_ARRAY2, int DIM1, int DIM2) {(discriminator_t* frames,
                                                                    std::size_t chans,
                                                                    std::size_t numFrames)};
%apply (double* INPLACE_ARRAY2, int DIM1, int DIM2) {(result_t* absFrames,
                                                      std::size_t absChans,
                                                      std::size_t absNumFrames)};
%apply (double* INPLACE_ARRAY1, int DIM1) {(result_t* samples,
                                            std::size_t numSamples)};

// A detector cache is a specialisation of a sliding buffer,
// but there's no need to generate python bindings to the
// underlying template classes. So we import the slidingbuffer
// rather than %including it, then declare the template instances
// we actually use. Similarly, the operator of each Segment in
// the cache is of no interest to Python, so it is ignored too.
%ignore slidingbuffer::Segment::operator[];
//%rename(__getitem__) slidingbuffer::SlidingBuffer::operator[];
%ignore slidingbuffer::SlidingBuffer::operator[];
%import "slidingbuffer.h"

%template(resultproducer) slidingbuffer::SegmentProducer<result_t *>;
%template(resultsegment)  slidingbuffer::Segment<result_t *,
                                                 detectorcache::Producer>;
%template(resultcache)    slidingbuffer::SlidingBuffer<result_t *,
                                                       detectorcache::Segment,
                                                       detectorcache::Producer>;
%include "detectortypes.h"
%include "detectorbank.h"
%include "detectorcache.h"

// This works...
// %pythoncode %{
//    DetectorCache.__getitem__ =  lambda self, t: self.getResultItem(t[0],t[1])
// %};


%pythoncode %{
    def _isint(a):
            # check if a given value, a, is an integer (either a Python built-in int
            # or a numpy integer)

            from numpy import integer

            # can't return from if statements, as integer needs to be deleted
            if isinstance(a, int) or isinstance(a, integer):
                result = True
            else:
                result = False

            del integer
            return result


    def _DetectorCache___init__(self, *args):
        """
        Construct a DetectorCache with default start and size.

        If the start channel parameter is omitted, assume 0. If the number of channels
        is omitted, use the value obtained by querying the supplied producer
        """

        # check that args (after Producer) are ints
        # without this, get "NotImplementedError: Wrong number or type of arguments for overloaded function 'new_DetectorCache'"
        # which is not very helpful

        for arg in args[1:]:
            if not _isint(arg):
                raise TypeError("'{}' object cannot be interpreted as an integer"
                                .format(type(arg).__name__))

        # if types are fine, make DetectorCache object
        # note: the following five lines are copied directly from detectorbank.py (before the type checking was implemented)

        this = _detectorbank.new_DetectorCache(*args)
        try:
            self.this.append(this)
        except __builtin__.Exception:
            self.this = this

    def _DetectorCache___getitem__(self, t):
        # print ("You'll want to check number/value of arguments here.")
        # print ("But I'm going to call the C++ __getitem__ with")
        # print ("idx1 = {}, idx2 = {}".format(t[0], t[1]))

        try:
            if t[0] >= self.getChans():
                raise IndexError('Attempting to access channel not in cache.')
            elif not _isint(t[0]):
                raise TypeError("'{}' object cannot be interpreted as an integer"
                                .format(type(t[0]).__name__))
            elif not _isint(t[1]):
                raise TypeError("'{}' object cannot be interpreted as an integer"
                                .format(type(t[1]).__name__))
            else:
                # even though we've just checked if indices are ints, we must cast
                # them as Python built-in ints, as DetectorCache is expecting longs
                # and Python ints work fine for this, but all numpy ints raise errors
                # (even numpy.int_, which is supposed to be a C long)
                return self.getResultItem(int(t[0]),int(t[1]))
        except TypeError:
            raise IndexError('Indices should be comma-separated integers.')

    DetectorCache.__init__ = _DetectorCache___init__
    DetectorCache.__getitem__ = _DetectorCache___getitem__
%}


// %ignore operator<<;
// %include "onsetdetector.h"
// namespace std {
//     %template(OnsetList) deque<Onset>;
//     %template(FreqValues) vector<onset_freq_t>;
// }
//
// %extend Onset {
//     const std::string __repr__() {
//         std::ostringstream ss;
//         ss << *($self);
//         return ss.str();
//     }
// }

%extend DetectorBank {
    /**
     * The C++ "native" version takes only two pointers
     * to arrays (target and source) and a single chans and frames
     * count, because they are constrained to be the same size.
     *
     * The python version needs to supply an INPLACE_ARRAY2 (target)
     * and IN_ARRAY2 (source), so this overload makes that easy.
     */
    inline result_t DetectorBank::absZ(result_t* absFrames,
                                       std::size_t absChans,
                                       std::size_t absNumFrames,
                                       discriminator_t* frames,
                                       std::size_t chans,
                                       std::size_t numFrames,
                                       std::size_t maxThreads = 0
                    ) const {
        if (absChans != chans || absNumFrames != numFrames)
            throw std::runtime_error(
                "DetectorBank::absZ input and output arrays must be the same shape"
            );

        return $self->absZ(absFrames, absChans, absNumFrames, frames, maxThreads);

    }
}

%apply (float* IN_ARRAY1, int DIM1) {(const inputSample_t* inputSignal,
                                      const std::size_t inputSignalSize)};
%apply (float* INPLACE_ARRAY1, int DIM1) {(inputSample_t* shiftedSignal,
                                           const std::size_t shiftedSignalSize)};

%include "frequencyshifter.h"


%include "onsetdetector.h"


%apply (float* IN_ARRAY1, int DIM1) {(const audioSample_t* inputBuffer,
                                      const std::size_t inputBufferSize)};
%apply (double* IN_ARRAY1, int DIM1) {(const parameter_t* freqs,
                                       const std::size_t freqsSize)};

%extend NoteDetector {
    %pythoncode %{
        SWIG__init__ = __init__
        def __init__(self, *args, **kwargs):
            if len(kwargs) != 0:
                if len(args) != 4:
                    raise TypeError('NoteDetector cannot be instantiated with a mixture of '
                                    'default positional and keyword arguments because of C++ '
                                    'binding limitations')
                optargs = NDOptArgs()
                for arg in kwargs:
                    set_method = getattr(optargs, arg, None)
                    if set_method:
                        set_method(kwargs[arg])
                    else:
                        raise TypeError('Unable to pass {0}={1} because {0} is not a valid '
                                        'paramter name'.format(arg, kwargs[arg]))
                args += (optargs,)
            NoteDetector.SWIG__init__(self, *args)
            
        ## NB this is copied directly from the DetectorBank bit of this file ##
        ## If changing anything, make sure both bits are changed ##
        def _checkBufferType(self, buf):
            # Check that the data type is float32
            # Also, flatten arrays of more than 1 dimension

            from numpy import array, dtype, mean

            # Check data type
            if buf.dtype is not dtype('float32'):
                if buf.dtype is dtype('int16') :
                    buf = array(buf, dtype=dtype('float32'))/(2**15)
                else:
                    buf = array(buf, dtype=dtype('float32'))

            # Check number of dimensions
            if buf.ndim > 1 :
                buf = mean(buf, axis=1)

            del array, dtype, mean

            return buf
    %}
};

## NB this is copied directly from the DetectorBank bit of this file ##
## If changing anything, make sure both bits are changed ##
%pythonprepend NoteDetector::NoteDetector %{
    # make audio mono, if required
    args = list(args)
    # The c++ code only deals with mono audio but we'd like to deal with
    # more channels on demand
    # Fortunately, the input buffer is the second argument in all forms
    # of the constructor, so no need to check len(args) here.
    # This also makes sure the data type is float32
    b = self._checkBufferType(args[1])

    if b is not args[1]:
        args[1] = b
        
    # Keep a local copy so it doesn't get garbage-collected
    self._ibuf = args[1]
%}

namespace std {
    %template(vector_size_t) vector<size_t>;
    %template(OnsetDict) map<size_t, vector<size_t>>;
}


%pythoncode %{
    def _OnsetDict__str__(self):
        # manual implementation of __str__, so that it prints like a regular
        # python dictionary, rather than
        # "<detectorbank.OnsetDict; proxy of <Swig Object of type 'std::map< ]
        # size_t,std::vector< int,std::allocator< int > > > *' at 0x7f0d99c3bde0> >"
        
        out = '{'
        keys = self.keys()
        
        # if the dictionary contains stuff, print it out
        if keys:
            for k in keys[:-1]:
                out += '{}: {}, '.format(k, self.__getitem__(k), end='')
            out += '{}: {}'.format(keys[-1], self.__getitem__(keys[-1]), end='')
            
        out += '}'

        return out

    OnsetDict.__str__ = _OnsetDict__str__
%}


%include "notedetector.h"

