#ifndef SERIALIZE_CONFIG_HPP
#define SERIALIZE_CONFIG_HPP

//NOTE: SERIALIZE_CLASS(class) must occur in global namespace scope!
//define ARCHIVE_DEFAULT_TEXT or _BINARY or _XML before including

#ifdef ARCHIVE_DEFAULT_XML
# include <boost/archive/xml_iarchive.hpp>
# include <boost/archive/xml_oarchive.hpp>
# define ARCHIVE_PREFIX_DEFAULT(x) boost::archive::xml ## x
#endif
#ifdef ARCHIVE_DEFAULT_TEXT
# include <boost/archive/text_iarchive.hpp>
# include <boost/archive/text_oarchive.hpp>
# define ARCHIVE_PREFIX_DEFAULT(x) boost::archive::text ## x
#endif
#if !defined(ARCHIVE_PREFIX_DEFAULT) || defined(ARCHIVE_DEFAULT_BINARY)
# include <boost/archive/binary_iarchive.hpp>
# include <boost/archive/binary_oarchive.hpp>
# define ARCHIVE_PREFIX_DEFAULT(x) boost::archive::binary ## x
#endif

#define OARCHIVE_DEFAULT ARCHIVE_PREFIX_DEFAULT(_oarchive)
#define IARCHIVE_DEFAULT ARCHIVE_PREFIX_DEFAULT(_iarchive)
typedef OARCHIVE_DEFAULT default_oarchive;
typedef IARCHIVE_DEFAULT default_iarchive;

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/level.hpp>

namespace serial = boost::serialization;

#define SERIALIZE_NOVERSION(myclass) BOOST_CLASS_IMPLEMENTATION(myclass, serial::object_serializable)

#ifdef SERIALIZE_NO_CLASS_VERSION
# define SERIALIZE_DEFAULT_VERSION(myclass) SERIALIZE_NOVERSION(myclass)
#else
# define SERIALIZE_DEFAULT_VERSION(myclass)
#endif

#ifdef SERIALIZE_NO_XML
# define varnamed(var) var
#else 
# define varnamed(var) BOOST_SERIALIZATION_NVP(var)
#endif 


#ifdef SERIALIZE_TRACK_POINTERS_ALWAYS
# define SERIALIZE_MAKE_UNSHARED(myclass) BOOST_CLASS_TRACKING(myclass, serial::track_never)
#else
# define SERIALIZE_MAKE_UNSHARED(myclass)
//BOOST_CLASS_TRACKING(SyntaxRuleTree, serial::track_always)
#endif

#define SERIALIZE_CLASS(myclass) SERIALIZE_DEFAULT_VERSION(myclass)
#define SERIALIZE_UNSHARED(myclass) SERIALIZE_CLASS(myclass) SERIALIZE_MAKE_UNSHARED(myclass)

#endif
