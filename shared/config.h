#ifndef CONFIG_H 
#define CONFIG_H 1

// options affecting a bunch of code:
//#define DEBUG

#ifdef DEBUG
#define DEBUGTRAIN
#define DEBUGTRAINDETAIL
#define DEBUGKBEST
#define DEBUGFB
#define DEBUGCOMPOSE
#define ALLOWED_FORWARD_OVER_BACKWARD_EPSILON 1e-5
#undef MEMDEBUG // link to MSVCRT
#endif

// unless defined, Weight(0) will may give bad results when computed with, depending on math library behavior
#define WEIGHT_CORRECT_ZERO
// however, carmel checks for zero weight before multiplying in a bad way.  if you get #INDETERMINATE results, define this

// bunch of small allocators wastes memory but faster new/delete
#define CUSTOMNEW
// crashes if not defined (??!)

// weak attempt at handling infinite sum of *e* paths with finite approximation.  not recommended, as the math is untested and suspect
#undef N_E_REPS

// allows WFST to be indexed in either direction?  not recommended.
#undef BIDIRECTIONAL

#endif //guard
