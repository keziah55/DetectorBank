#ifndef _FREQUENCYSHIFTER_H_
#define _FREQUENCYSHIFTER_H_

#include <complex>

#include "detectortypes.h"

/*! Shift a signal by a given frequency. Use SSB modulation,
 *  implemeted via the Hilbert transform (which is itself
 *  implemented with FFTs from the FFTW library).
 */
class FrequencyShifter {
    
public:
    enum HilbertMode {
        // Choose Hilbert method
        fir,      /*!< FIR */
        fft       /*!< FFT*/       
    };
    /*! Construct a FrequencyShifter, which uses either an FIR or FFT.
     * 
     *  \TODO We rely on the float version (low precision) of FFTW3
     *  for all the transforms, and assume inputSample_t to be float.
     *  Make the code work for inputSample_t double or long double
     *  (e.g. using templates) is not currently supported. Additionally
     *  the double-precision version of the FFTW3 library would have to
     *  be linked.
     * 
     *  \param inputSignal The signal to be shifted
     *  \param inputSignalSize Length of the signal
     *  \param sr Sample rate of the audio
     *  \param mode FrequencyShifter.fir or FrequencyShifter.fft
     */
    FrequencyShifter(const inputSample_t* inputSignal,
                     const std::size_t inputSignalSize,
                     const double sr,
                     HilbertMode mode = HilbertMode::fir);
    
     ~FrequencyShifter();
    
    /*! Shift the input signal by the stated amount and
     *  fill shiftedSignal
     *  \param fShift Frequency by which to shift the signal (Hz)
     *  \param shiftedSignal Output buffer
     *  \param shiftedSignalSize Length of the output buffer
     */
    void shift(const parameter_t fShift,
               inputSample_t* shiftedSignal,
               const std::size_t shiftedSignalSize);
        
protected:
    /*! input signal */
    const inputSample_t* inputSignal;
    /*! input signal size */
    const std::size_t inputSignalSize;
    /*! Complex array to store results of Hilbert transform */
    std::complex<inputSample_t>* analyticSignal;
    /*! Sample rate */
    const parameter_t sr;
    
};

#endif
