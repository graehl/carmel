/** \file

    make_shared but return a new pointer to shared_ptr that must be deleted, i.e.
    instead of new shared_ptr<T>(new T(...)), new_shared<T>(...)
*/

#ifndef NEW_SHARED_GRAEHL_HPP
#define NEW_SHARED_GRAEHL_HPP
#pragma once

#include <memory>
#include <utility>

namespace graehl {

/// equivalent to new shared_ptr<T>(new T(args...)), new_shared<T>(args...) but with make_shared allocation benefit
/// perhaps C++2x will also allow the shared_ptr to be singly allocated contiguous to its implementation
template <class T, class... A>
std::shared_ptr<T> *new_shared(A&&... args) {
  return new std::shared_ptr<T>(std::make_shared<T>(std::forward<A>(args)...));
}

} // namespace graehl

#endif
