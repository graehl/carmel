// given a filename, creates a (reference counted) input/output file/stream object, with "-" = STDIN/STDOUT, and ".gz" appropriately (de)compressed using gzstream.h - also, parameter parsing for Boost (command-line) Options library 
#ifndef FILEARGS_HPP
#define FILEARGS_HPP
//TODO: support linking to gzstream

#include <string>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
//#include "fileheader.hpp"
#include "funcs.hpp"
#ifndef GZSTREAM_NAMESPACE
# define USE_GZSTREAM_NAMESPACE
//ns_gzstream
#endif
#include "gzstream.h"
#if defined(SINGLE_MAIN) || defined(SINGLE_MAIN_GZSTREAM)
# include "gzstream.C"
#endif
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"

static const char gz_ext[]=".gz";

typedef boost::shared_ptr<std::istream> Infile;
typedef boost::shared_ptr<std::ostream> Outfile;
typedef boost::shared_ptr<std::ifstream> InDiskfile;
typedef boost::shared_ptr<std::ofstream> OutDiskfile;

struct null_deleter {
    void operator()(void*) {}
};

#ifndef DEFAULT_IN_P
#define DEFAULT_IN_P &std::cin
#endif

#ifndef DEFAULT_OUT_P
#define DEFAULT_OUT_P &std::cout
#endif

#ifndef DEFAULT_LOG_P
#define DEFAULT_LOG_P &std::cerr
#endif

static Infile default_in(DEFAULT_IN_P,null_deleter());
static Outfile default_log(DEFAULT_LOG_P,null_deleter());
static Outfile default_out(DEFAULT_OUT_P,null_deleter());
static Infile default_in_none;
static Outfile default_out_none;
static InDiskfile default_in_disk_none;
static OutDiskfile default_out_disk_none;

inline bool is_default_in(const Infile &i) {
    return i.get() == DEFAULT_IN_P;
}
inline bool is_default_out(const Outfile &o) {
    return o.get() == DEFAULT_OUT_P;
}
inline bool is_default_log(const Outfile &o) {
    return o.get() == DEFAULT_LOG_P;
}

inline Infile infile(const std::string &s) 
{
        if (s == "-") {
        boost::shared_ptr<std::istream> r(DEFAULT_IN_P, null_deleter());
        return (r);
    } else if (s == "-0") {
        return default_in_none;
    } else if (match_end(s.begin(),s.end(),gz_ext,gz_ext+sizeof(gz_ext)-1)) {
        typedef USE_GZSTREAM_NAMESPACE::igzstream igzs;
        boost::shared_ptr<igzs> r(new igzs(s.c_str()));
        if (!*r) 
            throw std::runtime_error(std::string("Could not open compressed input file ").append(s));
        boost::shared_ptr<std::istream> r2(r);
        return (r2);
    } else {   
        boost::shared_ptr<std::ifstream> r(new std::ifstream(s.c_str()));
        if (!*r)
            throw std::runtime_error(std::string("Could not open input file ").append(s));
        boost::shared_ptr<std::istream> r2(r);
        return (r2);
    }

}

inline InDiskfile indiskfile(const std::string &s)
{
    if (s == "-0") {
        return default_in_disk_none;
    } else {
        boost::shared_ptr<std::ifstream> r(new std::ifstream(s.c_str()));
        if (!*r) {
            throw std::runtime_error(std::string("Could not open input file ").append(s));
        }
        return (r);
    }
}

namespace fs = boost::filesystem;

inline fs::path full_path(const std::string &relative) 
{
    return fs::system_complete( fs::path( relative, fs::native ) );
}

inline bool directory_exists(const fs::path &possible_dir)
{
    return fs::exists(possible_dir) && fs::is_directory(possible_dir);
}

// works on .gz files!
inline size_t count_newlines(const std::string &filename) 
{
    Infile i=infile(filename);
    char c;
    size_t n_newlines=0;
    while (i->get(c)) {
        if (c=='\n')
            ++n_newlines;
    }
    return n_newlines;    
}


// return the absolute filename that would result from "cp source dest" (and write to *dest_exists whether dest exists) - throws error if source is the same as dest
inline std::string output_name_like_cp(const std::string &source,const std::string &dest,bool *dest_exists=NULL) 
{
    fs::path full_dest=full_path(dest);
    fs::path full_source=full_path(source);
    
    if (directory_exists(full_dest))
        full_dest /= full_source.leaf();
    if (dest_exists && fs::exists(full_dest))
        *dest_exists=1;
        

    full_dest.normalize();
    full_source.normalize();
    
    if (fs::equivalent(full_dest,full_source))
        throw std::runtime_error("Destination file is same as source!");
    
    return full_dest.native_file_string();
}

inline Outfile outfile(const std::string &s) 
{
    if (s == "-") {
        boost::shared_ptr<std::ostream> w(DEFAULT_OUT_P, null_deleter());
        return (w);
    } else if ( s== "-2") {
        boost::shared_ptr<std::ostream> w(DEFAULT_LOG_P, null_deleter());
        return (w);
    } else if (s == "-0") {
        return default_out_none;
    } else if (match_end(s.begin(),s.end(),gz_ext,gz_ext+sizeof(gz_ext)-1)) {
        typedef USE_GZSTREAM_NAMESPACE::ogzstream ogzs;
        boost::shared_ptr<ogzs> w(new ogzs(s.c_str()));
        if (!*w) 
            throw std::runtime_error(std::string("Could not create compressed output file ").append(s));
        boost::shared_ptr<std::ostream> w2(w);
        return (w2);
    } else {
        boost::shared_ptr<std::ofstream> w(new std::ofstream(s.c_str()));
        if (!*w) {
            throw std::runtime_error(std::string("Could not create output file ").append(s));
        }
        boost::shared_ptr<std::ostream> w2(w);
        return (w2);
    }
}

inline OutDiskfile outdiskfile(const std::string &s)
{
    if (s == "-0") {
        return default_out_disk_none;
    } else {
        boost::shared_ptr<std::ofstream> r(new std::ofstream(s.c_str()));
        if (!*r) {
            throw std::runtime_error(std::string("Could not create output file ").append(s));
        }
        return (r);
    }
}


//using namespace boost;
//using namespace boost::program_options;
namespace po=boost::program_options;
using boost::shared_ptr;

//using namespace std;

namespace boost {    namespace program_options {
inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              size_t* target_type, int)
{
    typedef size_t value_type;
    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    std::istringstream i(po::validators::get_single_string(values));
    v=boost::any(parse_size<value_type>(i));
    char c;
    if (i >> c)
        throw std::runtime_error(std::string("Succesfully read a nonnegative size, but read extra characters after."));
    return;
}

/* Overload the 'validate' function for shared_ptr<std::istream>. We use shared ptr
   to properly kill the stream when it's no longer used.
*/
inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::istream>* target_type, int)
{
    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);
    
    v=boost::any(infile(s));
}

inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ostream>* target_type, int)
{
    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);
    
    v=boost::any(outfile(s));
}

inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ofstream>* target_type, int)
{
    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);

    v=boost::any(outdiskfile(s));
}

inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ifstream>* target_type, int)
{
    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);

    v=boost::any(indiskfile(s));
}


}}


#endif
