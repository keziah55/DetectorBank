#include <vector>
#include <deque>
#include <utility>
#include <cmath>
#include <numeric>
#include <iterator>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "detectorbank.h"
#include "detectorcache.h"
#include "onsetdetector.h"
#include "slidingbuffer.h"

OnsetDetector::OnsetDetector(DetectorBank& db, const std::size_t offset, 
                             std::string csvfile) 
    : db( db )
    , offset( offset )
    , chans( db.getChans() )
    , sr( db.getSR() )
    , p( new detectorcache::Producer(db) )
    , seg_len( static_cast<std::size_t>(0.02 * sr) )  // segment length is 20ms
    , num_segs ( 1000 )                                // store 1000 segments, i.e. 20 seconds
    , cache( new detectorcache::DetectorCache(*p, num_segs, seg_len) )
    , n ( 0 )
    , end ( cache->end() )
{
    // reset detectorbank to the beginning of the input, if necessary
    if (db.tell() > 0)
        db.seek(0);
    
    std::ostringstream oss1;
    oss1 << csvfile << ".csv";
    std::string segname = oss1.str();
    segfile.open(segname);
    
    std::ostringstream oss2;
    oss2 << csvfile << "_debug.txt"; 
    std::string logname = oss2.str();
    logfile.open(logname);
    logfile << std::fixed;
}

void OnsetDetector::analyse(std::vector<std::size_t> &onsets, 
                            result_t threshold)
{      
    threshold = std::log(threshold);
    
//     result_t current;
    result_t last;
    
    std::size_t start;
    std::size_t stop;
    
    // count all segments
    std::size_t seg_count {0};
    
    // vector to store continuously increasing segment values
    std::vector<result_t> segments;
    
    // this is for the debug printing
    std::size_t sample;
    
    while (p->more()) {
        
        if (!segments.empty())
            last = segments.back();
        else
            last = 0;
        
        // get mean log and place in vector
        segments.push_back(getSegAvg());
        
        // increment count of segments
        seg_count++;
        
        if (seg_count*seg_len >= offset)
            segfile << segments.back() << "\n";
        
        // if three criteria are met, an onset may have occured
        if (segments.back() < last && segments.size() >= 3 && last >= threshold) {
            
            sample = (seg_count-segments.size())*seg_len;
            logfile << "Sample: " << sample << ", time: " << sample/sr << ", seg_count: " << seg_count << "\n";
            logfile << "    Count: " << segments.size() << "\n";
            logfile << "    Last: " << last << ", threshold: " << threshold << "\n";
            logfile << "    Last: " << last << ", first: " << segments.front() << ", last-first: log(" << std::exp(last - segments.front()) << ")\n";
            
            if (last-segments.front() >= std::log(2)) {
                
                logfile << "    Detection accepted\n";
                
                // get sample number at beginning of increase
                if (segments.size() != seg_count)
                    start = (seg_count-segments.size())*seg_len;
                else
                    start = 0;
                
                // get sample number at end of largest increase
                if (seg_count >= 1) {
                    // overwrite abs segments with differences
                    std::adjacent_difference(segments.begin(), segments.end(), segments.begin());
                    // find max
                    std::vector<result_t>::iterator mx {std::max_element(segments.begin()+1, segments.end())};
                    // find index of max
                    long i {std::distance(segments.begin(), mx)};
                    // get sample number at end of largest increase
                    stop = start + (i * seg_len);
                }
                else
                    stop = 0;
                
                logfile << "    Searching between sample " << start << " and " << stop << " for onset\n";
                
                // ..verify or reject
                std::pair<bool, std::size_t> onset = findExactTime(start, stop);
                int onsetTime;
                // if verified, put in `onsets` vector
                if (onset.first) {
                    if (onset.second < offset)
                        onsetTime = 0;
                    else
                        onsetTime = static_cast<int>(onset.second-offset);
                    onsets.push_back(onsetTime);
                    logfile << "    Onset found at " << onsetTime << " samples, " << onsetTime/sr << " seconds\n\n";
                }
                else
                    logfile << "    Onset not verified\n\n";
            }
            else
                logfile << "    Detection rejected\n\n";
                
            // reset vector
            segments.clear();
        }
    }
    logfile.close();
    segfile.close();
}

result_t OnsetDetector::getSegAvg()
{
    // get the average of the entire segment
    result_t seg_avg {0};
    result_t result;
           
    // for number of samples in a segment, m
    for (std::size_t m {0}; m<seg_len; m++) {
        if (n >= end) // check that we've not reached the end of the input
            break;
        else {
            // add the average at current sample, n
            for (std::size_t k {0}; k<chans; k++) {
                seg_avg += logResult(k, n);
//                 result = cache->getResultItem(k, n);
//                 if (result != 0)
//                     seg_avg += std::log(result);
            }
        }   
        n++;
    }  
    // calculate average of segment
    seg_avg /= (chans*seg_len);
    
    return seg_avg;
}

result_t OnsetDetector::logResult(std::size_t channel, std::size_t idx)
{
    // get value from the DetectorCache
    // if value is not zero, return log 
    // otherwise, return zero
    result_t resultItem;
    resultItem = cache->getResultItem(channel,idx);
    if (resultItem != 0)
        return std::log(resultItem);
    else
        return 0.;
}

std::pair<bool, std::size_t> OnsetDetector::findExactTime(std::size_t incStart, 
                                                          std::size_t incStop)
{
    // `incStart` is sample number of beginning of first segment in run
    // `incStop`  is sample number of beginning of last segment in run
    
    std::size_t stop_time = static_cast<std::size_t>(sr*0.1) ;
    
    // look from 100ms before incStart (or to zero if we are too close)
    std::size_t stop;
    if (stop_time > incStart) // cannot check if incStart-stop_time < 0 as both are size_t i.e. unsigned ints
        stop = 0;
    else
        stop = incStart - stop_time;
    
    std::size_t idx {incStop};    
      
    // get current value (i.e. mean log of band at current sample)
    result_t current {0};
    for (std::size_t k {0}; k<chans; k++)
        current += logResult(k, idx);
    current /= chans;
    
    // calculate averages over 75ms (or however many samples we have)
    std::size_t N {static_cast<std::size_t>(sr*0.075)};
    if (N > idx) 
        N = idx;
    
    // mean of whole block
    result_t mean {0};
    
    // get mean log of N values before current sample
    for (std::size_t i {idx-N}; i<idx; i++) {
        for (std::size_t k {0}; k<chans; k++)
            mean += logResult(k,i);
    }
    mean /= (chans*N);
    
    logfile << "    Backtracking from sample " << idx << " to " << stop << "...\n";
    
    // go backwards until have either verified the onset or reach `stop` value
    while (idx > stop+N) {
        
        logfile << "        sample: " << idx << ", mean: " << mean << ", current: " << current << ", mean<current: ";        
        
        if (mean < current) {
            logfile << "True\n";
            // if amplitude of previous values < current, keep going backwards
            idx--;
            
            current = 0;
            for (std::size_t k {0}; k<chans; k++)
                current += logResult(k,idx);
            current /= chans;
            
            // remove most recent from mean...
            mean -= (current / N);
            
            // ...and add an older sample
            result_t older {0};
            for (std::size_t k {0}; k<chans; k++)
                older += logResult(k,idx-N);
            mean += (older / (chans*N));
            
        }
        // otherwise, get min in previous 10ms from current
        // if this min is significantly different from current,
        // return time at min as onset. Otherwise, return time at current
        else {
            logfile << "False\n";            
            result_t mn {current};
            std::size_t onset {idx};
            std::size_t M {static_cast<std::size_t>(sr*0.01)}; 
            
            for (std::size_t i {idx}; i>=idx-M; i--) {
                result_t avg {0};
                for (std::size_t k {0}; k<chans; k++) 
                        avg += logResult(k,i); 
                avg /= chans;
                        
                if (avg<mn) {
                    mn = avg;
                    onset = i;
                }
            }
            
            logfile << "    local min: " << mn << ", current/mn: " << current/mn << "\n";
            
            //if local min and current are v similar, or local min is zero, use current time as onset
            if (std::isnan(current/mn) || current/mn >= 0.95) {
                onset = idx;
                logfile << "    therefore, not returning local min time\n";
            }
            else
                logfile << "    therefore, returning local min time\n";
            
            return std::make_pair(true, onset);
        }
    }
    // if we have got to the end of the while loop, we haven't found an onset
    return std::make_pair(false, 0);
}
