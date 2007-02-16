#ifndef GRAEHL__SHARED__NULL_DELETER_HPP
#define GRAEHL__SHARED__NULL_DELETER_HPP

namespace graehl {

struct null_deleter {
    void operator()(void*) const {}
};

}



#endif
