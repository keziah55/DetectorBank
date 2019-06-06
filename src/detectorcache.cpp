#include "slidingbuffer.h"
#include "detectorbank.h"
#include "detectorcache.h"

#include <algorithm>

#include <iostream>

std::size_t detectorcache::Producer::generate(result_t* seg[],
                                              const std::size_t idx,
                                              const std::size_t size) {
    
    // size is the number of channels (i.e. chans)
    // (but we're not technically using it here)
    
//     const std::size_t chans = db.getChans();
    const std::size_t chans = size;
    
    discriminator_t* zComplex = new discriminator_t[seg_len*chans];
    //TODO Check that SlidingBuffer guarantees to make idx increase by size
    //TODO Maybe worth an assert here.
    
    // If the target structure is null, reserve memory for it.
    result_t* storage = new result_t[seg_len*chans];
#   if DEBUG & 4
    std::cout << "detectorcache::Producer::generate: reserved "
              << seg_len*chans << " result_ts at " << storage
              << std::endl;
#   endif    
    for (std::size_t i {0} ; i < chans ; i++, storage += seg_len) {
#       if DEBUG & 4
        std::cout << "... seg[" << i << "] = " << storage << "\n... ";
#       endif
        seg[i] = storage;
    }

    std::size_t samples_to_process{seg_len};
    
    // If we're operating in stream mode, read (up to) seg_len
    // data from the input stream and tell the DetectorBank to
    // use it.
    inputSample_t* audiobuf{nullptr};
    if (audiostream) {
        audiobuf = new inputSample_t[seg_len];
        audiostream->read(reinterpret_cast<char *>(audiobuf),
                          seg_len * sizeof(inputSample_t));
        samples_to_process = audiostream->gcount()/sizeof(inputSample_t);
        db.setInputBuffer(audiobuf, seg_len);
    }
    
    db.getZ(zComplex, chans, samples_to_process, start_chan);
    db.absZ(seg[0], chans, samples_to_process, zComplex);
    
    if (audiobuf) delete[] audiobuf;
    delete[] zComplex;

    //FIXME At the moment what gets returned is the number of channels
    // for which data is stored. Now it's possible to pass an istream,
    // would it be better to return the number of time-samples processed?
    // Then the caller would know if the last result happened after a partial
    // read because the input stream was exhausted.
    return size; // <- samples_to_process?
}


detectorcache::Segment::Segment (const std::size_t size, const std::size_t origin,
                                 Producer& sp)
    : slidingbuffer::Segment<result_t *, detectorcache::Producer>(size, origin, sp) // size is number of channels
{ 
#   if DEBUG & 4
    std::cout << "detectorcache::Segment::Segment(size=" << size
              << ", origin=" << origin << ")\n";
#   endif
}
        
detectorcache::Segment::~Segment() {
#   if DEBUG & 4
    std::cout << "Deleting segment at " << seg[0] << std::endl;
#   endif
    
    delete[] seg[0];
}


detectorcache::DetectorCache::DetectorCache(detectorcache::Producer& p,
                                            const std::size_t num_segs,
                                            const std::size_t seg_len,
                                            const std::size_t start_chan,
                                            const std::size_t num_chans)
    : slidingbuffer::SlidingBuffer<result_t *,
                                   detectorcache::Segment,
                                   detectorcache::Producer>
        (p,
         num_segs,
         num_chans)
        
    , chans { num_chans }
    , seg_len { seg_len }
    , p { p }
    , start_chan { start_chan }
{
#   if DEBUG & 4
    std::cout << "detectorcache::DetectorCache::DetectorCache: "
              << num_segs << " segments of " << seg_len << " samples and "
              << chans << " channels\n";
#   endif
    p.set_num_samps(seg_len);
    p.set_start_chan(start_chan);
}


result_t detectorcache::DetectorCache::getResultItem(long int ch, long int n) {
    
    // DetectorCache is a 2D mapping of SlidingBuffer where the channel number
    // is the SlidingBuffer index
    // if ch < 0, slidingbuffer::NegativeIndexException will be thrown, but
    // if n < 0, it won't
    // so throw it here, if necessary
    if (n < 0)
        throw slidingbuffer::NegativeIndexException(n);
    
    if (n >= static_cast<long int> (p.getBuflen())) { 
#       if DEBUG & 4
        std::cout << "detectorcache::DetectorCache::getResultItem: index "
                  << n << " of channel "
                  << ch << " exceeds buffer length ("
                  << p.getBuflen() << "): returning 0\n";
#       endif
        return 0;
    }
        
    const long int gen_buf_num { n / static_cast<long int> (seg_len) };

    const long int c { ch + gen_buf_num * static_cast<long int> (chans) };
    const long int i { n - gen_buf_num * static_cast<long int> (seg_len) };

#   if DEBUG & 4
    std::cout << "item:        Ch " << ch << "[" << n << "] -> buf #"
              << gen_buf_num << "( (*this)["
              << c << "][" << i << "]) = "
              << (*this)[c][i] << " at " 
              << &((*this)[c][i]) << std::endl;
#   endif
    
    return (*this)[c][i];
}

    
std::size_t detectorcache::DetectorCache
                         ::getPreviousResults(const std::size_t chan,
                                              const std::size_t currentSample,
                                              result_t* samples,
                                              std::size_t numSamples) {
    // For 0 requested samples, do nothing.
    if (numSamples == 0) return 0;
    
    std::size_t samplesDone {0};
    
    const std::size_t startSample   { currentSample - (numSamples-1) };
    // The start/current samples appear in this block of abs values
    const std::size_t currentBlkIdx { currentSample/seg_len };
    const std::size_t startBlkIdx   { startSample/seg_len };
    // ... to be found by accessing these segments
    const std::size_t currentSegIdx { chan + currentBlkIdx*chans };
          std::size_t startSegIdx   { chan + startBlkIdx*chans };
    // ... at these offsets
    const std::size_t currentOffs   { currentSample - seg_len*currentBlkIdx };
          std::size_t startOffs     { startSample - seg_len*startBlkIdx };
    
#   if DEBUG & 4
    std::cout << "detectorcache::DetectorCache::getPreviousResults: ch"
              << chan << '[' << startOffs << "] to " << samples << " for "
              << numSamples << std::endl;
#   endif
    
    // Copy all but the final absZ block's samples into the target array
    while (startSegIdx < currentSegIdx) {
#       if DEBUG & 4
        std::cout << "... seg " << startSegIdx << '(' << (*this)[startSegIdx]
                  << ") + " << startOffs << " seglen " << seg_len << std::endl;
#       endif

        std::copy((*this)[startSegIdx] + startOffs,
                  (*this)[startSegIdx] + seg_len,
                  samples);
        const std::size_t copied {seg_len - startOffs};
        samplesDone += copied;
        samples += copied;
        startSegIdx += chans;
        startOffs = 0;
    }
    
    // Copy the final absZ block's samples into the output array
#   if DEBUG & 4
    std::cout << "... seg " << startSegIdx << '(' << (*this)[startSegIdx]
              << ") + " << startOffs << " to " << currentOffs << std::endl;
#   endif
    std::copy((*this)[startSegIdx] + startOffs,
              (*this)[startSegIdx] + currentOffs + 1,
              samples);
    samplesDone += currentOffs+1 - startOffs;
    
    return samplesDone;    
}
