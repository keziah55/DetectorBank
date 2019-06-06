#include <complex>
#include <algorithm>
#include <cmath>
#include <iostream>

#include "frequencyshifter.h"
#include "hilbert.h"
#include "detectortypes.h"

FrequencyShifter::FrequencyShifter(const inputSample_t* inputSignal,
                                   const std::size_t inputSignalSize,
                                   const parameter_t sr,
                                   HilbertMode mode)
    : inputSignal(inputSignal)
    , inputSignalSize(inputSignalSize)
    , sr(sr)
{
    HilbertTransformer *transformer;

    analyticSignal = new std::complex<inputSample_t>[inputSignalSize];

    switch (mode) {
        case HilbertMode::fir:
            transformer = new HilbertFIR();
            break;
        case HilbertMode::fft:
            transformer = new HilbertFFT();
            break;
        default:
            throw std::invalid_argument("FrequencyShifter mode should be FIR or FFT");
    }

    transformer->hilbert(inputSignal, analyticSignal, inputSignalSize);
    
    delete transformer;
}

FrequencyShifter::~FrequencyShifter()
{
    delete[] analyticSignal;
}

void FrequencyShifter::shift(const parameter_t fShift,
                             inputSample_t* shiftedSignal,
                             const std::size_t shiftedSignalSize)
{
    std::complex<parameter_t> c(1.0);
    const std::complex<parameter_t> phase_inc {
        std::exp(std::complex<parameter_t>(0., fShift*2.0*M_PI / sr))
    };
    
    for (std::size_t i {0}; i < shiftedSignalSize; i++) {
        
        shiftedSignal[i] =
            real(analyticSignal[i])*real(c) - imag(analyticSignal[i])*imag(c);

        c *= phase_inc;
    }
}
