#ifndef LEAF_CONFIGURABLE_JG2012919_HPP
#define LEAF_CONFIGURABLE_JG2012919_HPP

/** \file

     for a leaf_configurable class that you don't own:

     e.g.

     #include <boost/filesystem/path.hpp>

     LEAF_CONFIGURABLE_EXTERNAL(boost::filesystem::path)

     (note that the macro is invoked from the root namespace) - it simply
     ensures that configure::leaf_configurable<boost::filesystem::path> is a
     compile-time boolean integral constant = true


     for your leaf_configurable type T, you may wish to provide ADL-locatable
     overrides for:

     inline std::string to_string_impl(T const&);

     inline void string_to_impl(std::string const&,T &);

     // (both default to boost::lexical_cast, except that boost::optional none can be "none" as well as ""

     inline std::string type_string(T const&) // argument ignored. defaults to ""

     inline std::string example_value(T const&) // argument ignored. for example configs

     void init_impl(T &t) // default destroys and reconstructs

     void assign_impl(T &to,T const& from) // default to=from

     note that std::string, boost::optional, enums, integral, and floating point
     are already supported in configure.hpp (so providing a
     LEAF_CONFIGURABLE_EXTERNAL specialization would result in multiple
     definition errors)
*/

namespace configure {
template <class Val,class Enable=void>
struct leaf_configurable;
}

/** Use macro at global scope with fully qualified t. */
#define LEAF_CONFIGURABLE_EXTERNAL(t) namespace configure { \
  template<> struct leaf_configurable<t,void> { typedef bool value_type; enum {value=1}; }; }


#endif // LEAF_CONFIGURABLE_JG2012919_HPP
