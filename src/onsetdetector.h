#ifndef _ONSETDETECTOR_H_
#define _ONSETDETECTOR_H_

#include <vector>
#include <deque>
#include <utility>

#include <string>
#include <iostream>
#include <fstream>

#include "detectorbank.h"
#include "detectorcache.h"


/*! Onset detector
 */
class OnsetDetector {

public:
    /*! Construct an onset detector, given a DetectorBank.
     * \param db DetectorBank for this critical band
     * \param offset Number of pad samples at beginning of the input buffer
     */
    OnsetDetector(DetectorBank& db, const std::size_t offset, 
                  std::string csvfile);

    /*! Analyse the input and return any onsets
     * \param threshold amplitude above which DetectorBank output must be
     * in order to qualify as an onset
     * \return vector of onset sample times
     */
    void analyse(std::vector<std::size_t> &onsets, result_t threshold);//, 
//                  double w0, double w1, double w2);

//     std::vector<std::size_t> analyse(result_t threshold, std::string outfile);

protected:
     /*! The DetectorBank which produces the results */
    DetectorBank& db;
    /*! Number of pad samples at beginning of input buffer */
    const std::size_t offset;
    /*! Number of channels */
    const std::size_t chans;
    /*! Sample rate */
    const parameter_t sr;
    /*! DetectorCache Producer */
    std::unique_ptr<detectorcache::Producer> p;
    /*! OnsetDetector and DetectorCache segment length */
    const std::size_t seg_len;
    /*! Number of segments to be stored */
    const std::size_t num_segs;
    /*! DetectorCache from which to get abs(z) values */
    std::unique_ptr<detectorcache::DetectorCache> cache;
    /*! Current sample number */
    std::size_t n;
    /*! Total number of samples in cache */
    std::size_t end;
    /*! Get average of current segment*/
    result_t getSegAvg();
    /*! Verify or reject onsets found by `analyse` */
    std::pair<bool, std::size_t> findExactTime(std::size_t incStart,
                                               std::size_t incStop);  
    /*! Get item from DetectorCache and calculate log. If item is 0, 0 is returned */
    result_t logResult(std::size_t channel, std::size_t idx);
    std::ofstream logfile;
    std::ofstream segfile;
};


#endif
