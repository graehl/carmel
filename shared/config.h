#ifndef CONFIG_H 
#define CONFIG_H 1

// options affecting a bunch of code:
//#define DEBUG

// use singly linked list
//#define USE_SLIST

#ifdef DEBUG
#define DEBUGTRAIN
#define DEBUGTRAINDETAIL
#define DEBUG_NORMALIZE
#define DEBUGKBEST
#define DEBUGPRUNE
//#define DEBUGFB
#define DEBUGCOMPOSE
#define ALLOWED_FORWARD_OVER_BACKWARD_EPSILON 1e-4
//#define MEMDEBUG // link to MSVCRT
#endif

// unless defined, Weight(0) will may give bad results when computed with, depending on math library behavior
#define WEIGHT_CORRECT_ZERO
// however, carmel checks for zero weight before multiplying in a bad way.  if you get #INDETERMINATE results, define this


// special-purpose allocators for hash tables - never reclaims memory for general pool, but faster (?)
//#define CUSTOMNEW

// allows WFST to be indexed in either direction?  not recommended.
//#define BIDIRECTIONAL

#include <iostream>
namespace Config {
	inline std::ostream &log() {
		return std::cerr;
	}
	inline std::ostream &debug() {
		return std::cerr;
	}
	inline std::ostream &warn() {
		return std::cerr;
	}
};

#endif //guard
