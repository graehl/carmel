#define GRAEHL__SINGLE_MAIN
#ifdef DEBUG
//# define TEST_ADD_ONE_LIMIT
# endif
#include "forest-em-params.hpp"
//#define SINGLE_PRECISION
//#define HINT_SWAPBATCH_BASE
#include <graehl/shared/config.h>
#include <memory> //auto_ptr
#include <graehl/shared/main.hpp>
#ifndef GRAEHL_TEST

using namespace boost;
using namespace std;
using namespace boost::program_options;
using namespace graehl;



//#define FOREST_EM_VERSION_STR(type,size) "sizeof(" #type ")=" FOREST_EM_STRINGIZE(size)
//#define FOREST_EM_VERSION_SIZE(name,type) FOREST_EM_VERSION_STR(name,sizeof(type))
//#define FOREST_EM_SIZE_COUNT sizeof(forest::count_t)
//#define FOREST_EM_VERSION_STRING FOREST_EM_VERSION "-" FOREST_EM_VERSION_STR(count,FOREST_EM_SIZE_COUNT)
//FOREST_EM_VERSION_SIZE(prob,forest::prob_t)

MAIN_BEGIN
{
    DBP_INC_VERBOSE;
#ifdef DEBUG
        DBP::set_logstream(&cerr);
#endif
//DBP_OFF;
        
        return forest_em_param.main(argc,argv);
        
}
MAIN_END

#endif

