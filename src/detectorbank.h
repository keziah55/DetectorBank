#ifndef _DETECTORBANK_H_
#define _DETECTORBANK_H_

//! DetectorBank
/*! Bank of note onset detectors, each operating at a given frequency.
 */
#include <complex>
#include <utility>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <list>
#include <condition_variable>
#include <cereal/access.hpp>

#include "detectortypes.h"
#include "thread_pool.h"

class AbstractDetector;
class ProfileManager;

//! Use multiple detectors to find note onsets at given frequencies (with multithreading).
class DetectorBank {
    
public:
    /*! Bit mask for specifying the solver method */
    static constexpr int solverMask { 0xff };
    /*! Bit mask for specifying the frequency normalisation method */
    static constexpr int freqNormalizationMask { 0xff << 8 };
    /*! Bit mask for specifying the amplitude normalisation method */
    static constexpr int ampNormalizationMask { 0xff << 16 };
    /*! Specify numerical and normalisation methods for this detector bank. 
     *  Please see \link FeaturesExplained DetectorBank Features\endlink for more information.
     */
    enum Features {
        
        // Choose numerical method
        central_difference = 1,        /*!< Central-difference */
        runge_kutta        = 2,        /*!< Fourth order Runge-Kutta */
        
        // Frequency normalisation
        freq_unnormalized  = 1 << 8,   /*!< Without frequency normalisation */
        search_normalized  = 2 << 8,   /*!< Iteratively adjust response */
        
        // Amplitude normalisation
        amp_unnormalized   = 1 << 16,  /*!< Without amplitude normalisation */
        amp_normalized     = 2 << 16,  /*!< Scale real and imaginary parts of the response */
        
        // Vanilla
        //! Default is Runge-Kutta, unnormalised frequency, normalised amplitude
        defaults            = runge_kutta | freq_unnormalized | amp_normalized
        
    };
    
    /*! Mask which selects possible numerical methods */
    static constexpr int method_mask {
        Features::central_difference |
        Features::runge_kutta
    };

    /*! Mask which selects possible frequency normalizations */    
    static constexpr int frequency_normalization_mask {
        Features::freq_unnormalized |
        Features::search_normalized
    };

    /*! Mask which selects possible amplitude normalizations */    
    static constexpr int amplitude_normalisation_mask {
        Features::amp_unnormalized |
        Features::amp_normalized
    };
    
    /*!
     * Construct a DetectorBank from archived parameters
     * \param profile The name of the profile to read from the archive
     * \param inputBuffer Audio input
     * \param inputBufferSize Length of audio input
     * \throw std::string Profile 'profile' not found.
     */
    DetectorBank(const std::string& profile,
                 const inputSample_t* inputBuffer,
                 const std::size_t inputBufferSize);

    /*!
     * Construct a DetectorBank.
     * \param sr Sample rate of audio. (This must be 44100 or 48000.)
     * \param inputBuffer Audio input
     * \param inputBufferSize Length of audio input
     * \param numThreads Number of threads to execute concurrently
     * to determine the detector outputs. Passing a value of less than
     * 1 causes the number of threads to be set according to the number
     * of reported CPU cores
     * \param freqs Array of frequencies for the detector bank
     * \param bw Array of bandwidths for each detector. If nullptr, minimum 
     * bandwidth detetors will be constructed
     * \param numDetectors Length of the freqs and bandwidths arrays
     * \param features Numerical method (runge_kutta or central_difference),
     * freqency normalisation (freq_unnormalized or search_normalized) and
     * amplitude normalisation (amp_unnormalized or amp_normalized).
     * Default is runge_kutta|freq_unnormalized|amp_normalized.
     * See \link FeaturesExplained DetectorBank Features\endlink for more information.
     * \param damping Damping for all detectors
     * \param gain Audio input gain to be applied. (Default value of 25
     * is recommended in order to keep the numbers used in the internal
     * calculations within a sensible range.)
     * \throw std::string Sample rate should be 44100 or 48000
     * \throw std::string Central difference can only be used for minimum bandwidth detectors.
     */
    DetectorBank(const parameter_t sr,
                 const inputSample_t* inputBuffer,
                 const std::size_t inputBufferSize,
                 std::size_t numThreads,
                 const parameter_t* freqs = EDO12_pf, 
                 parameter_t* bw = nullptr,
                 const std::size_t numDetectors = EDO12_pf_size,
                 Features features = Features::defaults,
                 parameter_t damping = 0.0001,
                 const parameter_t gain = 25.0);

    virtual ~DetectorBank();
    
    // Maybe want to reuse the object on a different input buffer
    /*!
     * Change the input, without recreating the detector bank.
     * \param inputBuffer New input samples
     * \param inputBufferSize Length of new input
     */
    void setInputBuffer(const inputSample_t* inputBuffer,
                        const std::size_t inputBufferSize);
    // Get some frames of z-values from the discriminators
    // Repeated calls progressively traverse the audio input buffer.
    // Returns the number of frames actually processed
    // (0 at end of input)
    /*! Get the next numFrames of detector bank output.
     * \param frames Output array
     * \param chans Height of output array
     * \param numFrames Length of output array
     * \param startChan Channel from which to start
     * \return Number of frames processed
     */
    int getZ(discriminator_t* frames,
             std::size_t chans, std::size_t numFrames,
             const std::size_t startChan = 0);
    /*! Take z-frames and fill a given array of the same dimensions (absFrames) with 
     *  their absolute values.
     *  Also returns the maximum value in absFrames.
     *  \param absFrames Output array
     *  \param absChans Height of the output array
     *  \param absNumFrames Length of the input array
     *  \param frames Input array of complex z values
     *  \param maxThreads The number of threads used to perform the calculations.
     *         0 (the default) causes the value used when constructing the
     *         DetectorBank to be used. If only a small number of samples
     *         are to be converted, it might help to force single-thread
     *         operation because the abs(·) function is so light-weight.
     *  \return The maximum value found while performing the conversion
     */
    result_t absZ(result_t* absFrames,
                  std::size_t absChans,
                  const std::size_t absNumFrames,
                  discriminator_t* frames,
                  std::size_t maxThreads = 0
                 ) const;
    /*! Set input sample at which to start the detection.
     *  Negative values seek from the end of the current input buffer
     * \param offset New sample index
     * \return `true` for success; `false` if the requested offset is out of range
     */
    bool seek(long int offset);
    
    // ACCESS FUNCTIONS
                  
    /*! Get the index of current input sample
     * \return Current input sample index
     */
    std::size_t tell(void) const { return currentSample; };
    /*! Get the sample rate associated with this DetectorBank
     * \return The current sample rate
     */
    parameter_t getSR(void) const { return sr; };
    /*! Return the number of detectors currently maintained by this DetectorBank
     * \return The number of detectors
     */
    std::size_t getChans(void) const { return detectors.size(); };
    /*! Find out the total number of samples currently available
     * \return The total number of samples in the audio buffer
     */
    std::size_t getBuflen(void) const { return inBufSize; };
    /*! Find the frequency of a given channel's 
     *  \link AbstractDetector detector\endlink. If the signal has been modulated 
     *  and/or normalised, the adjusted frequency will be returned.
     *  Returns 0 if the channel number is invalid.
     * \param ch Channel number
     * \return w = 2pi.f for the specified channel
     */
    parameter_t getW(std::size_t ch) const;
    
    /*! Find the input frequency of a given channel's 
     *  \link AbstractDetector detector\endlink.
     *  Returns 0 if the channel number is invalid.
     * \param ch Channel number
     * \return f_in for the specified channel
     */
    parameter_t getFreqIn(std::size_t ch) const;
        
    /*! Return description of the detectorbank serialised in XML form */
    std::string toXML(void) const;
    /*! Set the state of the detectorbank according to a previously
     *  serialised XML description.
     * \param xml The description of the detector bank's new state
     */
    void fromXML(std::string xml);
    /*! Save the current profile so that a DetectorBank can be constructed
     *  conveniently in future
     * \param name The name of the profile
     */
    void saveProfile(std::string name);
    /*!
     * Return list of existing profiles
     * \return List of strings
     */
    std::list<std::string> profiles();
    
    friend class cereal::access;
    /*! Write the relevant properties of the detector bank to an archive
     * \param archive The archive to which properties should be written */
    template<class Archive> void save(Archive& archive) const;
    /*! Write the relevant properties of the detector bank to an archive
     * \param archive The archive from which properties should be read */
    template<class Archive> void load(Archive& archive);

protected:    
    /*! Apply a gain to the inputBuffer
     * \param signal Signal to be amplified
     * \param signalSize Size of signal
     * \param gain Gain to be applied
     */
    void amplify(const inputSample_t*& signal,
                 std::size_t signalSize,
                 const parameter_t gain);
    
    void worker(int id);
    
    /*!
     * Struct to pass getZ thread parameters to a worker thread.
     */
    typedef struct {
        std::size_t firstChannel;     /*!< First channel to process */
	std::size_t numChannels;      /*!< Number of channels to process */
        discriminator_t* frames;      /*!< Output array */
        std::size_t framesPerChannel; /*!< Number of frames per channel */
        std::size_t numFrames;        /*!< Number of frames left to process */
    } GetZ_params;
    
    /*!
     * Perform one thread's worth of work on the given channels.
     * Called by getZ().
     * Parameters are required as a pointer to a GetZ_params.
     * \param args Arguments
     */
    void getZDelegate(void* args);

    /*!
     * Struct to pass absZ thread parameters to a worker thread.
     */
    typedef struct {
        std::size_t start, end;
        result_t& mx;
    } AbsZ_params;

    /*! Printable string representations of the flags in the Features enum
     *  Use the provided routines through preference to produce a human-readable
     *  readable format, as they will deal with combinations of flags
     */
    static const std::map<int, std::string> featuresToStringMap;
    /*! Set this DetectorBank's features from a human-readable
     *  string using featuresToStringMap
     * \param desc Human-readable feature list of comma-separated features
     * \throws "Illegal feature name reading XML profile" if a feature in the
     *         list is unrecognised;
     * \throws "No valid features in feature list reading XML profile" if the
     *        feature list is empty
     */
    void stringToFeatures(const std::string& desc);
    /*! Produce a human-readable string describing
     *  this DetectorBank's features using featuresToStringMap.
     * \returns Human-readable comma-separated string descibing the features
     */
    const std::string featuresToString(void) const;

    /*! Detectors to be run by this detector bank */
    std::vector<std::unique_ptr<AbstractDetector>> detectors;
    std::size_t inBufSize;        /*!< Size of the current audio input buffer */
    const inputSample_t* inBuf;   /*!< The current input buffer */
    /*! Thread manager for concurrent sections */
    std::unique_ptr<ThreadPool> threadPool;
    std::size_t currentSample;    /*!< How far along the input for next read */
    parameter_t d;                /*!< Detector damping factor */
    parameter_t sr;               /*!< Operating sample rate */
    Features features;            /*!< Detector method & normalistion */
    parameter_t* bw;              /*!< Array of bandwidths */
    parameter_t gain;             /*!< Audio input gain to be applided */
    
    /*! If a gain is applied to the input signal, this pointer refers
     *  to the locally allocated buffer containing the amplified signal
     *  and inBuf is set to the same address
     */
    std::unique_ptr<inputSample_t[]> gainBuf;
    
    /*! Standard tuning set for a 12EDO piano keyboard */
    static const parameter_t EDO12_pf[];
    /*! Size of standard 12EDO piano keyboard */
    static const int EDO12_pf_size;
    
    /*! The profile manager responsible for loading and saving
     * detectorbank properties */
    static ProfileManager profileManager;
    
    /*!
     * Metaparameters for a detector
     * 
     * When a detector of frequency f_in is requested, it may
     * produce better results first to frequency-shift the input
     * signal then to call upon a detector which operates at a
     * lower frequency on a shifted version of the input.
     */
    struct detector_components {
        const parameter_t f_in;      /*!< The caller's requested frequency */
        const parameter_t f_actual;  /*!< The frequency of the detector used */
        const inputSample_t* signal; /*!< Pointer to a frequency-shifted version
                                          of the input data */
        const parameter_t bandwidth; /*!< Detector bandwdith */
    };
    
    /*! Vector of detector_components describing each AbstractDetector in
     *  the vector detectors
     */
    std::vector<detector_components> dbComponents;
    
    /*!
     * Create the detector_components required for each detector in the
     * detector bank.
     * 
     * If the frequency requested for a detector exceeds an
     * empirically defined constant (based on the normalization
     * method and the type of solver in use) the results are
     * obtained from a lower frequency detector operating
     * on a frequency-shifted input buffer. This method initialises
     * the dbComponents vector and generates the frequency-shifted
     * input signals if required.
     */
    void setDBComponents(const parameter_t* frequencies, 
                           const parameter_t* bandwidths,
                           const std::size_t numDetectors);
        
    parameter_t modF;  /*!< Frequency above which the signal should be modulated */
    
private:
    /*!
     * Utility routine to make the appropriate detectors and tune them
     * according to the required feature set. The detectors are pushed
     * appended to the protected vector of detectors. Would normally only
     * be called when then class is being constructed or deserialised.
     * If the f is not provided (nullptr), the initial freqency is set
     * to 0Hz in the expectation that it will be corrected later on during
     * a deserialisation process. In this case, normalisation will also
     * be skipped.
     * \param numDetectors Size of the array freqs
     * \param mu Criticality
     * \param d Damping
     * \param sr Sample rate
     * \param features Type of filter and normalisation method
     * \param b First Lyapunov coeffcient
     */
    void makeDetectors(const std::size_t numDetectors,
                       const parameter_t mu,
                       const parameter_t d,
                       const parameter_t sr,
                       const Features features,
                       //const parameter_t b,
                       const parameter_t gain);
    
    /*!
     * Mapping of ratio of requested frequency to the maximum
     * used detector frequency for this solver and normalization method.
     */
    std::map<int, std::unique_ptr<inputSample_t[]>> input_pool;
    
    /*! Has DetectorBank created its own array of zeros for bandwidth? */
    bool auto_bw;
};

#endif
