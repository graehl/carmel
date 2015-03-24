/** \file

    avoid elimination of dead symbols while static linking.
*/

#ifndef FORCE_LINK_JG_2015_03_23_HPP
#define FORCE_LINK_JG_2015_03_23_HPP
#pragma once

#include <cstdlib>

namespace graehl {

static void force_link(void *p) {
  static volatile std::size_t forced_link;
  forced_link ^= (std::size_t)p;
}

template <class C>
static void force_link_class() {
  static C f;
  force_link(&f);
}

#define GRAEHL_FORCE_LINK_CLASS(x) graehl::force_link_class< x >();

}

#endif
