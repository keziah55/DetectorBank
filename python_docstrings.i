// Python docstrings for methods which take numpy arrays

// DetectorBank

%feature("autodoc", "

Construct a DetectorBank from either given parameters or an archived profile.

Either:

    DetectorBank::DetectorBank(profile, inputBuffer)

Or:
    
    DetectorBank::DetectorBank(sr, inputBuffer, numThreads, detector_characteristics, features=runge_kutta|freq_unnormalized|amp_normalized, damping=0.0001, gain=25)


Parameters
-----------
profile : str
    The name of the profile to read from the archive

inputBuffer : numpy.ndarray
    Audio input

    
Parameters
----------
sr : float
    Sample rate of audio. (This must be 44100 or 48000.)

inputBuffer : numpy.ndarray
    Audio input

numThreads : int
    Number of threads to execute concurrently to determine
    the detector outputs. Passing a value of less than 1 causes the number
    of threads to be set according to the number of reported CPU cores

detector_characteristics : numpy.ndarray
    2D array of frequencies and bandwidths for each detector

features : Features
    Numerical method (runge_kutta or central_difference),
    freqency normalisation (freq_unnormalized or search_normalized) and
    amplitude normalisation (amp_unnormalized or amp_normalized). Default
    is runge_kutta|freq_unnormalized|amp_normalized. 

damping : float
    Damping for all detectors

gain : float
    Audio input gain to be applied. (Default value of 25 is
    recommended in order to keep the numbers used in the internal
    calculations within a sensible range.)

Raises
------
ValueError
    Sample rate should be 44100 or 48000") DetectorBank;


%feature("autodoc", "

Change the input without recreating the detector bank.

Parameters
----------
inputBuffer : numpy.ndarray
    New input samples") DetectorBank::setInputBuffer;


%feature("autodoc", "

Get the next numSamples of detector bank output.

Parameters
----------
frames : numpy.ndarray
    Complex 2D output array (channels x numSamples)

Returns
-------
Number of frames processed") DetectorBank::getZ;


%feature("autodoc", "

Take (complex) z frames and fill a given array of the same dimensions
(absFrames) with their absolute values.

Also returns the maximum value in absFrames.

Parameters
----------
absFrames : numpy.ndarray
    Output array

frames : numpy.ndarray
    Input array of complex z values

Returns
-------
The maximum value found while performing the conversion.") DetectorBank::absZ;


// DetectorCache

%feature("autodoc", "

Construct a DetectorCache.  

Parameters
----------
p : Producer
    The segment producer for this cache
num_segs : int
    Number of historical segments to remember.  
seg_len : int
    Number of audio samples per segment.  
start_chan : int
    First channel for this cache.
num_chans : int
    Number of channels in this cache.") detectorcache::DetectorCache;

%feature("autodoc", "

Efficiently copy a range of cached data from a given cache channel to
the designated target.

The caller must allocate sufficient memory for the requested number of
result_t data, and pass its address and the number of results desired.
The supplied memory is filled with results from the detectorcache,
ending at the given index.

This permits an external onset detecting algorithm to request the
history of a detector channel once an onset threshold has been
achieved in order to refine its estimate of the precise onset time.

Parameters
----------

chan : int
    Channel for which to provide history

currentSample : int
    Index from which to roll back

samples : numpy.ndarray   
    Target array

Returns
-------
The number of samples actually copied.

Raises
------
IndexError slidingbuffer::ExpiredIndexException 
    If the requested number of samples exceeds that in the cache.") detectorcache::DetectorCache::getPreviousResults;

    
// OnsetDetector

%feature("autodoc", "

Construct an onset detector, given a DetectorBank

Parameters
----------
DetectorBank : DetectorBank
    DetectorBank object") OnsetDetector;

    
// NoteDetector
    
%feature("autodoc", "

Construct a NoteDetector.

Please note that any keyword arguments cannot be supplied as a mixture of default
positional and keyword arguments because of C++ binding limitations.

Parameters
----------

sr : float
    Sample rate of the input audio
inputBuffer : numpy.ndarray
    Input array
freqs : numpy.ndarray
    Array of frequencies to look for
bandSize : int 
    Number of detectors in to make in each critial band 
bandwidth : float
     Bandwidth for every detector. Default is minimum bandwidth.
features : Features
    Features enum containing method and normalisation. 
    Default is runge_kutta | freq_unnormalized | amp_normalized
damping : float
    Detector damping. Default is 0.0001
gain : float
    Detector gain. Default is 25.") NoteDetector;
    
    
%feature("autodoc", "

Get onsets (and, later, pitch) for all given frequencies.  

Parameters
----------
* `onsets` :  
    Empty detectorbank.OnsetDict() to be filled  
* `threshold` :  
    OnsetDetector threshold  
* `w0` :  
    OnsetDetector::analyse weight for 'count' criterion  
* `w1` :  
    OnsetDetector::analyse weight for 'threshold' criterion  
* `w2` :  
    OnsetDetector::analyse weight for 'difference' criterion  

Returns
-------
Dictionary which maps indices from the frequencies array to tuples of
onset sample times.") NoteDetector::analyse;

    
// OnsetDict

%feature("autodoc", "

Make an OnsetDict for NoteDetector results.

The NoteDetector finds onset times for every requested frequency. 
When calling NoteDetector.analyse(), an OnsetDict() must be supplied to be filled
with the found onset times. OnsetDict() is a dictionary which maps indices from the 
frequencies array to tuples of onset sample times.") OnsetDict;


// FrequencyShifter

%feature("autodoc", "

Construct a FrequencyShifter. 

Parameters
----------
inputSignal : numpy.ndarray
    The signal to be shifted  
sr : float
    Sample rate of the audio
mode : HilbertMode
    FIR or FFT") FrequencyShifter;
    
    
%feature("autodoc", "

Shift the input signal by the stated amount and fill shiftedSignal.  

Parameters
----------
fShift : float
    Frequency by which to shift the signal (Hz)  
shiftedSignal : numpy.ndarray 
    Output buffer") FrequencyShifter::shift;
