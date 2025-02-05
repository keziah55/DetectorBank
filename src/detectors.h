#ifndef _ABSTRACTDETECTOR_H_
#define _ABSTRACTDETECTOR_H_

#include <complex>
#include <utility>
#include <memory>
#include <string>

#include <cereal/cereal.hpp>
#include <cereal/access.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/types/complex.hpp>

#include "detectorbank.h"
#include "detectortypes.h"

/*!
 * Base class for detectors using different numerical methods.
 * The process() method must be supplied by the derived classes.
 */
class AbstractDetector {
    
public:
    /*!
     * \param f Detector centre frequency (Hz)
     * \param mu Detector control parameter (should be in the region of 0)
     * \param d Detector damping ratio
     * \param sr Sample rate of input audio (should be 44.1kHz or 48kHz)
     * \param detBw Detector bandwidth (Hz)
     * \param gain Detector gain
     */
    AbstractDetector(parameter_t f, parameter_t mu, 
                     parameter_t d, parameter_t sr, 
                     parameter_t detBw, parameter_t gain);
                    
    virtual ~AbstractDetector();
    //! Process audio using the numerical method appropriate to the derived class.
    /*!
     * The derived class's process() method is called and if
     * amplitude normalisation is required, the predetermined 
     * gain and stiffness constant is applied to the result.
     * 
     * \param target Pointer to output array. This should have the correct 
     * dimensions for your desired detector bank: height = number of 
     * detectors, length = length of audio input.
     * \param start Pointer to beginning of audio
     * \param count Number of frames to process
     */
    void processAudio(discriminator_t* target,
                      const inputSample_t* start, std::size_t count);

    /*!
     * Normalise the detector frequency using an iterative scheme. 
     * The lower and upper bounds of the search must be respectively 
     * lower and higher than the actual characteristic frequency of 
     * the detector. The search iterates by moving the worst performing 
     * bounding frequency half way towards the mean of the two current 
     * bounds until the maximum number of iterations or is exceeded or 
     * the specified accuracy is achieved (see maxNormIterations and 
     * normConverged).
     * 
     * \param searchStart Lower bound of search (ratio of specified f0)
     * \param searchEnd Upper bound of search (ratio of specified f0)
     * \param toneDuration Length constant test tone to be generaated
     * \throw std::string Invalid detector type while attempting
     *                    search-normalisation
     * \throw std::string Searching for normalised charactersitc frequency:
     *                    test range does not span maximum response.
     */
    bool searchNormalize(parameter_t searchStart,
                         parameter_t searchEnd,
                         const parameter_t toneDuration,
                         const parameter_t forcing_amplitude);
    
    /*! Set the detector's a and iScale attributes by profiling an ideal
     * detector response. These can then be used to normalise the detector's 
     * amplitude response to the range 0-1.
     * 
     *  \param forcingAmplitude Gain that was applied to the input signal
     *  \returns true
     *  \throw std::string Invalid detector type while attempting amplitude 
     *                     normalization
     */
    bool amplitudeNormalize(const parameter_t forcing_amplitude);
    
    /*! Calculate amplitude scale factor */
    void scaleAmplitude();
    
    /*! Generate a sine tone
     *  \param tone Array for output tone
     *  \param duration Tone duration (in samples)
     *  \param frequency Frequency at which to generate tone (Hz)
     */
    void generateTone(inputSample_t* tone, 
                      const std::size_t duration,
                      const parameter_t frequency);
    
    // ACCESS FUNCTIONS
    
    /*! Find the specficied characteristic frequency for this detector
     *  (post-modulation and -normalisation) 
     * \returns Characterisic frequency in rad/s
     */
    parameter_t getW(void) const { return w; };
    
    /*! Reset the internal values used to calculate z to 0.
     *  This is called when DetectorBank.seek() is called.
     *  Overridden in derived classes.
     */
    virtual void reset() = 0;
    
protected:
    /*!
     * This method gets overridden in derived classes
     * to produce an unnormalised version of the required output.
     */
    virtual void process(discriminator_t* target,
                         const inputSample_t* start, std::size_t count) = 0;
                         
     
    /*! Find the first Lyapunov coefficient required for a given
     *  bandwidth, with a given forcing amplitude.
     *  If the desired bandwidth is given as zero, the returned value
     *  will be zero. This will create a detetor with the narrowest 
     *  possible bandwidth (~1.5Hz)
     * \param bw Desired bandwidth
     * \param amp Forcing amplitude 
     * \return First Lyapunov coefficient
     */
    static const parameter_t getLyapunov(const parameter_t bw, const parameter_t amp);

    // DetectorBank is responsible for detector serialisation
    /*!
     * Permit cereal and the archive functions of the DetectorBank
     * with which this detector is composed to read
     * protected variables for archival purposes.
     */
    template <class Archive> friend void DetectorBank::save(Archive&) const;
    /*!
     * Permit cereal and the archive functions of the DetectorBank
     * with which this detector is composed to write
     * protected variables for archival purposes
     */
    template <class Archive> friend void DetectorBank::load(Archive&);
    friend class cereal::access;
    /*!
     * Save the DetectorBank as a cereal archive
     * \param archive The archive to which properties are written.
     */
    template <class Archive> void save(Archive& archive) const
    {
        archive.setNextName("AbstractDetector");
        archive.startNode();
        archive(cereal::make_nvp("w_adjusted", w),
                CEREAL_NVP(aScale),
                CEREAL_NVP(iScale)
                );
        archive.finishNode();
    }
    /*!
     * Load the DetectorBank from a cereal archive
     * \param archive The archive from which properties are read.
     */
    template <class Archive> void load(Archive& archive)
    {
        //std::cout << "Starting to load an AbstractDetector\n";
        archive.startNode();
        archive(cereal::make_nvp("w_adjusted", w),
                CEREAL_NVP(aScale),
                CEREAL_NVP(iScale)
        );

        archive.finishNode();
    }
    
    parameter_t w;               /*!< Characteristic frequency */
    parameter_t const mu;        /*!< Distance from the bifurcation point */
    parameter_t const d;         /*!< Detector damping factor */
    parameter_t const sr;        /*!< Sample rate */
    parameter_t const detBw;     /*!< Detector bandwidth */
    parameter_t const gain;      /*!< Detector gain */
    discriminator_t aScale;      /*!< Amplitude scaling factor */ 
    parameter_t iScale;          /*!< Scaling factor for imaginary part */
    parameter_t b;               /*!< Detector's first Lyapunov coefficent */
    discriminator_t scale;       /*!< Detector's scale factor */
    bool nrml { false };         /*!< If search normalisation has been applied */
     
    /*! The maximum number of iterations to find best response
     *  during search_normalization */
    static constexpr int maxNormIterations { 100 };
    /*! Ratio of discovered frequency to specification frequency
     *  to be considered "close enough" during seach_normalization.
     *  1.0057929410678534 = 2^(1/120) = 10 cents;
     *  1.0028922878693671 = 2^(1/240) =  5 cents
     */
    static constexpr double normConverged { 1.0028922878693671 };
    
    /*! Make freq and factor vectors for given method and normalisation */
    void makeScaleVectors();
    
    /*! Get a scale value for a given frequency*/
    void getScaleValue(const parameter_t fr);
    
    std::vector<parameter_t> detScaleFreqs;
    std::vector<discriminator_t> detScaleFactors;
    static const std::array<std::vector<parameter_t>, 8> scaleFreqs;
    static const std::array<std::vector<discriminator_t>, 8> scaleFactors;
};

/*! 
 * Use the central-difference approximation to calculate the output.
 * 
 * Uses the discrete central difference approximation,
 * \f[ \dot y[n] = \frac 1{2\delta}\bigg(y[n+1] - y[n-1]\bigg)\f]
 * to evalute the next output sample of the Hopf bifurcation
 * \f[\dot z=(\mu+j\omega_0)z + b|z|^2z + X\f]
 * with control parameter \f$\mu\f$, first Lyapunov coeffcient \f$b\f$ (real)
 * and driving force \f$X\f$. For a supercritical bifurcation, \f$b < 0\f$.
 * \f$b = 0\f$ yields a degenerate ("Bautin") bifurcation, and
 * \f$b > 0\f$ yields a subcritical bifurcation which is unstable.
 */
class CDDetector : public AbstractDetector {
public:
    /*!
     * \param f Detector centre frequency (Hz)
     * \param mu Detector control parameter. (Setting mu = 0 positions the 
     * system at the bifurcation point)
     * \param d Detector damping ratio
     * \param sr Sample rate of input audio
     * \param detBw Detector bandwidth
     * \param gain Detector gain
     */
    CDDetector(const parameter_t f, const parameter_t mu, 
               const parameter_t d, const parameter_t sr, 
               const parameter_t detBw, const parameter_t gain);
    //! CDDetector destructor
    virtual ~CDDetector();
    /*! Reset internal values to 0.
     *  This is invoked when Detect.seek() is called.
     */
    virtual void reset();
    /*! Method to process audio using central-difference approximation.
     *  Called by runChannels() in DetectorBank.
     * \param target Output array. This should have the correct 
     * dimensions for your desired detector bank: height = number of 
     * detectors, length = length of audio input.
     * \param start Current audio sample
     * \param count Number of frames to process
     */

    virtual void process(discriminator_t* target,
                         const inputSample_t* start,
                         const std::size_t count) override;
                         
private:
    std::complex<parameter_t> zp;        //!< previous z value
    std::complex<parameter_t> zpp;       //!< z value two samples ago
    inputSample_t xp;                    //!< previous audio input sample
};

/*!
 * Use the fourth order Runge-Kutta method to calculate the output.
 * 
 * Uses the discrete Runge-Kutta method to solve the first order
 * differential equation \f$y'=f(y, t)\f$.
 * Let:
 * 
 * \f{eqnarray*}
 * k_0 & = & f(y_n, n-2) \\
 * k_1 & = & f(y_n + k_0\delta, n-1) \\
 * k_2 & = & f(y_n + k_1\delta, n) \\
 * k_3 & = & f(y_n + k_22\delta, n)
 * \f}
 * 
 * where \f$\delta\f$ is the time step between samples,
 * 
 * then:
 * 
 * \f[
 * y_{n+1} = y_n + \frac\delta 3(k_0 + 2k_1 + 2k_2 + k_3).
 * \f]
 * 
 * This procedure is used iteratively to find
 * output samples of the Hopf bifurcation
 * \f[\dot z=(\mu+j\omega_0)z + b|z|^2z + X\f]
 * with control parameter \f$\mu\f$, first Lyapunov coefficient \f$b\f$ (real)
 * and driving force \f$X\f$. For a supercritical bifurcation, \f$b < 0\f$.
 * \f$b = 0\f$ yields a degenerate ("Bautin") bifurcation, and
 * \f$b > 0\f$ yields a subcritical bifurcation which is unstable.
 */
class RK4Detector : public AbstractDetector {
public:
    /*!
     * \param f Detector centre frequency (Hz)
     * \param mu Detector control parameter. (Setting mu = 0 positions the 
     * system at the bifurcation point)
     * \param d Detector damping ratio
     * \param sr Sample rate of input audio
     * \param detBw Detector bandwidth
     * \param gain Detector gain
     */
    RK4Detector(const parameter_t f, const parameter_t mu, 
                const parameter_t d, const parameter_t sr, 
                const parameter_t detBw, const parameter_t gain);
    virtual ~RK4Detector();
    /*! Reset internal values to 0 
     *  This is invoked when Detect.seek() is called.
     */
    virtual void reset();
    /*! Method to process audio using fourth order Runge-Kutta approximation.
     *  Called by \link DetectorBank::runChannels() runChannels()\endlink in DetectorBank.
     * \param target Output array. This should have the correct 
     * dimensions for your desired detector bank: height = number of 
     * detectors, length = length of audio input.
     * \param start Current audio sample
     * \param count Number of frames to process
     */

    virtual void process(discriminator_t* target,
                         const inputSample_t* start,
                         const std::size_t count) override;
private:
    std::complex<parameter_t> zp;        //!< previous z value
    std::complex<parameter_t> zpp;       //!< z value two samples ago
    inputSample_t xp;                    //!< previous audio input sample
    inputSample_t xpp;                   //!< audio input sample two samples ago
};

#endif
