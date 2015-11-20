// Copyright 2014 Jonathan Graehl-http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

    avoid elimination of dead symbols while static linking.
*/

#ifndef FORCE_LINK_JG_2015_03_23_HPP
#define FORCE_LINK_JG_2015_03_23_HPP
#pragma once

#include <cstdlib>

namespace graehl {

static void force_link(void* p) {
  static volatile std::size_t forced_link;
  forced_link ^= (std::size_t)p;
}

template <class C>
static void force_link_class() {
  static C f;
  force_link(&f);
}

#define GRAEHL_FORCE_LINK_CLASS(x) graehl::force_link_class<x>();


}

#endif
