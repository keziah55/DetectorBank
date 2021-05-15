#ifndef _NOTEDETECTOR_H_
#define _NOTEDETECTOR_H_

#include <vector>
#include <utility>
#include <map>

#include "detectortypes.h"
#include "detectorbank.h"
#include "eventdetector.h"
#include "thread_pool.h"


class NDOptArgs;

/*! Note detector 
 */
class NoteDetector {
    
public:

    /*! Argument specifiers for the named-properties constructor */
    class OptArgs;
    
    /*! Default detector bandwidth */
    static constexpr parameter_t default_bandwidth {0};
    /*! Default detector features */
    static constexpr int default_features {
        DetectorBank::Features::runge_kutta |
        DetectorBank::Features::freq_unnormalized |
        DetectorBank::Features::amp_unnormalized
    };
    /*! Default damping for detectors */
    static constexpr parameter_t default_damping {0.0001};
    /*! Default detector gain */
    static constexpr parameter_t default_gain {50};
    /* TODO Document... */
    static const std::string default_path;
    
    /*! Construct a note detector maybe using some default properties
     *  \param sr Sample rate of input audio
     *  \param inputBuffer Input 
     *  \param inputBufferSize Size of audio
     *  \param freqs Array of frequencies to look for
     *  \param freqsSize Size of frequency array
     *  \param edo Number of divisons per octave.
     *  \param bandwidth Bandwidth for every detector. Default is minimum bandwidth.
     *  \param features Features enum containing method and normalisation. 
     *  Default is runge_kutta | freq_unnormalized | amp_normalized
     *  \param damping Detector damping. Default is 0.0001
     *  \param gain Detector gain. Default is 25.
     *  TODO: document "path"
     */
    NoteDetector(const parameter_t sr,
                 const inputSample_t* inputBuffer,
                 const std::size_t inputBufferSize,
                 const parameter_t* freqs,
                 const std::size_t freqsSize,
                 const std::size_t edo,
                 const parameter_t bandwidth = default_bandwidth,
                 const int features = default_features,
                 const parameter_t damping = default_damping,
                 const parameter_t gain = default_gain,
                 const std::string path=default_path);

    /*! Construct a note detector maybe using named properties
     * 
     *  bandwidth, features, damping and gain my be specified
     *  in the variable argument list in any order by prefixing
     *  the value with the appropriate enum arg value. The final
     *  value in the argument lest must be arg::nd_last.
     * 
     *  If a parameter is not specified, its default value is used
     * 
     *  \param sr Sample rate of input audio
     *  \param inputBuffer Audio 
     *  \param inputBufferSize Size of audio
     *  \param freqs Array of frequencies to look for
     *  \param freqsSize Size of frequency array
     *  \param edo Number of divisons per octave
     *  \param args "Keyword argument" structure
     */
    NoteDetector(const parameter_t sr,
                 const inputSample_t* inputBuffer,
                 const std::size_t inputBufferSize,
                 const parameter_t* freqs,
                 const std::size_t freqsSize,
                 const std::size_t edo,
                 const NDOptArgs& args);

    ~NoteDetector();
    
    /*! Get onsets (and, later, pitch) for all given frequencies
     *  \param onsets Empty Onsets_t map to be filled
     *  \param threshold OnsetDetector threshold
     *  \param w0 OnsetDetector::analyse weight for 'count' criterion
     *  \param w1 OnsetDetector::analyse weight for 'threshold' criterion
     *  \param w2 OnsetDetector::analyse weight for 'difference' criterion
     *  \return Map of frequency index-onsets vector pairs 
     */
    void analyse(Onsets_t& onsets, double threshold);//, double w0, 
//                  double w1, double w2);
    
    const parameter_t get_bandwidth() const { return bandwidth; }
    const int         get_features()  const { return features;  }
    const parameter_t get_damping()   const { return damping;   }
    const parameter_t get_gain()      const { return gain;      }

    
protected:
    const parameter_t sr;              /*!< Input sample rate */
    const parameter_t* freqs;          /*!< Frequencies to look for */
    const std::size_t freqsSize;       /*!< Number of frequencies to look for */
    const std::size_t edo;             /*!< Number of divisons per octave */
    // The following might be changed by a constructor post initialisation,
    // so they can't be const.
    parameter_t bandwidth;             /*!< Bandwidth for every detector */
    int features;                      /*!< Method and normalisation */
    parameter_t damping;               /*!< Detector damping */
    parameter_t gain;                  /*!< Detector gain */
    
    inputSample_t* inputBufferPad;     /*!< Input buffer, zero padded */
    std::size_t inputBufferPadSize;    /*!< Size of padded input buffer */
    
    /*! Vector of EventDetectors to be run by the NoteDetector */
    std::vector<std::unique_ptr<EventDetector>> eventdetectors; 
    
    /*! Results from all EventDetectors 
     *  Map relates the centre frequency index to the vector of onsets */
    //Onsets_t* onsets; 
    
    std::unique_ptr<ThreadPool> threadPool; /*!< Thread manager */
    
    /*! Struct to pass args to EventDetector::analyse() in threads */
    typedef struct {
        std::vector<std::size_t> *temp;  /*!< Pointer to array of vectors, for onsets vector to be put */
        std::size_t firstBand;   /*!< Index of first EventDetector to be run by this thread */
        std::size_t numBands;    /*!< Number of EventDetectors to be run by this thread */
        double threshold;        /*!< OnsetDetector threshold */
        double w0;               /*!< OnsetDetector::analyse weight */
        double w1;               /*!< OnsetDetector::analyse weight */
        double w2;               /*!< OnsetDetector::analyse weight */
    } Analyse_params;

    
    /*! Initialise the NoteDetector once all the construction
     *  arguments are known
     */
    void init(void);
    
    /*! Perform one thread's worth of work.
     *  Called by analyse()
     *  \param args Arguments (pointer to Analyse_params struct)
     */
    void analyseDelegate(void* args);
    
};

/*! Optional Arguments for kwargs-style initialisation
 *  Not within the NoteDetector namespace to keep SWIG happy
 */
struct NDOptArgs
{
    parameter_t nd_bandwidth { NoteDetector::default_bandwidth};
    int         nd_features  { NoteDetector::default_features };
    parameter_t nd_damping   { NoteDetector::default_damping };
    parameter_t nd_gain      { NoteDetector::default_gain };
    std::string nd_path         { NoteDetector::default_path };

    NDOptArgs& bandwidth(parameter_t b)  { nd_bandwidth = b; return *this; }
    NDOptArgs& features(int f)           { nd_features  = f; return *this; }
    NDOptArgs& damping(parameter_t d)    { nd_damping   = d; return *this; }
    NDOptArgs& gain(parameter_t g)       { nd_gain      = g; return *this; }
    NDOptArgs& path(std::string p)          { nd_path = p; return *this; }
};

#endif
