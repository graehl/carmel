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

    traits for configure library - see configure.hpp

     for a leaf_configurable class that you don't own:

     e.g.

     #include <boost/filesystem/path.hpp>

     LEAF_CONFIGURABLE_EXTERNAL(boost::filesystem::path)

     (note that the macro is invoked from the root namespace) - it simply
     ensures that configure::leaf_configurable<boost::filesystem::path> is a
     compile-time boolean integral constant = true


     for your leaf_configurable type T, you may wish to provide ADL-locatable
     overrides for:

     std::string to_string_impl(T const&);

     void string_to_impl(std::string const&,T &);

     // (both default to lexical_cast, except that boost::optional none can be "none" as well as ""

     std::string type_string(T const&) // argument ignored. defaults to ""

     std::string example_value(T const&) // argument ignored. for example configs

     void init_impl(T &t) // default destroys and reconstructs

     void assign_impl(T &to, T const& from) // default to=from

     note that std::string, boost::optional, enums, integral, and floating point
     are already supported in configure.hpp (so providing a
     LEAF_CONFIGURABLE_EXTERNAL specialization would result in multiple
     definition errors)
*/

#ifndef GRAEHL_SHARED__LEAF_CONFIGURABLE_HPP
#define GRAEHL_SHARED__LEAF_CONFIGURABLE_HPP
#pragma once

namespace configure {
template <class Val, class Enable = void>
struct leaf_configurable;
}

/** Use macro at global scope with fully qualified t. */
#define LEAF_CONFIGURABLE_EXTERNAL(t) \
  namespace configure {               \
  template <>                         \
  struct leaf_configurable<t, void> { \
    typedef bool value_type;          \
    enum { value = 1 };               \
  };                                  \
  }




#endif
