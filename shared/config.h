#ifndef CONFIG_H 
#define CONFIG_H 1

#define STRINGPOOL 
// reference counts of alphabet symbols/state names - might save a little memory and could hurt or help performance

//#define DEBUG

// use singly linked list
#define USE_SLIST

// for meaningful compose state names
#define MAX_STATENAME_LEN 15000

#define PLACEMENT_NEW new

#ifdef DEBUG
#ifdef _MSC_VER
//#define MEMDEBUG // link to MSVCRT
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define INITLEAK _CrtMemState s1, s2, s3; do { _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_FILE );_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR ); _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );} while(0)
#define CHECKLEAK(i) do {  _CrtMemCheckpoint( (((i)%2)?&s1:&s2) ); if ((i)>0) { _CrtMemDifference( &s3, (((i)%2)?&s2:&s1), (((i)%2)?&s1:&s2) ); _CrtMemDumpStatistics( &s3 );} } while(0)
#define NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define NEW new
#define INITLEAK
#define CHECKLEAK(i)
#endif
#define DEBUGLEAK
#define DEBUGTRAIN
//#define DEBUGTRAINDETAIL
#define DEBUGNORMALIZE
//#define DEBUGKBEST
#define DEBUGPRUNE
//#define DEBUGFB
#define DEBUGCOMPOSE
#define ALLOWED_FORWARD_OVER_BACKWARD_EPSILON 1e-4
#else
#define NEW new
#define INITLEAK
#define CHECKLEAK(i)
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
