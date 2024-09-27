#include <tap++.h>
#include <string>
#include <map>
#include <rapidxml/rapidxml.hpp>

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

// Read the salient parameters from a Detector node
// and return a map of them.
typedef std::map<std::string, double> DetectorParams;
DetectorParams get_params(rapidxml::xml_node<>* detector)
{
    DetectorParams results;

    for ( rapidxml::xml_node<>* n { detector->first_node() } ;
          n != nullptr ;
          n = n->next_sibling() ) {
        //std::cout << n->name() << ":\t" << n->value() << std::endl;
        if (std::string(n->name()) != "AbstractDetector")
            results[n->name()] = std::stod(n->value());
        else // flatten the abstact detector's data into the result map
            for ( rapidxml::xml_node<>* adn { n->first_node() } ;
                  adn != nullptr ;
                  adn = adn->next_sibling() ) {
                //std::cout << adn->name() << ":\t" << adn->value() << std::endl;
                if ( std::string(adn->name()) == "aScale" ) {
                    // deal separately with complex parts
                    rapidxml::xml_node<>* parts { adn->first_node() };
                    results["aScale.re"] = std::stod(parts->value());
                    results["aScale.im"] = std::stod(parts->next_sibling()->value());
                } else {
                    results[adn->name()] = std::stod(adn->value());
                }
            }
    }
    
    return results;
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
    //std::cout << det_info << std::endl;
    rapidxml::xml_document<> xml_doc;
    // det_info is desposible, so don't need to copy it.
    xml_doc.parse<0>(&det_info[0]);
    
    // Just one detector, so find it and read its attributes.
    rapidxml::xml_node<>* root { xml_doc.first_node() };
    rapidxml::xml_node<>* detector_node {
        root->first_node("Detectors")->first_node("Detector")
    };

    DetectorParams dp { get_params(detector_node) } ;
    
    for ( const auto kvp : dp)
        std::cout << kvp.first << ":\t" << kvp.second << std::endl;
    
    // Normally we shouldn't compare doubles for eqality, but here we're
    // testing that they've not been touched since they were set with
    // constants.
    
    return dp["aScale.re"] == 1 && dp["aScale.im"] == 0
        && dp["iScale"] == 1
        && dp["w_in"] == dp["w_adjusted"];
}


int main() {
  plan(1);
//   ok(true, "This test passes");
//   is(foo(), 1, "foo() should be 1");
//   is(bar(), "a string", "bar() should be \"a string\"");
  is(ud_gain(), true, "Unnormalized detectors should have unity gain");
  
  return exit_status();
}
