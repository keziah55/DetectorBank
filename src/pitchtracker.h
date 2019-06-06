#ifndef _PITCHTRACKER_H_
#define _PITCHTRACKER_H_

#include <vector>
#include <deque>
#include <utility>

#include "detectorbank.h"
#include "detectorcache.h"


/*! Pitch Tracker
 */
class PitchTracker {

public:
    /*! Construct a PitchTracker
     */
    PitchTracker(DetectorBank& db);

    /*! Analyse the input and return frequency
     */
    std::vector<std::vector<double>> analyse();

protected:
     /*! The DetectorBank which produces the results */
    DetectorBank& db;
    /*! Vector of vectors of pitches */
    std::vector<std::vector<double>> pitches;
};


#endif
