#include <vector>
#include <utility>
#include <cstdarg>

#include <string>
#include <sstream>

#include "detectorbank.h"
#include "eventdetector.h"
#include "onsetdetector.h"

EventDetector::EventDetector(const std::string path,
                             const parameter_t sr,
                             const inputSample_t* inputBuffer,
                             const std::size_t inputBufferSize,
                             const std::size_t offset,
                             const parameter_t f0,
                             const std::size_t edo,
                             const std::size_t dbpSize,
                             ...)
    : sr(sr)
    , inputBuffer(inputBuffer)
    , inputBufferSize(inputBufferSize)
    , f0(f0)
    , edo(edo)
    , dbpSize(dbpSize)
{
    
//     for (std::size_t i {0}; i<inputBufferSize; i++)
//         std::cout << "inputBuffer[" << i << "]: " << inputBuffer[i] << "\n";
    
    // DetectorBank parameters
    parameter_t bandwidth;
    int features;
    parameter_t damping;
    parameter_t gain;
    
    // get whatever extra args were supplied
    va_list args;
    va_start(args, dbpSize);
    bandwidth = va_arg(args, parameter_t);
    features = va_arg(args, int);
    damping = va_arg(args, parameter_t);
    gain = va_arg(args, parameter_t);
    va_end(args);
    
    // If minimum bandwidth detectors are requested, bandwidth will be zero
    // However, when constructing critical bands, we need to know the actual
    // value of the minimum bandwidth in Hertz.
    // This depends on the damping and sample rate.
    if ( bandwidth == 0. )
        bHz = getMinBandwidth(damping);
    else
        bHz = bandwidth;
    
    // populate 'frequencies' array for this band
    makeBand();
    
    // get number of detectors in this band
    std::size_t bandSize = frequencies.size();
    
    // populate 'bandwidths' array for this band to be passed to DetectorBank
    // use given bandwidth value, not calculated (i.e. zero for minimum bandwidth)
    bandwidths.resize(bandSize, bandwidth);
    
    // make DetectorBank
    db.reset(new DetectorBank(sr, inputBuffer, inputBufferSize, 4,
                              &frequencies[0], &bandwidths[0], bandSize,
                              static_cast<DetectorBank::Features>(features),
                              damping, gain));
    
    std::ostringstream oss;
    oss << path << "/" << static_cast<int>(f0) << "Hz"; //_debug.txt"; // "Hz.csv";
    std::string csvname = oss.str();
    
    // make OnsetDetector
    od.reset(new OnsetDetector(*db, offset, csvname));
}

void EventDetector::makeBand()
{
    // start by getting the frequencies below centre
    double i { -1. };
    makeHalfBand(i);
    
    // insert the centre frequency into the array
    frequencies.push_back(f0);
    
    // get the frequencies above centre
    i = 1.;
    makeHalfBand(i);
}

void EventDetector::makeHalfBand(double i)
{
    parameter_t f { f0 }; // start frequency
    parameter_t f1 { f0 * std::pow(2., i/(2.*edo)) }; // stop freq 
    
    // difference between current and stop frequency
    // multiplying by i should make it positive
    parameter_t diff {i*(f1-f)};
    
    // until diff is at minimum or f is within b/4 of stop value
    // ( -5 <= -6 evaluates to False, i.e. this will get as close to zero
    // as possible)
    while ( i*(f1-f) <= diff && i*(f1-f) > (bHz/4.) ) {
        diff = i*(f1-f); // calculate diff before changing f
        f += (i*bHz); // when i = -1, b will be subtracted
        frequencies.push_back(f);
    }
}

parameter_t EventDetector::getMinBandwidth(const parameter_t damping)
{
    // vector of damping factors
    static const std::vector<parameter_t> d { 1e-4, 2e-4, 3e-4, 4e-4, 5e-4 };
    // vector for minimum bandwidths
    std::vector<parameter_t> b;
    
    // set b vector depending on given sample rate
    if ( sr == 44100. )
        b = { 0.850, 1.688, 2.528, 3.360, 4.200 };
    else if ( sr == 48000. )
        b = { 0.922, 1.832, 2.752, 3.660, 4.860 };
    else
        throw std::invalid_argument("Sample rate should be 44100 or 48000.");
    
    if ( damping < d[0] || damping > d[4] )
        throw std::invalid_argument("Damping must be between 1e-4 and 5e-4.");

    parameter_t diff;
    parameter_t nearest { 1. };
    std::size_t idx0;
    std::size_t idx1;
    
    // Compare the given damping with every value in damping vector
    // to find the closest value
    // If the closest value is bang-on, use the corresponding bandwidth
    // Otherwise, interpolate between the two closest damping values
    for (std::size_t i {0}; i < d.size(); i++) {
        
        // get absolute difference between given damping a current array value
        diff = std::abs(d[i] - damping);
        
        // if the given damping is in the vector, return the corresponding bandwidth
        if ( diff == 0. )
            return b[i];
        
        // if this is less than the stored minimum difference, replace this and 
        // store the index
        if ( diff < nearest ) {
            nearest = diff;
            idx0 = i;
        }
    }
    
    // get index of other damping factor to interpolate between 
    if ( damping >  d[idx0] )
        idx1 = idx0 + 1;
    else
        idx1 = idx0 - 1;
    
    // interpolate between the two closest damping factors
    // NB if damping < d[idx0], both numerator and denominator of p will be -ve
    parameter_t p { (damping-d[idx0]) / (d[idx1]-d[idx0]) };
    parameter_t bandwidth { b[idx0] + p * (b[idx1] - b[idx0]) };
    
    return bandwidth;
}

void EventDetector::analyse(std::vector<std::size_t> &result,
                            double threshold)//, double w0,
//                             double w1, double w2)
{
    // call OnsetDetector's analyse method
    od->analyse(result, threshold);//, w0, w1, w2);
}
