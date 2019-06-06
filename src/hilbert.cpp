#include <complex>
#include <cmath>
#include <fftw3.h>

#include "hilbert.h"


void HilbertFFT::hilbert(const inputSample_t* inputSignal,
                    std::complex<inputSample_t>* analyticSignal,
                    const std::size_t signalSize)
{
    fftwf_complex* intermediate_values {
        static_cast<fftwf_complex*>(
            fftwf_malloc(sizeof(fftwf_complex)*signalSize)
        )
    };
    
    fftwf_complex* analyticSignalFFTW { reinterpret_cast<fftwf_complex*>(analyticSignal)
//         static_cast<fftwf_complex*>(
//         fftwf_malloc(sizeof(fftwf_complex)*signalSize)
//         )
    };
    
    // we know that we're casting a const pointer to a non-const pointer
    // this is because fftw_preserve_input
    fftwf_plan fft {
        fftwf_plan_dft_r2c_1d(signalSize, 
                              (float*) inputSignal, 
                              intermediate_values, 
                              FFTW_ESTIMATE|FFTW_UNALIGNED|FFTW_PRESERVE_INPUT)
    };

    fftwf_plan ifft {
        fftwf_plan_dft_1d(signalSize, 
                          intermediate_values,
                          analyticSignalFFTW,
                          FFTW_BACKWARD,
                          FFTW_ESTIMATE)
    };
    
    // Perform the hilbert transform.
    // returns the analytic signal (input + j hilbert)    
  
    fftwf_execute(fft);

    // The inverse fft will scale its output by the length
    // of the input.  Take this into account while adjusting
    // the amplitude of the positive frequencies.
    intermediate_values[0][0] /= signalSize;
    intermediate_values[0][1] /= signalSize;
    // size of analyticSignalFFTW is n/2+1
    for (std::size_t i {1}; i < signalSize/2+1; i++) {
        intermediate_values[i][0] *= 2.0/signalSize;
        intermediate_values[i][1] *= 2.0/signalSize;
    }
    
    // zero negative frequencies
    float* iv {&(intermediate_values[0][0])};
    std::fill(iv+2*(signalSize/2+1), iv+2*signalSize, 0);  
   
    fftwf_execute(ifft);
    
    fftwf_free(intermediate_values);
    fftwf_destroy_plan(fft);
    fftwf_destroy_plan(ifft);
    
//     analyticSignal = reinterpret_cast<std::complex<inputSample_t>*>(analyticSignalFFTW);
    
//     fftwf_free(analyticSignalFFTW);
}

HilbertFIR::HilbertFIR(std::size_t FIRlength)
    : FIRlength {FIRlength}
    , kernelSize {2*((FIRlength+1)/4)}
    , kernel (new inputSample_t[kernelSize])
{    
//     kernelSize = 2*((FIRlength+1)/4);
//     kernel = new inputSample_t[kernelSize];
    make_kernel(kernel, kernelSize);
}

HilbertFIR::~HilbertFIR()
{
    delete[] kernel;
}

void HilbertFIR::hilbert(const inputSample_t* inputSignal,
                         std::complex<inputSample_t>* analyticSignal,
                         const std::size_t signalSize)
{
    std::size_t halfklen {FIRlength / 2};
    std::size_t koffset {0};
    std::size_t kmin, kmax;
    inputSample_t h;
    
    if (halfklen == kernelSize) {
        koffset = 1;
    }
    
    for (std::size_t n {halfklen}; n < signalSize+halfklen-1; n++) {
        kmin = std::max(static_cast<int>(n-(FIRlength-1)), 0) + koffset;
        kmax = std::min(n, signalSize-1) + koffset;
        
        h = 0.;
        
        for (std::size_t i {kmin}; i < kmax-1; i+=2) {
            h += inputSignal[i] * kernel[(n-i)/2];
        }
        
        analyticSignal[n-halfklen] = std::complex<inputSample_t>(inputSignal[n-halfklen], h);
    }
}

void HilbertFIR::make_kernel(inputSample_t* array, std::size_t N)
{
    std::size_t m {-N + 1};
    inputSample_t k;
    inputSample_t w;
    
    for (std::size_t n {0}; n < N; n++) {
        k = -(cos(m*M_PI)-1) / (m*M_PI);
        w = blackman(n, N);
        
        kernel[n] = k * w;
        kernel[N-n-1] = -k * w;
        
        m += 2;
    }
}

inputSample_t HilbertFIR::blackman(std::size_t n, std::size_t N)
{
    inputSample_t a0 = 0.42;
    inputSample_t a1 = 0.5;
    inputSample_t a2 = 0.08;
    
    inputSample_t alpha = M_PI*n/(N-1);
    
    return a0 - a1*cos(2.*alpha) + a2 * cos(4.*alpha); 
}

void HilbertFIR::setFIRlength(std::size_t length)
{
    FIRlength = length;
    kernelSize = 2*((FIRlength+1)/4);
    delete[] kernel;
    kernel = new inputSample_t[kernelSize];
    make_kernel(kernel, kernelSize);
}
