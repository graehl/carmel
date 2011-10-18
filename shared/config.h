#ifndef CARMEL_CONFIG_H
#define CARMEL_CONFIG_H

#include <sys/types.h>

// required now!  because values are never moved in memory from the singly linked nodes they live on.  so no assign/copy needed - can have hash_map of containers efficiently
#define USE_GRAEHL_HASH_MAP
// with stdext::hash_map, copies may be made of values (not tested lately)

// also required now.
// require USE_SLIST w/ -? deriv caching, for the same reason as USE_GRAEHL_HASH_MAP - list of containers guarantees no copies
#define USE_SLIST
//FIXME: USE_SLIST causes crash in kbest output with -O3 but not -O2.

#ifdef _MSC_VER
#ifndef USE_GRAEHL_HASH_MAP
#define USE_GRAEHL_HASH_MAP
#endif
#endif


#ifndef SINGLE_PRECISION
#define DOUBLE_PRECISION
#endif

#define GRAEHL_USE_LOG1P
// disable above if math.h doesn't include log1p, or if you want a speedup (but losing accuracy in Weight addition of small numbers to larger ones)

//#define CARMEL_DEBUG_PRINTS

#define ALLOWED_FORWARD_OVER_BACKWARD_EPSILON 1e-5

#if defined(DEBUG) && defined(CARMEL_DEBUG_PRINTS)
//# define DEBUG_STRINGPOOL
#define DEBUGLEAK
#define DEBUG_ESTIMATE_PP
#define DEBUGNAN
#define DEBUGDERIVATIONS
#define DEBUGTRAIN
#define DEBUGTRAINDETAIL
#define DEBUGFB
#define DEBUGNORMALIZE
#define DEBUGKBEST
#define DEBUG_RANDOM_GENERATE
#define DEBUGPRUNE
#define DEBUGCOMPOSE
#define DEBUG_ADAPTIVE_EM
#endif

#ifdef DEBUG
//# define DEBUG_GIBBS
//#define DEBUGNORMALIZE
#endif

//#define DEBUGNAN


#ifdef DEBUGNORMALIZE
#define CHECKNORMALIZE
#endif


#include <iostream>
namespace Config {
  inline std::ostream &out() { return std::cout; }
  inline std::ostream &err() {
    return std::cerr;
  }
  inline std::ostream &message() {
    return std::cerr;
  }
  inline std::ostream &log() {
    return std::cerr;
  }
  inline std::ostream &debug() {
    return std::cerr;
  }
  inline std::ostream &warn() {
    return std::cerr;
  }
}

#ifndef FLOAT_TYPE
#ifdef DOUBLE_PRECISION
# define FLOAT_TYPE double
#else
# define FLOAT_TYPE float
#endif
#endif

#include <graehl/shared/memleak.hpp>

namespace graehl {typedef short unsigned var_type; // 0 = no var, 1,2,3,... = var index
}

#define COPYRIGHT_YEAR 2011

//do this in Makefile for consistency with boost test lib src that don't include me:
//#define BOOST_DISABLE_THREADS
//#define BOOST_NO_MT


//#define UNORDERED_MAP

#define STATIC_HASH_EQUAL
#define STATIC_HASHER

// if not STRINGPOOL, then same string -> different address (but no global hashtable needed)
#if !defined(DEBUG)
#define STRINGPOOL
#endif
#ifndef STRINGPOOLCLASS
#define STRINGPOOLCLASS StringPool
#endif

// reference counts of alphabet symbols/state names - might save a little memory and could hurt or help performance





// use old, slower string hash
//#define OLD_HASH

// for meaningful compose state names
#define MAX_STATENAME_LEN 15000

#ifdef _MSC_VER
#pragma warning( disable : 4355 )
#endif


#define MAX_LEARNING_RATE_EXP 20

#define WEIGHT_FLOAT_TYPE FLOAT_TYPE
// unless defined, Weight(0) will may give bad results when computed with, depending on math library behavior
#define WEIGHT_CORRECT_ZERO
// however, carmel checks for zero weight before multiplying in a bad way.  if you get #INDETERMINATE results, define this
// definitely needs to be defined for Microsoft (debug or release) now



#endif //guard
