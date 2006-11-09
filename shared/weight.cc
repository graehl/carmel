#include "weight.h"

/* NOW HANDLED by funcs.hpp
#include <cstdlib>
#include <ctime>
struct InitRand {
        InitRand() {
                srand((unsigned int)time(NULL));
        }
};

static InitRand _Weight_Init_Rand;

*/

//using namespace std;

namespace graehl {

// xalloc gives a unique global handle with per-ios space handled by the ios
template <class Real>
const int logweight<Real>::base_index = std::ios_base::xalloc();
template <class Real>
const int logweight<Real>::thresh_index = std::ios_base::xalloc();
template <class Real>
THREADLOCAL int logweight<Real>::default_base = logweight<Real>::EXP;
template <class Real>
THREADLOCAL int logweight<Real>::default_thresh = logweight<Real>::ALWAYS_LOG;
}
