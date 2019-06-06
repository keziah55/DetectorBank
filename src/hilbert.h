#ifndef _HILBERT_H_
#define _HILBERT_H_

#include <complex>

#include "detectortypes.h"

/*! Generate an analytic signal where the real part is the input signal and the
 * imaginary part is the Hilbert transform of the input.
 * 
 * Classes which use both FIR and FFT to perform the transform are provided.
*/
class HilbertTransformer {
public:    
    virtual ~HilbertTransformer() {};
    
    /*! Get analytic signal
     * \param inputSignal Signal to be transformed
     * \param analyticSignal Output array to be filled
     * \param signalSize Size of both signals
     */
    virtual void hilbert(const inputSample_t* inputSignal,
                         std::complex<inputSample_t>* analyticSignal,
                         const std::size_t signalSize) = 0;
};


class HilbertFFT: public HilbertTransformer {
public:
    /*! Get analytic signal
     * \param inputSignal Signal to be transformed
     * \param analyticSignal Output array to be filled
     * \param signalSize Size of both signals
     */
    virtual void hilbert(const inputSample_t* inputSignal,
                         std::complex<inputSample_t>* analyticSignal,
                         const std::size_t signalSize);
};

class HilbertFIR: public HilbertTransformer {
public:
    HilbertFIR(std::size_t FIRlength=19);
    virtual ~HilbertFIR();
    /*! Get analytic signal
     * \param inputSignal Signal to be transformed
     * \param analyticSignal Output array to be filled
     * \param signalSize Size of both signals
     */
    virtual void hilbert(const inputSample_t* inputSignal,
                         std::complex<inputSample_t>* analyticSignal,
                         const std::size_t signalSize);
    /*! Default FIR length is 19, but can be changed.
     *  \param length New FIRlength
     */
    void setFIRlength(std::size_t length);

protected:
    /*! length of FIR filter (must be odd) */
    std::size_t FIRlength;
    /*! kernel size */
    std::size_t kernelSize;
    /*! FIR kernel */
    inputSample_t* kernel;
    /*! make FIR kernel
     *  \param array empty array to be filled
     *  \param N size of kernel
     */
    void make_kernel(inputSample_t* array, std::size_t N);
    /*! Get value for blackman window
     *  \param n current sample 
     *  \param N window length
     */
    inputSample_t blackman(std::size_t n, std::size_t N);
};
#endif
