#ifndef CONFIG_H
#define CONFIG_H

//#define DEBUG

#include "memleak.hpp"

#define COPYRIGHT_YEAR 2004

//#define DEBUGNAN

//do this in Makefile for consistency with boost test lib src that don't include me:
//#define BOOST_DISABLE_THREADS
//#define BOOST_NO_MT


//#define UNORDERED_MAP

#define STATIC_HASH_EQUAL
#define STATIC_HASHER

#define STRINGPOOL
#ifndef STRINGPOOLCLASS
#define STRINGPOOLCLASS StringPool
#endif

// reference counts of alphabet symbols/state names - might save a little memory and could hurt or help performance



// use singly linked list
#define USE_SLIST


// use old, slower string hash
//#define OLD_HASH

#define DOUBLE_PRECISION

#ifdef DOUBLE_PRECISION
typedef double FLOAT_TYPE;
#else
typedef float FLOAT_TYPE;
#endif


// for meaningful compose state names
#define MAX_STATENAME_LEN 15000

#ifdef _MSC_VER
#pragma warning( disable : 4355 )
#endif


#ifdef DEBUG
#define DEBUGLEAK
#define DEBUG_ESTIMATE_PP
#define DEBUGNAN
//#define DEBUGTRAIN
//#define DEBUGTRAINDETAIL
//#define DEBUGNORMALIZE
#define DEBUGKBEST
//#define DEBUGPRUNE
//#define DEBUGFB
#define DEBUGCOMPOSE
#define DEBUG_ADAPTIVE_EM
#define ALLOWED_FORWARD_OVER_BACKWARD_EPSILON 1e-3
#endif


#ifdef DEBUGNORMALIZE
#define CHECKNORMALIZE
#endif

#define MAX_LEARNING_RATE_EXP 20

// unless defined, Weight(0) will may give bad results when computed with, depending on math library behavior
#define WEIGHT_CORRECT_ZERO
// however, carmel checks for zero weight before multiplying in a bad way.  if you get #INDETERMINATE results, define this
// definitely needs to be defined for Microsoft (debug or release) now

// allows WFST to be indexed in either direction?  not recommended.
//#define BIDIRECTIONAL

#include <iostream>
namespace Config {
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
};

#endif //guard
