#define GRAEHL__SINGLE_MAIN

#include <graehl/shared/cmdline_main.hpp>

char const *usage_str="sample app\n"
    ""
    ;

using namespace std;

namespace graehl {

struct mate : public main
{
    mate() : main("mate",usage_str,"v1")
    {}
};

}

INT_MAIN(graehl::mate)


