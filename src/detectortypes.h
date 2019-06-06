#ifndef _DETECTORTYPES_H_
#define _DETECTORTYPES_H_

#include <complex>
#include <map>
#include <vector>

typedef double parameter_t;
typedef double result_t;
typedef float inputSample_t;
typedef std::complex<result_t> discriminator_t;
typedef std::map<std::size_t, std::vector<std::size_t>> Onsets_t;

#endif
