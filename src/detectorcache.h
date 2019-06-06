#ifndef _DETECTORCACHE_H_
#define _DETECTORCACHE_H_

#include "slidingbuffer.h"
#include "detectorbank.h"

namespace detectorcache {
    
    /*!
     * Specialisation of the slidingbuffer::SegmentProducer class to create
     * blocks of DetectorBank analysis channels and store them
     * in a detectorcache::Segment.
     * 
     * The 2D nature of the storage means that the specialisation
     * of SlidingBuffer is somewhat non-trivial. Refer to the detailed
     * description of detectorcache::DetectorCache for a full explaination
     * of the data structures involved.
     */
    class Producer : public slidingbuffer::SegmentProducer<result_t *>
    {
    public:
        /*!
         * Construct a Producer given a DetectorBank.
         * 
         * Since the Producer's final operating parameters (specifically,
         * the number of time-samples per segment) is unknown until
         * the associated DetectorCache is constructed, and because
         * this must happen after the Producer's construction,
         * no memory allocation can be usefully performed here.
         * 
         * The Producer can be operated in pre-buffered or stream
         * mode. In the former, the DetectorBank will have been
         * initialised with an array of audio samples, and each call to
         * Producer::generate will progress through the DetectorBuffer's
         * input buffer as normal. If an istream is supplied when the
         * producer is constructed, every time Producer::generate
         * is called, Producer::seg_len samples are read from
         * the associated audio istream, used to set the DetectorBank's
         * input buffer from which the analysis is generated
         * 
         * \param db The previously created detetorbank which will
         *           perform the analysis
         * \param is If stream mode is required, a pointer to the
         *           input stream from which samples (of inputSample_t)
         *           shall be read
         */
        Producer(DetectorBank &db,
                 std::istream* is = nullptr)
            : db{db}
            , audiostream{is} {};
        /*!
         * Generate a DetectorBank result block of the length specified by
         * our associated DetectorCache after its construction
         * 
         * \param seg Pointers to results for each channel to be populated
         *            by the Producer
         * \param idx Sequence key for the first channel in this block.
         *            Refer to the extended documentation for
         *            detectorcache::DetectorCache for more details
         * \param size Number of channels for which data is held (not used:
         *             this is read from the DetectorBank for security reasons).
         * \return The number of channels for which data is stored.
         */
        virtual std::size_t generate(result_t* seg[],
                                     const std::size_t idx,
                                     const std::size_t size);
        /*!
         * Establish wheter there is further data to be read from the
         * DetectorBank associated with this cache.
         * 
         * \returns `true` if more data is available, otherwise `false`.
         */
        virtual bool more(void) { return db.tell() < db.getBuflen(); }; //const?
        /*! 
         * Set the block length (in samples) of each audio block which the
         * Producer must generate. This is unknown until the Producer is
         * constructed (which has to be before the DetectorCache object
         * which determines this parameter).
         * 
         *\param n Number of audio samples to be generated for each channel
         */
        void set_num_samps(std::size_t n) { seg_len = n; }; // private? Friend function of Detectorcache?
        /*! Set the startChan for getZ 
         *  \param c Channel from which to calculate abs(z) values
         */
        void set_start_chan(std::size_t c) { start_chan = c; };
        
        // ACCESS FUNCTIONS - return values from the DetectorBank
        
        /*! Get the sample rate associated with this DetectorBank
        * \return The current sample rate
        */
        parameter_t getSR(void) const { return db.getSR(); };
        /*!
         * Enquire the number of channles for which data is produced
         * \return The number of channels for which this Producer shall
         *         provide valid data.
         */
        std::size_t getChans(void) { return db.getChans(); }; //const?
        /*!
         * Enquire the length of the data stored in the Producer's
         * associated DetectorBank.
         * \returns Length of the associated DetectorBank's input
         *          buffer in samples.
         */
        std::size_t getBuflen(void) { return db.getBuflen(); }; //const?
        /*! Find the frequency of a given channel's 
        *  \link AbstractDetector detector\endlink. If the signal has been modulated 
        *  and/or normalised, the adjusted frequency will be returned.
        *  Returns 0 if the channel number is invalid.
        * \param ch Channel number
        * \return w = 2pi.f for the specified channel
        */
        parameter_t getW(std::size_t ch) {return db.getW(ch);};
        /*! Get the input frequency of a given channel's 
        *  \link AbstractDetector detector\endlink.
        *  Returns 0 if the channel number is invalid.
        * \param ch Channel number
        * \return f_in for the specified channel
        */
        parameter_t getFreqIn(std::size_t ch) {return db.getFreqIn(ch);};        
        
    protected:
        DetectorBank& db;             /*!< The DetectorBank which produces the results */
        std::size_t seg_len;           /*!< Block-length (number of audio samples) to request
                                       on each call to `generate()` */
        std::istream* audiostream;     /*!< In stream mode (datastream non-null), the
                                       istream from which inputSample_t samples
                                       are to be read */
        std::size_t start_chan;        /*!< Channel in DetectorBank at which to start */
    };
    
    /*!
     * The producer class enabling a slidingbuffer::SlidingBuffer to
     * be deployed as a cache for the two-dimensional data emerging
     * from a DetectorBank.
     */
    class Segment : public slidingbuffer::Segment<result_t *, detectorcache::Producer>
    {
    public:
        /*!
         * Construct a segment for use by a slidingbuffer::SlidingBuffer.
         * Note that the SlidingBuffer entity is in fact a `result_t*`
         * containing the address of the first result of the channel
         * at the analysis-block sample-time origin.
         * The analysis block is stored in a contiguous 2D array
         * of result values. Conseqently the size parameter in particular
         * refers to the number of channels rather than the number of samples,
         * or number of samples per channel. Refer to the extended documentation
         * for detectorcache::DetectorCache for the bigger picture.
         * 
         * \param size Number of entities in this segment (i.e. the number of channels).
         * \param origin The ordinal key of the first channel pointer stored.
	 * \param sp The producer for this segment.
         */
        Segment(const std::size_t size, const std::size_t origin,
                Producer& sp);
        /*!
         * Destroy the Segment and the underlying audio samples in all of the
         * channels which it stores
         */
        virtual ~Segment();
    
        friend class Producer;
    };
    
    /*! Cache results from a detectorbank */
    class DetectorCache
        : public slidingbuffer::SlidingBuffer<result_t *,
                                              detectorcache::Segment,
                                              detectorcache::Producer>
    { 
    public:
        /*! Construct a DetectorCache
         * 
         * \param p The segment producer for this cache.
         * \param num_segs Number of historical segments to remember.
         * \param seg_len Number of audio samples per segment.
         * \param start_chan First channel for this cache.
         * \param num_chans Number of channels in this cache.
         */
        DetectorCache(Producer& p,
                      const std::size_t num_segs,
                      const std::size_t seg_len,
                      const std::size_t start_chan,
                      const std::size_t num_chans);
        
        /*! Construct a DetectorCache with default start and size
         * 
         * If the start channel parameter is omitted, assume 0.
         * If the number of channels is omitted, use the value
         * obtained by querying the supplied producer.
         */
        DetectorCache(detectorcache::Producer& p,
                      const std::size_t num_segs,
                      const std::size_t seg_len,
                      const std::size_t start_chan = 0)
            : DetectorCache(p,
                            num_segs,
                            seg_len,
                            start_chan,
                            p.getChans()) { }

        /*!
         * Return the absolute value of the DetectorBank output
         * at the given channel and sample time. The sample
         * time must be positive, and not be too old to have been
         * deleted by the detectorcache.
         * 
         * If the sample time is negative, a slidingbuffer::NegativeIndexException
         * will be thrown. This must be done explicitly here, as the DetectorCache
         * is a 2D representation of a SlidingBuffer, where the channel is the 
         * SlidingBuffer index, so only a negative channel will result in a 
         * NegativeIndexException thrown by the SlidingBuffer itself.
         * 
         * If the sample time exceeds the length of the audio buffer, getResultItem
         * will return 0.
         * 
         * \param ch Channel number of the output sample.
         * \param n Sample's time-index.
         * \returns The value of the detector's output at that time.
         * \throws slidingbuffer::NegativeIndexException If n < 0 
         */
        result_t getResultItem(long int ch, long int n);
        /*! Efficiently copy a range of cached data from a given cache channel
         *  to the designated target.
         * 
         *  The caller must allocate sufficient memory for the requested
         *  number of result_t data, and pass its address and the number
         *  of results desired. The supplied memory is filled with results
         *  from the detectorcache, ending at the given index.
         * 
         *  This permits an external onset detecting algorithm
         *  to request the history of a detector channel once
         *  an onset threshold has been achieved
         *  in order to refine its estimate of the precise onset time.
         * 
         *  \param chan Channel for which to provide history
         *  \param currentSample Index from which to roll back
         *  \param samples Pointer to target storage
         *  \param numSamples Number of samples to copy to target (currentSample
         *                    being the last)
	 *  \return The number of samples actually copied.
         *  \throws slidingbuffer::ExpiredIndexException If the requested
         *          number of samples exceeds that in the cache.
         * 
         *  \sa DetectorCacheDesign
         */
        std::size_t getPreviousResults(const std::size_t chan,
                                       const std::size_t currentSample,
                                       result_t* samples, std::size_t numSamples);
        /*! Get the total number of samples the cache can return.
         *  \returns Length of the associated DetectorBank's audio input
         *           buffer in samples.
         */
        std::size_t end(void) { return p.getBuflen(); };
        /*! Get the sample rate of the associtaed DetectorBank
         */
        parameter_t getSR(void) const { return p.getSR(); };
        /*! Get the number of channels in the cache
         */
        std::size_t getChans(void) { return chans; };
        
    protected:
        const std::size_t chans;       /*!< Number of analysis channels */
        const std::size_t seg_len;     /*!< Number of audio samples per segment */
        Producer& p;                   /*!< This Producer for this DetectorCache */
        const std::size_t start_chan;  /*!< Channel in DetectorBank at which to start */
    };

};

#endif
