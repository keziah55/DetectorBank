#include <tap++.h>
#include <string>

#include <detectorbank.h>
#include <notedetector.h>

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

int main() {
  plan(1);
//   ok(true, "This test passes");
//   is(foo(), 1, "foo() should be 1");
//   is(bar(), "a string", "bar() should be \"a string\"");
  ok(create(), "Allocate a detectorbank of 88 channels");
  return exit_status();
}
