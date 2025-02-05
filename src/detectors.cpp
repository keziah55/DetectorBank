#include <complex>
#include <utility>
#include <memory>
#include <string>
#include <typeinfo>
#include <string>

#include <iostream>

// For writing debugging files
#if (DEBUG & 2)
#   include <cstdio>
#   include <fstream>
#endif

#include "detectors.h"

AbstractDetector::AbstractDetector(parameter_t f, parameter_t mu, 
                                   parameter_t d, parameter_t sr, 
                                   parameter_t detBw, parameter_t gain)
    : w(f*2.0*M_PI)
    , mu(mu)
    , d(d)
    , sr(sr)
    , detBw(detBw)
    , gain(gain)
    , aScale(discriminator_t(1,0))
    , iScale(1.0)
    , scale(discriminator_t(1,0))
{
}

AbstractDetector::~AbstractDetector()
{
}

void AbstractDetector::processAudio(discriminator_t* target,
                                    const inputSample_t* start, std::size_t count)
{
    // perform Hopf bifurcation calculation
    process(target, start, count);
    
    // combine amplitude scaling and amplitude normalisation factor into single value
    const discriminator_t totalScale {scale * aScale};

    for (std::size_t i{0}; i < count; i++) {
        *target *= aScale; // totalScale;
        
        const auto re { std::real(*target) };
        const auto im { std::imag(*target) };
        *target = discriminator_t(re, im*iScale); // correcting eccentricity
        
        target++;
    }
}

const parameter_t AbstractDetector::getLyapunov(const parameter_t bw, const parameter_t amp)
{
    // get first Lyapunov coefficient for given bandwidth and amplitude by scaling
    // empirical values found when amplitude=25
    
    parameter_t b;
    
    if (bw > 0.0) {
        b = -12.5 * pow(bw,3) / pow(amp, 2);
    } else if (bw < 0.0)
        throw std::invalid_argument("Desired bandwidth should be non-negative.");
    else b = 0.0;
    
    return b;
}

void AbstractDetector::generateTone(inputSample_t* tone,
                                    const std::size_t duration,
                                    const parameter_t frequency)
{
    inputSample_t theta(0);
    const inputSample_t wPerS(2.*M_PI*frequency/sr);
    
    for (std::size_t i{0}; i < duration; i++) {
        tone[i] = sin(theta);
        theta += wPerS;
        theta = fmod(theta, 2.*M_PI);
    }
}

bool AbstractDetector::searchNormalize(parameter_t searchStart,
                                       parameter_t searchEnd,
                                       const parameter_t toneDuration,
                                       const parameter_t forcingAmplitude)
{
    nrml = true;
    
    // Specified detector frequency
    const parameter_t f_spec { w/(2.0*M_PI) };
    // The frequency actually required to achieve that.
    parameter_t f { f_spec };
    
    DetectorBank::Features method;
    
    // Determine this's numerical method
    if (typeid(*this) == typeid(CDDetector))
        method = DetectorBank::Features::central_difference;
    else if (typeid(*this) == typeid(RK4Detector))
        method = DetectorBank::Features::runge_kutta;
    else
        throw std::runtime_error(
            "Invalid detector type while attempting search-normalization"
        );
        
    const std::size_t samples { static_cast<std::size_t>(toneDuration*sr) };
    inputSample_t tone[samples];
    generateTone(tone, samples, f);

    std::unique_ptr<discriminator_t[]> results(new discriminator_t[3*samples]);

    // Initial search parameters
    parameter_t testFreq[] { searchStart * f, searchEnd * f, f };
    
    std::size_t test_len = sizeof(testFreq)/sizeof(testFreq[0]);
    parameter_t test_bw[test_len];
    std::fill_n(test_bw, test_len, detBw);  
    
    int iteration{0};     // Number of attempts so far
    
    
    
    // On the first iteration, check that the target
    // frequency produces greater output than the upper
    // and lower estimates. If the upper and lower
    // estimates do not span the target frequency,
    // immediately return failure
    std::unique_ptr<DetectorBank> db(
        new DetectorBank(sr, tone, samples, 3, testFreq, test_bw, 3,
                         static_cast<DetectorBank::Features>(
                            method|DetectorBank::Features::freq_unnormalized|
                            DetectorBank::Features::amp_unnormalized
                         ), d, forcingAmplitude)
    );    
    db->getZ(results.get(), 3, samples);

    // Look from 75% to 90% of the tone to find max amplitudes
    const std::size_t testFrom { 3*samples / 4 };
    const std::size_t testTo { 9*samples / 10 };
    double amplitudes[] = {0.0, 0.0, 0.0};
    
    // Results contains the output from all the detectors,
    // with a stride of samples. We'll loop though the outputs
    // keeping the largest magnitude. In the case of the
    // first detector, we'll keep the index of the biggest
    // magnitude too, so it can be used for gain normalization
    // and stiffness correction later.
    for (std::size_t i{testFrom}; i < testTo; i++) {
        for (std::size_t j{0}; j < 3; j++) {
            amplitudes[j] =
                std::max(std::abs(results[j*samples + i]), amplitudes[j]);
        }
    }

    if (amplitudes[0] > amplitudes[2] || amplitudes[1] > amplitudes[2]) {
        std::cout << "Searching for normalized characteristic frequency: "
                "test range does not span maximum response.\n";
        return false;
    }
    
    // Now iterate to find best response. On each iteration,
    // the search range is reduced by moving the frequency
    // with the lowest amplitude response half way towards
    // the centre frequency
#   if DEBUG & 2 
    std::ofstream zf;
#   endif
    while (
        iteration++ < maxNormIterations &&
        testFreq[1]/testFreq[0] > normConverged
    ) {
        const double cf = 0.5*(testFreq[0] + testFreq[1]);
        if (amplitudes[0] < amplitudes[1])
            testFreq[0] = 0.5*(testFreq[0] + cf);
        else
            testFreq[1] = 0.5*(testFreq[1] + cf);
        
        // Get a new bunch of detectors
        db.reset(
            new DetectorBank(sr, tone, samples, 2, testFreq, test_bw, 2, 
                             static_cast<DetectorBank::Features>(
                                method|DetectorBank::Features::freq_unnormalized|
                                DetectorBank::Features::amp_unnormalized),
                             d, forcingAmplitude)
        );
        
        // Perform new analysis runs using just the upper and lower bounds
        db->getZ(results.get(), 2, samples);
        
        // Search for maxima.
        amplitudes[0] = amplitudes[1] = 0.0;
        for (std::size_t i{testFrom}; i < testTo; i++) {
            for (std::size_t j{0}; j < 2; j++) {
                amplitudes[j] =
                    std::max(std::abs(results[j*samples + i]), amplitudes[j]);
            }
        }
        
        f = 0.5 * (testFreq[0] + testFreq[1]);
        w = 2*M_PI*f;
        
#       if DEBUG & 2 
        zf.open("/tmp/z.dat", std::ofstream::out | std::ofstream::app);
    
        zf << "===== ITERATION " << iteration << std::endl;
        zf << "frequencies: "
            << testFreq[0] << ", "
            << testFreq[1] << std::endl;
        zf << "amplitudes:  "
            << amplitudes[0] << ", "
            << amplitudes[1] << std::endl;
        zf << "Ratio of upper to lower test tone frequency: "
            << testFreq[1]/testFreq[0] << std::endl;
#       endif
    }
    
#   if DEBUG & 2     
    zf << "=== Best correction:\n\tUse f=" << f
    << " (for f_spec=" << f_spec << ");" << std::endl;
#   endif
        
    return true;
}
    
bool AbstractDetector::amplitudeNormalize(const parameter_t forcingAmplitude)
{
    // make tone and detector at detector frequency (3 seconds)
    const std::size_t dur {60};
        
    const std::size_t samples { static_cast<std::size_t>(dur*sr) };
    
    std::unique_ptr<inputSample_t[]> tone(new inputSample_t[samples]);

    const parameter_t f { w/(2.0*M_PI) };
    
    generateTone(&tone[0], samples, f);
    
    DetectorBank::Features method;
    
    // Determine this's numerical method
    if (typeid(*this) == typeid(CDDetector))
        method = DetectorBank::Features::central_difference;
    else if (typeid(*this) == typeid(RK4Detector))
        method = DetectorBank::Features::runge_kutta;
    else
        throw std::runtime_error(
            "Invalid detector type while attempting amplitude normalization"
        );
    
    parameter_t test_bw[] {detBw};
    
    // make a DetectorBank with the same method and f_norm and damping
    std::unique_ptr<DetectorBank> db(
        new DetectorBank(sr, &tone[0], samples, 1, &f, test_bw, 1, 
                         static_cast<DetectorBank::Features>(
                            method|DetectorBank::Features::freq_unnormalized|
                            DetectorBank::Features::amp_unnormalized
                         ), d, forcingAmplitude)
    );
    
    std::unique_ptr<discriminator_t[]> results(new discriminator_t[samples]);
        
    db->getZ(results.get(), 1, samples);
    
    // find index where |z| is at max
    parameter_t z_max = 0.0;
    std::size_t where;
    for (std::size_t i{0}; i < samples; i++) {
        if (z_max < std::abs(results[i])) {
            z_max = std::abs(results[i]);
            where = i;
        }
    }

    // (complex) amplitude normalisation factor
    aScale = 1. / results[where];

    // number of oscillations over which to find eccentricity
    const std::size_t nOsc {5};
    // number of samples in nOsc oscillations
    const std::size_t sOsc {static_cast<std::size_t>(sr * nOsc/f)};
        
    // find eccentricity (if orbits aren't circular)
    parameter_t mxim{0.}, mxre{0.};
    for (std::size_t i{samples-sOsc}; i < samples; i++) {
        const discriminator_t z { results[i] * aScale };
        if ( std::abs(z.imag()) > std::abs(mxim) ) mxim = z.imag();
        if ( std::abs(z.real()) > std::abs(mxre) ) mxre = z.real();
    }
    iScale = mxre/mxim;
    
    // z values will be scaled by a and iScale
    
#   if DEBUG & 2      
    std::ofstream zf;
    zf.open("/tmp/z.dat", std::ofstream::out | std::ofstream::app);
    zf << "\n\tnormalisation " << aScale.real() << "+j(" << aScale.imag()
        << "),\n\teccentricity " << iScale << std::endl;
#   endif
    
    return true;
}

void AbstractDetector::scaleAmplitude() {
    makeScaleVectors();
    getScaleValue(w/(2.0*M_PI));
}

    
void AbstractDetector::makeScaleVectors() {
    // ScaleFreqs and ScaleFactors come from scale_values.inc
    
    std::string method;
    
    // Determine this's numerical method
    if (typeid(*this) == typeid(CDDetector))
         method = "CD";
    else if (typeid(*this) == typeid(RK4Detector))
        method = "RK4";
    else
        throw std::runtime_error(
            "Invalid detector type while attempting amplitude scaling");
        
    int offset;
    
    if (sr==44100.)
        offset = 0;
    else
        offset = 4;
    
    // unnormalised RK4
    if (method == "RK4" && !nrml) {
        detScaleFreqs = scaleFreqs[0+offset];
        detScaleFactors = scaleFactors[0+offset];
    }
    // normalised RK4
    else if (method == "RK4" && nrml) {
        detScaleFreqs = scaleFreqs[1+offset];
        detScaleFactors = scaleFactors[1+offset];
    }
    // unnormalised CD
    else if (method == "CD" && !nrml) {
        detScaleFreqs = scaleFreqs[2+offset];
        detScaleFactors = scaleFactors[2+offset];
    }
    // normalised CD
    else if (method == "CD" && nrml) {
        detScaleFreqs = scaleFreqs[3+offset];
        detScaleFactors = scaleFactors[3+offset];
    }
}

void AbstractDetector::getScaleValue(const parameter_t fr) {
    
    discriminator_t scl {1};
    std::size_t i {0};
    parameter_t ratio {1};

//     for (std::size_t i {0}; i < detScaleFreqs.size()-1;  i++) {
    while (true) {

        // if frequency is in vector, get the scale factor
        if (fr == detScaleFreqs[i])
            scl = detScaleFactors[i];

        // otherwise, interpolate between the nearest two frequencies
        else if ((fr > detScaleFreqs[i]) && (fr < detScaleFreqs[i+1])) {

            ratio = (fr - detScaleFreqs[i]) / (detScaleFreqs[i+1] - detScaleFreqs[i]);

            scl = detScaleFactors[i] + (ratio * (detScaleFactors[i+1] - detScaleFactors[i]));
            
            break;
        }
        if (i >= detScaleFreqs.size()-1)
            break;
        
        i++;
    }
    scale = 1./scl;
}




CDDetector::CDDetector(parameter_t f, parameter_t mu, 
                       parameter_t d, parameter_t sr, 
                       parameter_t detBw, parameter_t gain)
    : AbstractDetector(f, mu, d, sr, detBw, gain)
    , zp(0), zpp(0), xp(0)
{
    b = 0;
}

CDDetector::~CDDetector()
{
}

void CDDetector::reset()
{
    zp = 0;
    zpp = 0;
    xp = 0;
}

void CDDetector::process(discriminator_t* target,
                         const inputSample_t* start, const std::size_t count)
{    
    for (std::size_t i{0}; i < count; i++) {
        const std::complex<double> result { 
            (((mu + std::complex<double>(0,1) * w) * zp
             + b * std::abs(zp*zp) * zp 
             + static_cast<double>(xp)) 
             * 2.0/sr + zpp)
             * (1.-d)
        };
        
        zpp = zp;
        zp = *target++ = result;
        
        xp = *start++;
    }
}

RK4Detector::RK4Detector(parameter_t f, parameter_t mu, 
                         parameter_t d, parameter_t sr, 
                         parameter_t detBw, parameter_t gain)
    : AbstractDetector(f, mu, d, sr, detBw, gain)
    , zp(0), zpp(0), xp(0), xpp(0)
{
    b = getLyapunov(detBw, gain);
}

RK4Detector::~RK4Detector()
{
}

void RK4Detector::reset()
{
    zp = 0;
    zpp = 0;
    xp = 0;
    xpp = 0;
}

void RK4Detector::process(discriminator_t* target,
                          const inputSample_t* start, const std::size_t count)
{   
    auto dzdt = [&] (const std::complex<double> z, const double x)
    {
        return (mu + std::complex<double>(0,1) * w) * z 
                + b * std::abs(z*z) * z 
                + x;
    };
    
    for (std::size_t i{0}; i < count; i++) {
        const std::complex<double> u0 {zpp};
        const std::complex<double> k0 {dzdt(u0, xpp)};
        
        const std::complex<double> u1 {u0 + k0/sr};
        const std::complex<double> k1 {dzdt(u1, xp)};
            
        const std::complex<double> u2 {u0 + k1/sr};
        const std::complex<double> k2 {dzdt(u2, xp)};
        
        const std::complex<double> u3 {u0 + k2 * 2.0/sr};
        const std::complex<double> k3 {dzdt(u3, *start)};
            
        zpp = zp;
        zp = *target++ = (u0 + (k0 + 2.0*k1 + 2.0*k2 + k3)/(3.*sr)) * (1.-d);

        xpp = xp;
        xp = *start++;
    }
}

#include "scale_values.inc"
