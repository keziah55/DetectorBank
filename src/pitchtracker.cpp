#include <vector>
#include <deque>
#include <utility>

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>


#include "detectorbank.h"
#include "detectorcache.h"
#include "pitchtracker.h"

PitchTracker::PitchTracker(DetectorBank& db) 
    : db( db )
{
    // reset detectorbank to the beginning of the input, if necessary
    if (db.tell() > 0)
        db.seek(0);
}

std::vector<std::vector<double>> PitchTracker::analyse()
{    
    return pitches;
}

