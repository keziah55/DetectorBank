#include <vector>
#include <map>
#include <utility>
#include <thread>
#include <cstring>

#include <string>
#include <iostream>
#include <fstream>

#include "eventdetector.h"
#include "notedetector.h"
#include "detectorbank.h"

const std::string NoteDetector::default_path {"log"};

NoteDetector::NoteDetector(const parameter_t sr,
                           const inputSample_t* inputBuffer,
                           const std::size_t inputBufferSize,
                           const parameter_t* freqs,
                           const std::size_t freqsSize,
                           const std::size_t edo,
                           const parameter_t bandwidth,
                           const int features,
                           const parameter_t damping,
                           const parameter_t gain,
                           const std::string path
                          )
    : sr(sr)
    , freqs(freqs)
    , freqsSize(freqsSize)
    , edo(edo)
    , bandwidth(bandwidth)
    , features(features)
    , damping(damping)
    , gain(gain)
    , threadPool(new ThreadPool(0)) // default number of threads
{
std::cout << "sr = " << sr << " freqsize = " << freqsSize << " edo = " << edo << " bandwidth = " << bandwidth << " features = " << features << " damping = " << damping << " gain = " << gain << std::endl;
    // zero pad the beginning of the audio
    const std::size_t offset {static_cast<std::size_t>(sr)/4};
    inputBufferPadSize = inputBufferSize+offset;
    inputBufferPad = new inputSample_t[inputBufferPadSize];
    
    // inputBufferSize * sizeof inputBuffer[0] is size of inputBuffer in bytes
    std::memset(inputBufferPad, 0, offset * sizeof inputBuffer[0]);
    std::memcpy(inputBufferPad+offset, inputBuffer, 
                inputBufferSize * sizeof inputBuffer[0]);
    
    // make an EventDetector for each requested frequency
    EventDetector *eventdetector;
    
    for (std::size_t i {0}; i < freqsSize; i++) {
        // make EventDetectors and put in vector
        eventdetector = new EventDetector(path, sr, inputBufferPad, inputBufferPadSize,
                                          offset, freqs[i], edo, 0,
                                          bandwidth, features, damping, gain);
        eventdetectors.push_back(std::unique_ptr<EventDetector>(eventdetector));
    }
}

NoteDetector::NoteDetector(const parameter_t sr,
                           const inputSample_t* inputBuffer,
                           const std::size_t inputBufferSize,
                           const parameter_t* freqs,
                           const std::size_t freqsSize,
                           const std::size_t edo,
                           const NDOptArgs& args)
    : NoteDetector(sr, inputBuffer, inputBufferSize,
                   freqs, freqsSize, edo,
                   args.nd_bandwidth,
                   args.nd_features,
                   args.nd_damping,
                   args.nd_gain,
                   args.nd_path
                  )
{}
        

NoteDetector::~NoteDetector()
{
    delete[] inputBufferPad;
}

void NoteDetector::analyse(Onsets_t &onsets, double threshold)//,
//                            double w0, double w1, double w2)
{
    const std::size_t maxThreads {threadPool->threads};

    // may be asked for fewer critical bands than threads
    const std::size_t numThreads {std::min(freqsSize, maxThreads)};

    // (minimum) number of bands for each thread to analyse
    const std::size_t bandsPerThread { freqsSize/maxThreads };
    const std::size_t extra { freqsSize%maxThreads };

    // array of parameters to be passed to delegate method
    void* threadArgs[numThreads];
    
    // Array of vectors for onsets
    // A pointer to this array is given to each Analyse_params struct
    // The vector of found onsets is then put in the array in the right place
    // After the threads have run, non-empty vectors are put into the onsets map
    std::vector<std::size_t> temp[freqsSize];

    for (std::size_t t {0}, startBand {0}; t < numThreads; t++) {

        // get thread to analyse one more band, if necessary
        const std::size_t bandsThisThread {
            (t < extra) ? bandsPerThread + 1 : bandsPerThread
        };

        threadArgs[t] = new Analyse_params { temp,
                                             startBand, bandsThisThread,
                                             threshold };//, w0, w1, w2 };
        startBand += bandsThisThread;
    }

    auto delegate { [this](void* args){ analyseDelegate(args); } };

    // run the threads...
    threadPool->manifold(delegate, threadArgs, numThreads);

    // delete everything that was newed to make the array of args
    for (std::size_t t {0}; t < numThreads; t++)
        delete static_cast<Analyse_params*>(threadArgs[t]);
        
    // go through array of vectors
    // non-empty vectors are put in the map of frequency index : onsets vector
    for (std::size_t i {0}; i<freqsSize; i++) {
        if (!temp[i].empty())
            onsets.insert({i, std::move(temp[i])});
    }
}

void NoteDetector::analyseDelegate(void* args)
{
    Analyse_params* const a = static_cast<Analyse_params*>(args);

    // loop through bands given in args struct
    for (std::size_t i {a->firstBand}; i < a->firstBand+a->numBands; i++) {
        
        // call the 'analyse' method of the EventDetector for this band
        std::vector<std::size_t> result;
        try {
            eventdetectors[i]->analyse(result, a->threshold);//, a->w0, a->w1, a->w2);
        } catch (slidingbuffer::SlidingBufferException &e) {
            throw slidingbuffer::SlidingBufferException(e.what(), e.index);
        }
        
        // put vector of onsets into array
        a->temp[i] = std::move(result);
    }
}

