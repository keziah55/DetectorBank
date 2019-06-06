#include <tap++.h>
#include <string>

#include <detectorbank.h>
#include <notedetector.h>

#include <iostream>
#include <iomanip>

using namespace TAP;

// Typical defaults

constexpr std::size_t inputBufferSize {1024};
static inputSample_t buffer[inputBufferSize];
static const parameter_t edo {12};

static const parameter_t frequencies[] = {261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440.000, 466.164, 493.883, 523.251};
static const std::size_t freqsSize {
    sizeof(frequencies)/sizeof(parameter_t)
};

bool default_args(void)
{    
    std::cout << "\n\nConstructing NoteDetector with frequencies:\n";
    std::ios::fmtflags f { std::cout.flags() };
    std::cout <<  std::fixed << std::setprecision(3);
    for (int i {0} ; i < freqsSize ; i++)
            std::cout << std::setw(10) << frequencies[i] << std::endl;
    std::cout.flags(f);

    NoteDetector nd(48000, buffer, inputBufferSize,
                    frequencies, freqsSize,
                    edo);
                              
    std::cout << "\nBW: " << nd.get_bandwidth()
              << "; features: " << nd.get_features()
              << "; damping: " << nd.get_damping()
              << "; gain: " << nd.get_gain() << std::endl;

    return nd.get_bandwidth() == NoteDetector::default_bandwidth &&
           nd.get_features()  == NoteDetector::default_features  &&
           nd.get_damping()   == NoteDetector::default_damping   &&
           nd.get_gain()      == NoteDetector::default_gain;
}

bool ndarg1(void)
{
    std::cout << "\n\nConstructing NoteDetector with NDOptArgs (defaults):";

    NoteDetector nd(48000, buffer, inputBufferSize,
                    frequencies, freqsSize, edo,
                    NDOptArgs()
                   );
                              
    std::cout << "\nBW: " << nd.get_bandwidth()
              << "; features: " << nd.get_features()
              << "; damping: " << nd.get_damping()
              << "; gain: " << nd.get_gain() << std::endl;

    return nd.get_bandwidth() == NoteDetector::default_bandwidth &&
           nd.get_features()  == NoteDetector::default_features  &&
           nd.get_damping()   == NoteDetector::default_damping   &&
           nd.get_gain()      == NoteDetector::default_gain;
}

bool ndarg2(void)
{
    std::cout << "\n\nConstructing NoteDetector with NDOptArgs (half the gain, twice the damping):";

    NoteDetector nd(48000, buffer, inputBufferSize,
                    frequencies, freqsSize, edo,
                    NDOptArgs()
                        .gain(NoteDetector::default_gain/2.)
                        .damping(NoteDetector::default_damping*2.)
                   );
                              
    std::cout << "\nBW: " << nd.get_bandwidth()
              << "; features: " << nd.get_features()
              << "; damping: " << nd.get_damping()
              << "; gain: " << nd.get_gain() << std::endl;

    return nd.get_bandwidth() == NoteDetector::default_bandwidth  &&
           nd.get_features()  == NoteDetector::default_features   &&
           nd.get_damping()   == NoteDetector::default_damping*2. &&
           nd.get_gain()      == NoteDetector::default_gain/2.;
}

int main() {
  plan(3);
  
  ok(default_args(),
     "Create a note detector with default arguments");
  ok(ndarg1(),
     "Create a note detector with an NDOptArg object");
  ok(ndarg2(),
     "Create a note detector with an NDOptArg, specify gain & damping");
  
  return exit_status();
}
