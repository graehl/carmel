#ifndef SERIALIZE_CONFIG_HPP
#define SERIALIZE_CONFIG_HPP

//NOTE: SERIALIZE_CLASS(class) must occur in global namespace scope!
//define ARCHIVE_DEFAULT_TEXT or _BINARY or _XML before including

#ifdef ARCHIVE_DEFAULT_XML
# include <boost/archive/xml_iarchive.hpp>
# include <boost/archive/xml_oarchive.hpp>
# define ARCHIVE_PREFIX_DEFAULT(x) boost::archive::xml ## x
#else
# ifdef ARCHIVE_DEFAULT_TEXT
#  include <boost/archive/text_iarchive.hpp>
#  include <boost/archive/text_oarchive.hpp>
#  define ARCHIVE_PREFIX_DEFAULT(x) boost::archive::text ## x
# else
#  ifndef ARCHIVE_PREFIX_DEFAULT
#   define ARCHIVE_DEFAULT_BINARY
#  endif
#  ifdef ARCHIVE_DEFAULT_BINARY
#   include <boost/archive/binary_iarchive.hpp>
#   include <boost/archive/binary_oarchive.hpp>
#   define ARCHIVE_PREFIX_DEFAULT(x) boost::archive::binary ## x
#  endif
# endif
#endif

#define OARCHIVE_DEFAULT ARCHIVE_PREFIX_DEFAULT(_oarchive)
#define IARCHIVE_DEFAULT ARCHIVE_PREFIX_DEFAULT(_iarchive)
typedef OARCHIVE_DEFAULT default_oarchive;
typedef IARCHIVE_DEFAULT default_iarchive;

#ifdef ARCHIVE_NO_HEADER
# define ARCHIVE_FLAG_NO_HEADER boost::archive::no_header
#else
# define ARCHIVE_FLAG_NO_HEADER 0
#endif

#if defined(ARCHIVE_NO_CODECVT) || defined(ARCHIVE_DEFAULT_BINARY)
# define ARCHIVE_FLAG_NO_CODECVT boost::archive::no_codecvt
#else
# define ARCHIVE_FLAG_NO_CODECVT 0
#endif

#ifndef ARCHIVE_FLAGS_DEFAULT_CUSTOM
#define ARCHIVE_FLAGS_DEFAULT_CUSTOM 0
#endif

static const int ARCHIVE_FLAGS_DEFAULT=ARCHIVE_FLAG_NO_HEADER | ARCHIVE_FLAG_NO_CODECVT | ARCHIVE_FLAGS_DEFAULT_CUSTOM;

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

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <stdexcept>
#include <fstream>

namespace graehl {

template <class Archive,class Data,class Ch,class Tr>
inline void save_to_stream(std::basic_ostream<Ch,Tr> &o,const Data &d, unsigned flags=ARCHIVE_FLAGS_DEFAULT) 
{
    Archive oa(o,flags);
    oa & d;
}

template <class Archive,class Data,class Ch,class Tr>
inline void load_from_stream(std::basic_istream<Ch,Tr> &i,Data &d, unsigned flags=ARCHIVE_FLAGS_DEFAULT) 
{
    Archive ia(i,flags);
    ia & d;
}

template <class Archive,class Data>
inline void save_to_file(const std::string &fname,const Data &d, unsigned flags=ARCHIVE_FLAGS_DEFAULT) 
{
    std::ofstream o(fname.c_str());
    if (!o)
        throw std::runtime_error(std::string("Couldn't create output serialization file ").append(fname));
    save_to_stream<Archive>(o,d,flags);
}

template <class Archive,class Data>
inline void load_from_file(const std::string &fname,Data &d, unsigned flags=ARCHIVE_FLAGS_DEFAULT) 
{
    std::ifstream i(fname.c_str());
    if (!i)
        throw std::runtime_error(std::string("Couldn't read input serialization file ").append(fname));
    load_from_stream<Archive>(i,d,flags);
}



}

#define load_from_file_default graehl::load_from_file<default_iarchive>
#define save_from_file_default graehl::save_to_file<default_oarchive>
#define load_from_stream_default graehl::load_from_stream<default_iarchive>
#define save_from_stream_default graehl::save_to_stream<default_oarchive>
//FIXME: not in namespace; how about some real wrappers?
#endif
