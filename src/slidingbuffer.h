#ifndef _SLIDINGBUFFER_H_
#define _SLIDINGBUFFER_H_

#include <cstddef>
#include <stdexcept>
#include <deque>
#include <memory>

#include "uniqueallocator.h"

#include <iostream>

namespace slidingbuffer {

    /*!
     * The base class for all exceptions thrown by a SlidingBuffer.
     * Records the reason for the exception in the parent class
     * and the requested index which caused the exception.
     */
    class SlidingBufferException
        : public std::runtime_error {
    public:
        /*! Construct an Exception object
         * \param what The reason for the exception.
         * \param idx Index of requested access which caused the exception.
         */
        SlidingBufferException(const char* what, const long int idx)
            : std::runtime_error(what)
            , index { idx }
            { };
        const long int index;   /*!< The exception was caused by attempting
                                     to access this index */
    };
    
    /*!
     * Exception thrown when an attempt was made to access a negative index
     * in the SlidingBuffer
     */
    class NegativeIndexException
        : public SlidingBufferException {
    public:
        /*! Construct an Exception object
         * \param idx Index of requested access which caused the exception.
         */
        NegativeIndexException(const long int idx)
            : SlidingBufferException(
                "SlidingBuffer: Attempt to access negative index",
                idx) { };
    };
    
    /*!
     * Exception thrown when an apparently valid index (positive,
     * within the extent of the data read so far) is accessed,
     * but the entity has been destroyed because it is older
     * than the oldest entity held in the SlidingBuffer.
     */
    class ExpiredIndexException
        : public SlidingBufferException {
    public:
        /*! Construct an Exception object
         * \param idx Index of requested access which caused the exception.
         */
        ExpiredIndexException(const long int idx)
            : SlidingBufferException(
                "SlidingBuffer: Indexed item no longer available (underflow)",
                idx) { };
    };
    
    /*!
     * Exception thown when the entity at position 'index' is
     * not yet available and the SegmentProducer for this SlidingBuffer
     * is unable to produce further data to satisfy the reqest.
     */
    class IndexOutOfRangeException
        : public SlidingBufferException {
    public:
        /*! Construct an Exception object
         * \param idx Index of requested access which caused the exception.
         */
        IndexOutOfRangeException(const long int idx)
            : SlidingBufferException(
                "SlidingBuffer: Attempt to access past end of data",
                idx) { };
    };
    
    /*!
     * Template of a pure virtual class which populates a Segment
     * with entities.
     * 
     * \tparam T Type of entity produced by the SegmentProducer
     */
    template <typename T> class SegmentProducer {
    public:
        /*! Destructor: to be provided by derived class */
        virtual ~SegmentProducer() { };
        /*!
         * Populate the array supplied by the Segment
         * with newly constructed entities. The first such
         * entity shall have the supplied index with in the
         * SlidingBuffer.
         * 
         * \param seg  Storage for the constructed entities
         *             supplied by the caller.
         * \param idx  The origin (index of the first entity
         *             from the point of view of the SlidingBuffer).
         * \param size The number of entities to construct.
         * \return The number of entities actually constructed.
         *         This shall not exceed size.
         */
        virtual std::size_t generate(T seg[],
                                     const std::size_t idx,
                                     const std::size_t size) = 0;
        
        /*!
         * Determine whether a subsequent call to `generate()` would
         * result in more entities becoming available.
         *
         * \return `true` if more data can be read, `false` if not.
         */                            
        virtual bool more(void) = 0;
    };
    
    /*!
     * A Segment is an aggregation of base data with a SlidingBuffer.
     * It is the underlying blocksize which is required of a
     * SegmentProducer.
     * 
     * Segements provide an implementation of the `[]` operator;
     * however, access to the underlying data would normally be
     * via the aggregating class SlidingBuffer. Classes derived
     * from SlidingBuffer may need to access the segements' data
     * directly, however.
     *
     * \tparam T The type of each entity stored in the Segment.
     * \tparam P The type of the Segment's Producer. 
     */
    template <typename T, typename P> class Segment {
    protected:
        T* seg;                   //!< Dynamic array of entities in storage
        bool valid;               /*!< Indicates entities have been constructed
                                    (for deferred initialisation: not yet implemented). */
        std::size_t size;         //!< Number of entities held
        const std::size_t origin; /*!< Index of the first entity in this Segment
                                    from the point of view of the SlidingBuffer. */
        P& sp;                    //!< The producer invoked to fill this Segment
    public:
        /*! Construct a Segment
         * \param size The number of base entities to be held in the segment.
         * \param origin The offset with in the aggregating SlidingBuffer
         * of the first element in this Segment
         * \param sp The Producer for this Segment
         */
        Segment(const std::size_t size, const std::size_t origin, P& sp) 
            : seg { new T [size] }
            , valid { false }
            , size { sp.generate(seg, origin, size) } // size is number of elements (channels)
            , origin { origin }
            , sp { sp }
        { }
        
        /*! Destroy this object and all the entities it contains */
        virtual ~Segment() {
            delete[] seg;
        }
        
        /*!
         * Access an entity, waiting for it to be constructed if
         * deferred construction is still taking place.
         * 
         * Since deferred construction is not yet implemented,
         * this method currently returns the entity reference immediately.
         * 
         * \param idx Index of the entity within this segment.
         * \returns Reference to the indexed entity
         */
        T& operator[](std::size_t idx) {
            /*TODO Check the Segment Producer has more data */
            if (!valid) {
//std::cout << "\nSegment origin " << origin << " not yet valid... calling producer\n";
                valid = true;
            }
//std::cout << "\nSeg idx: " << idx << std::endl;
            return seg[idx];
        }
        
        /*! Get the Segment's size
         * 
         * \returns Number of entities stored in this Segment
         */
        std::size_t get_size(void) const { return size; }
    };
    
    /*!
    * SlidingBuffer is class template which can be used when reading
    * a data stream to provide access to the stream's immediate history.
    * 
    * # Structure of a SlidingBuffer
    * 
    * A SlidingBuffer is composed of a number of
    * \link Segment Segments \endlink. It provides a
    * method of accessing the segments through the [] operator so that
    * the reading and discarding of segments is automatic and transparent.
    * This means the data stream can be accessed as if it it were a
    * conventional array (or other container supporting the
    * `operator[]` method), except that as the greatest index to date
    * increases, older elements are forgotten.
    * 
    * Segments are created and filled on demand. As the number of Segments specified
    * at instantiation is exceeded, the oldest Segments are discarded.
    * This means that the recent history of the buffered data is always
    * available, "recent" being defined by the size of the SlidingBuffer.
    * 
    * SlidingBuffers may only slide forward, which is to say that
    * the accessed index must never be less than the length of the
    * buffer prior to the greatest index ever accessed. Failing
    * to comply with this constraint causes an exception to be thrown.
    * 
    * For example, suppose that a buffer comprising four segments
    * each of three integers is created.
    * 
    * 
    * TODO The following isn't implemented yet...
    * `SegmentProducer::generate()` is called asynchronously to fill
    * new Segments. A separate thread is created to calculate the
    * datasamples used to populate the segment. In the event that
    * a SlidingBuffer Segment is accessed before the SegmentProducer
    * has finished filling it, the access blocks until such time as
    * tne data becomes available.
    * END TODO
    * 
    * Mindful that the act of filling a SlidingBuffer may be
    * (and usually is) stateful, although the accessed index can
    * be advanced arbitrarily towards the end of the SlidingBuffer,
    * the SegmentProducer will be called to generate data up until the point
    * it is required, attempting to access data
    * before the beginning of the SlidingBuffer will cause an exception to be thrown,
    * even though the index may be well above 0.
    * 
    * \tparam T The type of the primitive data held in the buffer
    * \tparam S The types of the segments comprising the buffer
    * \tparam P The type of the producer
    * 
    * \sa \link SlidingBufferExample Sliding Buffer: Internal Operation\endlink
    */        
    template <typename T, typename S, typename P> class SlidingBuffer {
    protected:
        std::size_t origin;          /*!< Lowest index in buffer */
        /*! The largest number of segments in the buffer
         *  A history of at least \f$seg_size\cdot(max_segs-1)\f$ entities
         *  will be made available subject to sufficient data having
         *  been read from the data stream
         */
        const std::size_t max_segs;
        const std::size_t seg_size;   /*!< The number of entities in each segment */
        std::deque<S*, UniqueAllocator<S*>> segs;/*!< Segments held by the buffer */
        P& sp;                        /*!< The producer to call to fill a segment */
    public:
        /*!
         * Create a SlidingBuffer. Starting at any point in the buffer,
         * a history of at least `seg_size*(max_segs-1)` entities
         * is available.
         * 
         * \param sp The producer whose `generate()` method is invoked to
         *        populate a new segment
         * \param max_segs The maxiumum number of segments stored in the buffer
         * \param seg_size The number of entities in each segment
         */
        SlidingBuffer(P& sp,
                      const std::size_t max_segs,
                      const std::size_t seg_size
                      )
            : origin { 0 }
            , max_segs { max_segs }
            , seg_size { seg_size }
            , sp { sp }
        {
//              for (std::size_t i { 0 }; i < max_segs; i++)
//                  segs[i] = nullptr;
//                 segs[i] = new S(seg_size, seg_size*i, sp);
        };
        
        /*! Destructor */
        virtual ~SlidingBuffer() { };
        
        /*! Access an element of the SlidingBuffer
         * 
         * Return a reference to the element of the SlidingBuffer
         * or throw an exception if it is not available
         * \param idx Index to access.
         * \throws NegativeIndexException The index is less than 0.
         * \throws ExpiredIndexException The indexed element is no longer available.
         * \throws IndexOutOfRangeException The indexed element is not yet
         * in the SlidingBuffer and can not be produced by the Producer.
         */
        T& operator[](long int idx)  {

            if (idx < 0)
                throw NegativeIndexException(idx);
            
            if (static_cast<size_t>(idx) < origin) // Avoid casting origin to int
                throw ExpiredIndexException(idx);
            
            std::size_t segment { (static_cast<std::size_t>(idx) - origin) / seg_size };

            while (segment >= segs.size()) {
                
//std::cout << "segment: " << segment << ", origin: " << origin << ", segs.size: " << segs.size() << std::endl;
                    
                segs.push_back(new S(seg_size,
                                     origin + segs.size() * seg_size,
                                     sp)
                              );
                
                if (segs.size() > max_segs) {
                    //delete segs.front();
                    segs.pop_front();
                    origin += seg_size;
                    --segment;
                }

            }

            S* const s { segs[segment] };
            std::size_t const i { idx%seg_size };

            if (s->get_size() <= i)
                throw IndexOutOfRangeException(idx);
//std::cout << "Access " << segment << "[" << i << "]\n";
            return (*s)[i];
        }
    };

} // namespace slidingbuffer

#endif // _SLIDINGBUFFER_H_
