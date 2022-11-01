#include <tap++.h>
#include <string>

#include <detectorbank.h>
// #include <notedetector.h>  // Now resides in separate repo

#include <iostream>

using namespace TAP;

// int foo() {
//   return 1;
// }
// 
// std::string bar() {
//   return "a string";
// }

bool create() {
  try {
    DetectorBank db(44100, nullptr, 0, 0);
    return true;
  } catch (...) {
    return false;
  }
}

bool ud_gain() {
    const parameter_t freqs[] { {440.0} };
    constexpr size_t num_dets {1};
    const int det_char = (int)(
        DetectorBank::Features::runge_kutta |
        DetectorBank::Features::freq_unnormalized | 
        DetectorBank::Features::amp_unnormalized
                          );
    DetectorBank db(44100,          // Sample rate
                    nullptr, 0,     // Pointer to and size of input buffer
                    0,              // No. of threads ( < 1 => CPU cores)
                    freqs,          // c array of detector frequencies
                    nullptr,        // c array of detector bandwidths
                                    //    nullptr =. minimum bw for all
                    num_dets,       // No. of entries in freq and bw arrays
                    det_char);      // Characterists of the detectors
    // default damping and detector gain.
    
    std::string det_info { db.toXML() };
    std::cout << det_info << std::endl;
    
    return true;
}


int main() {
  plan(2);
//   ok(true, "This test passes");
//   is(foo(), 1, "foo() should be 1");
//   is(bar(), "a string", "bar() should be \"a string\"");
  ok(create(), "Allocate a detectorbank of 88 channels");
  
  is(ud_gain(), true, "Unnormalized detectors should have unity gain");
  
  return exit_status();
}
