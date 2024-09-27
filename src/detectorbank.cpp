#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <complex>
#include <utility>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <cassert>
#include <string>
#include <map>
#include <stdexcept>
#include <ctime>
#include <list>
// For writing debugging files
#if (DEBUG & 2)
#   include <cstdio>
#   include <fstream>
#endif

// cereal's output has to be in a node called <profile>
// Don't move this to after the following includes, or it'll break.
#define CEREAL_XML_STRING_VALUE   "profile"
#include <cereal/cereal.hpp>
#include <cereal/access.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/types/complex.hpp>

#include "detectorbank.h"
#include "detectors.h"
#include "frequencyshifter.h"
#include "profilemanager.h"

DetectorBank::DetectorBank(const std::string& profile,
                           const inputSample_t* inputBuffer,
                           const std::size_t inputBufferSize)
: DetectorBank(48000., // need valid sample rate
               inputBuffer, inputBufferSize, 0,
               nullptr, nullptr, 0)
{
#   if DEBUG
        std::cout << "Loading profile \"" << profile <<"\", viz:\n\n"
                  << profileManager.getProfile(profile) << std::endl;
#   endif

    // load profile
    try {
        fromXML(profileManager.getProfile(profile));
    }
    catch (std::invalid_argument& e) {
        // if profile not found, throw exception
        throw std::invalid_argument(e.what());
    }

#   if DEBUG
        std::cout << "Profile updated to use at most "
                  << threadPool->threads << " threads, running "
                  << detectors.size() << " discriminators.\n";
#   endif
}

DetectorBank::DetectorBank(const parameter_t sr,
                           const inputSample_t* inputBuffer,
                           const std::size_t inputBufferSize,
                           std::size_t numThreads,
                           const parameter_t* freqs,
                           parameter_t* bw,
                           const std::size_t numDetectors,
                           int features,
                           parameter_t damping,
                           const parameter_t gain)
    : inBufSize(inputBufferSize)
    , inBuf(inputBuffer)
    , threadPool(new ThreadPool(numThreads))
    , currentSample(0)
    , d(damping)
    , sr(sr)
    , features(features)
    , bw(bw)
    , gain(gain)
    , auto_bw(false)
{
    // throw exception if sample rate is not 44100 or 48000
    std::array<parameter_t, 2> valid_sr {44100., 48000.};

    if (std::find(valid_sr.begin(), valid_sr.end(), sr) == valid_sr.end())
         throw std::invalid_argument("Sample rate should be 44100 or 48000.");

    if (bw == nullptr) {
        bw = new parameter_t[numDetectors];
        std::fill(&bw[0], &bw[numDetectors+1], 0);
        auto_bw = true;
    };

    const int solver {features & solverMask};
    const int freq_normalization {features & freqNormalizationMask};

    for (std::size_t i {0}; i < numDetectors; i++)
        if (solver == 1 && bw[i] != 0)
            throw std::invalid_argument("Central difference can only be used for minimum bandwidth detectors.");

    amplify(inBuf, inBufSize, gain);

    setDBComponents(freqs, bw, numDetectors);

//     make_scale_vectors(solver, freq_normalization);

#   if DEBUG & 1
        std::cout << "This platform will concurrently execute "
                    << threadPool->threads << " threads.\n";
#   endif

#   if DEBUG & 2
        std::cout << "Using at most " << threadPool->threads << " threads, running "
                  << numDetectors << " discriminators. "
                  << inBufSize << " input samples.\n";
#   endif

    // Allocate Detectors
    makeDetectors(numDetectors, 0, d, sr, features, gain);
}

DetectorBank::~DetectorBank()
{
    if (auto_bw)
        delete[] bw;

}

void DetectorBank::setDBComponents(const parameter_t* frequencies,
                                     const parameter_t* bandwidths,
                                     const std::size_t numDetectors)
{
    const bool make_freqs { dbComponents.empty() };

    // Choose modF based on solver and frequency normalization
    static const std::map<int,double> modFmap = {
        {runge_kutta|freq_unnormalized,        1600.},    //  24000.},//    
        {runge_kutta|search_normalized,        2200.},    //  24000.},//    
        {central_difference|freq_unnormalized, 500.},     //  24000.},//    
        {central_difference|search_normalized, 700.},     //  24000.},//    
    };

    modF = modFmap.at(features & (solverMask|freqNormalizationMask));
    
    // use FIR filter to implement Hilbert transform in FrequencyShifter
    FrequencyShifter::HilbertMode mode = FrequencyShifter::HilbertMode::fir;

    // determine modulation criterea from features
    FrequencyShifter* fs {nullptr};

    for (std::size_t i {0}; i < numDetectors; i++) {

        // if frequencies is nullptr (i.e. setDBComponents has been called by setInputBuffer,
        // rather than the constructor) the next line will fail.
        // however, setDBComponents is always called when the DetectorBank is constructed
        // so detector_components already has a list of the frequencies

        int n ( (frequencies ?
                  frequencies[i] :
                  dbComponents[i].f_in)
                / modF
        );

        if (n == 0) {
            if (make_freqs)
                dbComponents.push_back(detector_components{frequencies[i], frequencies[i],
                                                            inBuf, bandwidths[i]});
            else
                dbComponents[i].signal = inBuf;
        }

        else {
            if (fs == nullptr)
                fs = new FrequencyShifter (inBuf, inBufSize, sr, mode);

            parameter_t f_shift = - n * modF + 50.;

            if (input_pool.find(n) == input_pool.end()) {
                std::unique_ptr<inputSample_t[]> mod_sig {new inputSample_t[inBufSize]};
                input_pool[n] = std::move(mod_sig);
                fs->shift(f_shift, input_pool[n].get(), inBufSize);
            }
            if (make_freqs)
                dbComponents.push_back(detector_components{frequencies[i], frequencies[i]+f_shift,
                                                            input_pool[n].get(), bandwidths[i]});
            else
                dbComponents[i].signal = input_pool[n].get();
        }

    }
    if (fs != nullptr)
        delete fs;

}

void DetectorBank::setInputBuffer(const inputSample_t* inputBuffer,
                                  const std::size_t inputBufferSize)
{
    inBufSize = inputBufferSize;
    inBuf = inputBuffer;
    amplify(inBuf, inBufSize, gain);
    currentSample = 0;
    // Delete frequency-shifted copies of the input buffer
    input_pool.clear();
    // Re-initialise the dbComponents using the new input data
    setDBComponents(nullptr, nullptr, detectors.size());
}

void DetectorBank::amplify(const inputSample_t*& signal,
                           std::size_t signalSize,
                           const parameter_t gain)
{
    if (gain != 1.0) {
        gainBuf = std::move(
            std::unique_ptr<inputSample_t[]>(new inputSample_t[signalSize])
        );
        // Copy input signal into new "gain buffer"
        // resetting signal to point at the amplified version
        const inputSample_t* inbuf { signal };
        inputSample_t* gbuf { gainBuf.get() };
        signal = gbuf;
        while (signalSize--)
            *gbuf++ = *inbuf++ * gain;
//             *signal++ *= gain;
    }
}

void DetectorBank::makeDetectors(const std::size_t numDetectors,
                                 const parameter_t mu,
                                 const parameter_t d,
                                 const parameter_t sr,
                                 const int features,
                                 const parameter_t gain
                                )
{
    AbstractDetector *detector;

    const int solver {features & solverMask};
    assert(solver == Features::central_difference ||
           solver == Features::runge_kutta);

    const int freq_normalization {features & freqNormalizationMask};
    assert(freq_normalization == Features::freq_unnormalized     ||
           freq_normalization == Features::search_normalized);

    const int amp_normalization {features & ampNormalizationMask};
    assert(amp_normalization == Features::amp_unnormalized     ||
           amp_normalization == Features::amp_normalized);

#   if (DEBUG & 2)
        std::ofstream zf;
        zf.open("/tmp/z.dat", std::ofstream::out | std::ofstream::app);
        time_t     now = time(0);
        struct tm  tstruct;
        char       buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
        zf << "-------- " << buf << " --------\n";
#   endif

    for (std::size_t i{0}; i < numDetectors; i++) {
        const parameter_t f = dbComponents.empty() ? 0 : dbComponents[i].f_actual;
        const parameter_t f_in = dbComponents.empty() ? 0 : dbComponents[i].f_in;
        const parameter_t det_bw = dbComponents.empty() ? 0 : dbComponents[i].bandwidth;

        switch (solver & method_mask) {
        case Features::central_difference:
            detector = new CDDetector(f, mu, d, sr, det_bw, gain);
            break;
        case Features::runge_kutta:
            detector = new RK4Detector(f, mu, d, sr, det_bw, gain);
            break;
        default:
            detector = nullptr;
        }

        // Perform nomalizations for frequencies and amplitudes.
        // Currently there's only one of each. Additional types and range
        // masks should be created in detectorbank.h
        
        if (!dbComponents.empty()) {
            switch (freq_normalization & frequency_normalization_mask) {
                case Features::search_normalized:
                    // Make three test tones and iterate by best-fit
                    // parabola search to find the best response.
                    // Parameters are f0, start and end freq wrt w0,
                    // tone duration and target amplitude.
                    detector->searchNormalize(0.92, 1.08, 3.0, gain);
                    break;
            }
            switch (amp_normalization & amplitude_normalisation_mask) {
                case Features::amp_normalized:
                    detector->amplitudeNormalize(gain);
                    break;
            }
        }
        
        if (!dbComponents.empty()) 
            detector->scaleAmplitude();

        detectors.push_back(std::unique_ptr<AbstractDetector>(detector));
    }
}

int DetectorBank::getZ(discriminator_t* frames,
                       std::size_t chans, std::size_t numFrames,
                       const std::size_t startChan
                      )
{
    const size_t maxThreads {threadPool->threads}; // should probably make this an argument
    const size_t numDetectors ( detectors.size() );

#   if (DEBUG & 1)
        std::cout << "Target requests " << chans
                  << " data per frame and "
                  << numFrames << " frames from channel "
                  << startChan << std::endl
                  << "inBufSize=" << inBufSize
                  << ", currentSample=" << currentSample
                  << std::endl;
#   endif

    // Don't try to run past the end of the buffer
    // or exceed the number of available channels
    std::size_t framesToDo(std::min(numFrames, inBufSize-currentSample));
    chans = std::min(static_cast<std::size_t>(chans), numDetectors);

    // Each thread in general processes chansPerThread channels
    // but if chans isn't divisible exactly by 'maxThreads',
    // the first 'extra' threads will have to do an additional channel each.
    const std::size_t chansPerThread { chans/maxThreads };
    const std::size_t extra { chans%maxThreads };
    void* threadArgs[maxThreads];
    // Small datasets may not use all threads.
    std::size_t numThreads {0};

    for (unsigned int t {0}, startChannel {0}; t < maxThreads; t++) {
        const std::size_t chansThisThread {
            (t < extra) ? chansPerThread + 1 : chansPerThread
        };

        if (chansThisThread && framesToDo) {
#           if (DEBUG & 1)
                std::cout << "Thread " << t
                            << " starts with channel " << startChannel
                            << " for " << chansThisThread
                            << std::endl;
#           endif

            threadArgs[t] = new GetZ_params { startChannel,
                                              chansThisThread,
                                              frames,
                                              numFrames,
                                              framesToDo };
            numThreads++;
        }
        startChannel += chansThisThread;
    }

    auto delegate { [this](void* args){ getZDelegate(args); } };

#   if (DEBUG & 1)
        std::cout << "Launching getZ manifold with " <<
                    numThreads << " threads...";
#   endif

    threadPool->manifold(delegate,
                         threadArgs,
                         numThreads);

#   if (DEBUG & 1)
        std::cout << " finished\n";
#   endif

    for (size_t t {0} ; t < numThreads ; t++)
        delete static_cast<GetZ_params*>(threadArgs[t]);

    currentSample += framesToDo;

    return framesToDo;
}

void DetectorBank::getZDelegate(void* args)
{
    GetZ_params* const a = static_cast<GetZ_params*>(args);
    for ( std::size_t c {a->firstChannel} ;
          c < a->firstChannel + a->numChannels ;
          c++ ) {
        discriminator_t* const target(a->frames + a->framesPerChannel*c);
        const inputSample_t* const source(dbComponents[c].signal + currentSample);
        detectors[c]->processAudio(target, source, a->numFrames);
    }
}

result_t DetectorBank::absZ(result_t* absFrames,
                            std::size_t absChans,
                            std::size_t absNumFrames,
                            discriminator_t* frames,
                            std::size_t maxThreads
                       ) const
{
    auto absZDelegate =
        [absFrames, frames](void* args) {
            const AbsZ_params* a { static_cast<AbsZ_params*>(args) };
            for (std::size_t i{a->start}; i < a->end; i++)
               a->mx = std::max((absFrames[i] = std::abs(frames[i])), a->mx);
        };

    const std::size_t dataPoints { absChans * absNumFrames };
    // How many threads to start?
    const std::size_t numThreads {
        // Now, pay attention.
        // The largest number of threads is the minumum of the number
        // the object has mandated (maxThreads) and the number available
        // from the threadPool. 0 in maxThreads means "let the ThreadPool
        // decide what's best". But for very small data sets, absChans
        // might be even smaller than that, so the number of threads is
        // reduced further.
        std::min(absChans,
                 maxThreads == 0 ? threadPool->threads :
                                   std::min(maxThreads, threadPool->threads))

    };
    void* threadArgs[numThreads];
    result_t maxVals[numThreads] = { }; // Initially 0

    for (std::size_t t{0}; t < numThreads; t++)
        threadArgs[t] = new AbsZ_params { t*dataPoints/numThreads,
                                          (t+1)*dataPoints/numThreads,
                                          std::ref(maxVals[t]) };

    result_t overallMax { 0.0 };

#   if (DEBUG & 1)
        std::cout << "Launching absZ manifold with "
                  << numThreads << " threads...";
#   endif
                  
    threadPool->manifold(absZDelegate, threadArgs, numThreads);
    
#   if (DEBUG & 1)
        std::cout << " finished\n";
#   endif
        
    for (std::size_t th{0}; th < numThreads; th++) {
        overallMax = std::max(overallMax, maxVals[th]);
        delete static_cast<AbsZ_params*>(threadArgs[th]);
    }

    return overallMax;
}

const std::map<int, std::string> DetectorBank::featuresToStringMap {
        {{central_difference}, {"Central difference method"}},
        {{runge_kutta},        {"Runge-Kutta method"}},
        {{freq_unnormalized},  {"Frequency unnormalized"}},
        {{search_normalized},  {"Search-normalized"}},
        {{amp_unnormalized},   {"Amplitude unnormalized"}},
        {{amp_normalized},     {"Amplitude normalized"}}
};

void DetectorBank::stringToFeatures(const std::string& desc) {
    std::istringstream featureList(desc);
    std::string feature;
    int result {0};
    while (!featureList.eof()) {
        // Read a feature from the human-readable string
        std::getline(featureList, feature, ',');
        // Search for and or-in the appropriate bit from featuresToStringMap
        bool matched {false};
        for (std::pair<int, std::string>nvp : featuresToStringMap) {
            if (nvp.second == feature) {
                std::cout << "Found " << nvp.second << " (" << nvp.first << ")\n";
                result |= nvp.first;
                matched = true;
                break;
            }
        }
        std::cout << result << std::endl;
        if (!matched)
            throw std::runtime_error("Illegal feature name reading XML profile");
    }

    if (result == 0)
        throw std::runtime_error("No valid features in feature list reading XML profile");

    features = static_cast<Features>(result);
};

const std::string DetectorBank::featuresToString(void) const {
    const int solver {features & solverMask};
    const int freq_normalization {features & freqNormalizationMask};
    const int amp_normalization {features & ampNormalizationMask};

    // Can't use operator[] on a const map because that operator
    // creates the referenced entry if it doesn't exist. The at() method
    // throws an exception instead.
    std::string solverName, freqNormalizationName, ampNormalizationName;
    try {
        solverName = featuresToStringMap.at(solver);
    } catch (...) {
        solverName = "[Unknown solver]";
    }

    try {
        freqNormalizationName = featuresToStringMap.at(freq_normalization);
    } catch (...) {
        freqNormalizationName = "[Unknown frequency normalization method]";
    }

    try {
        ampNormalizationName = featuresToStringMap.at(amp_normalization);
    } catch (...) {
        ampNormalizationName = "[Unknown amplitude normalization method]";
    }

    return  solverName + "," + freqNormalizationName + "," + ampNormalizationName;
};


std::string DetectorBank::toXML(void) const
{
    std::ostringstream xml;
    {
        cereal::XMLOutputArchive archive(xml);
        save(archive);
    }
    return xml.str();
}

template<class Archive> void DetectorBank::save(Archive& archive) const
{
    const std::string featureSet { featuresToString() };
    const cereal::size_type numDetectors(detectors.size());

    archive(CEREAL_NVP(sr),
            CEREAL_NVP(d),
            cereal::make_nvp("maxThreads", threadPool->threads),
            CEREAL_NVP(featureSet),
            CEREAL_NVP(gain),
            CEREAL_NVP(numDetectors)
           );


    archive.setNextName("Detectors");
    archive.startNode();
    for (cereal::size_type i{0}; i<numDetectors; i++) {

        archive.setNextName("Detector");
        archive.startNode();
        archive(cereal::make_nvp("w_in", dbComponents[i].f_in * 2.0*M_PI));
        archive(cereal::make_nvp("bw", dbComponents[i].bandwidth));
        detectors[i]->save(archive);
        archive.finishNode();

    }
    archive.finishNode();

}

void DetectorBank::fromXML(std::string xml)
{
    std::istringstream xml_is(xml);
    {
        cereal::XMLInputArchive archive(xml_is);
        load(archive);
    }
}

void DetectorBank::saveProfile(std::string name)
{
    profileManager.saveProfile(name, toXML());
}

std::list<std::string> DetectorBank::profiles()
{
    return profileManager.profiles();
}

template<class Archive> void DetectorBank::load(Archive& archive)
{
    std::string featureSet;
    size_t threads;
    archive(sr, d, threads, featureSet, gain);
    threadPool = std::unique_ptr<ThreadPool>(new ThreadPool(threads));
    stringToFeatures(featureSet);

    cereal::size_type numDetectors;
    archive(numDetectors);


    // Create the right sort of detectors,
    // then restore their state later.
    dbComponents.clear();
    detectors.clear();
//     std::cout << "I'm going to load " << numDetectors << " detectors\n";
    makeDetectors(numDetectors, 0, d, sr, features, gain);
    std::cout << "There are now " << detectors.size() << " detector(s)\n";

    // Reload all of the working parameters for each detector
    archive.setNextName("Detectors");
    archive.startNode();
    parameter_t freqs[numDetectors];
    parameter_t bw [numDetectors];
    for (cereal::size_type i{0}; i<numDetectors; i++) {
        archive.setNextName("Detector");
        archive.startNode();
        archive(cereal::make_nvp("w_in", freqs[i]));
        freqs[i] /= 2.0*M_PI;
        archive(cereal::make_nvp("bw", bw[i]));
        detectors[i]->load(archive);
        archive.finishNode();
    }
    archive.finishNode();

    // We're going to have to rebuild the dbComponents vector
    // from the raw frequencies and bandwdiths now.
    setDBComponents(freqs, bw, numDetectors);
}

bool DetectorBank::seek(long int offset) {
    bool result;
    if (offset >= 0 && static_cast<std::size_t>(offset) < inBufSize) {
        currentSample = offset;
        result = true;
    } else if (offset < 0 && static_cast<std::size_t>(-offset) <= inBufSize) {
        currentSample = inBufSize + offset;
        result = true;
    } else {
        result = false;
    }

    // if seeking the beginning of the audio, reset all the previous values to 0
    if (offset == 0) {
        for (std::size_t i {0}; i < detectors.size(); i++)
            detectors[i]->reset();
    }

    return result;
}

parameter_t DetectorBank::getW(std::size_t ch) const {
    return (ch < 0 || static_cast<unsigned>(ch) >= detectors.size())
        ? 0
        : detectors[ch]->getW();
}

parameter_t DetectorBank::getFreqIn(std::size_t ch) const {
    return (ch < 0 || static_cast<unsigned>(ch) >= detectors.size())
        ? 0
        : dbComponents[ch].f_in;
}



ProfileManager DetectorBank::profileManager;

#include "pitches.inc"

