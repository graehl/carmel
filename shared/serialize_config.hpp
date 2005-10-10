#ifndef SERIALIZE_CONFIG_HPP
#define SERIALIZE_CONFIG_HPP

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/tracking.hpp>

#ifdef SERIALIZE_NO_CLASS_VERSION    
# include <boost/serialization/level.hpp>
# define SERIALIZE_CLASS_VERSION(myclass) BOOST_CLASS_IMPLEMENTATION(myclass, boost::serialization::object_serializable)
#else
# define SERIALIZE_CLASS_VERSION(myclass)
#endif

#define varnamed(var) BOOST_SERIALIZATION_NVP(var)


#ifdef SERIALIZE_TRACK_POINTERS_ALWAYS
# define SERIALIZE_UNSHARED(myclass) BOOST_CLASS_TRACKING(SyntaxRuleTree, boost::serialization::track_never)
#else
#define SERIALIZE_UNSHARED(myclass)
//BOOST_CLASS_TRACKING(SyntaxRuleTree, boost::serialization::track_always)
#endif

#define SERIALIZE_CLASS(myclass) SERIALIZE_CLASS_VERSION(myclass)
#define SERIALIZE_CLASS_UNSHARED(myclass) SERIALIZE_CLASS(myclass) SERIALIZE_UNSHARED(myclass)

#endif
