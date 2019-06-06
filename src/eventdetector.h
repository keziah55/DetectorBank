#ifndef _EVENTDETECTOR_H_
#define _EVENTDETECTOR_H_

#include <vector>
#include <utility>
#include <cstdarg>

#include "detectorbank.h"
#include "onsetdetector.h"

class EventDetector {
    
public:
    
    // make delegate constructor
    // one constructor which takes ... and another
    // which takes a va_list, so args can be passed
    // in from NoteDetector without unpacking them first
    
    /*! Make an EventDetector
     *  \param sr Sample rate of input
     *  \param inputBuffer Input data
     *  \param inputBufferSize Size of input data
     *  \param offset Number of pad samples at beginning of inputBuffer
     *  \param f0 Centre frequency
     *  \param bandSize Number of detectors to form critical band around f0
     *  \param dbpSize Number of additional DetectorBank parameters being supplied
     *  \param edo Number of divisions per octave
     *  \param features Features enum containing method and normalisation. 
     *  Default is runge_kutta | freq_unnormalized | amp_normalized
     *  \param damping Detector damping. Default is 0.0001
     *  \param gain Detector gain. Default is 25.
     */
    EventDetector(const std::string path,
                  const parameter_t sr,
                  const inputSample_t* inputBuffer,
                  const std::size_t inputBufferSize,
                  const std::size_t offset,
                  const parameter_t f0,
                  const std::size_t edo,
                  const std::size_t dbpSize,
                  ...);
    
    /*! Get onsets (and pitches)
     *  \param result Vector to put onset times in
     *  \param threshold OnsetDetector threshold
     *  \param w0 OnsetDetector::analyse weight for 'count' criterion
     *  \param w1 OnsetDetector::analyse weight for 'threshold' criterion
     *  \param w2 OnsetDetector::analyse weight for 'difference' criterion
     */
    void analyse(std::vector<std::size_t> &result, double threshold);//, 
//                  double w0, double w1, double w2);

protected:
    const parameter_t sr;                 /*!< Input sample rate */
    const inputSample_t* inputBuffer;     /*!< Input buffer */
    const std::size_t inputBufferSize;    /*!< Size of input buffer */
    const parameter_t f0;                 /*!< Centre frequency */
    const std::size_t edo;                /*!< Number divisions per octave */
    const std::size_t dbpSize;            /*!< Number of additional DetectorBank parameters supplied */
    std::unique_ptr<DetectorBank> db;     /*!< DetectorBank for this band */
    std::unique_ptr<OnsetDetector> od;    /*!< OnsetDetector for this band */ 
    std::vector<parameter_t> frequencies; /*!< Vector of detector frequencies for this band */
    std::vector<parameter_t> bandwidths;  /*!< Vector of bandwidths for this band (for creating DetectorBank) */
    parameter_t bHz;                      /*!< Actual (ie non-zero) detector bandwidth (in Hz) */
    
    /*! Make array of frequencies for this band 
     */
    void makeBand();
    
    /*! Make half of the frequencies for this band
     *  \param i Indicates whether to make the frequencies below (i=-1.) or above (i=1.) centre
     */
    void makeHalfBand(double i);
       
    /*! For a given damping, get the minimum bandwidth of a detector
     *  \param damping Detector damping. Should be between 1e-4 and 5e-4
     */
    parameter_t getMinBandwidth(const parameter_t damping);
};

#endif
