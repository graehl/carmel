#ifndef GRAEHL_SHARED__UNIMPLEMENTED_HPP
#define GRAEHL_SHARED__UNIMPLEMENTED_HPP

#include <stdexcept>

namespace graehl {

struct unimplemented_exception : public std::runtime_error
{
    unimplemented_exception(char const* c) : std::runtime_error(c) {  }
};

inline void unimplemented(char const* m="unimplemented") {
    throw unimplemented_exception(m);
}

}

#endif
