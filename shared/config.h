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

// bunch of small allocators wastes memory but faster new/delete
#define CUSTOMNEW

// weak attempt at handling infinite sum of *e* paths with finite propogation.  not recommended
#undef N_E_REPS

// allows WFST to be indexed in either direction?  not recommended.
#undef BIDIRECTIONAL

#endif //guard
