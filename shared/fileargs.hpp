// given a filename, creates a (reference counted) input/output file/stream object, with "-" = STDIN/STDOUT, and ".gz" appropriately (de)compressed using gzstream.h - also, parameter parsing for Boost (command-line) Options library 
#ifndef GRAEHL__SHARED__FILEARGS_HPP
#define GRAEHL__SHARED__FILEARGS_HPP
//TODO: support linking to gzstream

#include <graehl/shared/gzstream.hpp>
#include <graehl/shared/size_mega.hpp>
#include <graehl/shared/string_match.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace graehl {

static const char gz_ext[]=".gz";

template <class Stream>
struct file_arg : public boost::shared_ptr<Stream>
{
    typedef boost::shared_ptr<Stream> parent_type;
    file_arg() {}
    file_arg(file_arg<Stream> const& o) : parent_type((parent_type const&)o) {}
    template <class C1>
    file_arg(C1 const &c1) : parent_type(c1) {}
    parent_type &parent() { return *this; }
    parent_type const& parent() const { return *this; }
    bool valid() const
    {
        return parent() && *parent();
    }
    Stream &stream() 
    {
        return *parent();
    }
    operator Stream &() 
    {
        return stream();
    }
};
    

typedef boost::shared_ptr<std::istream> Infile;
typedef boost::shared_ptr<std::ostream> Outfile;
typedef boost::shared_ptr<std::ifstream> InDiskfile;
typedef boost::shared_ptr<std::ofstream> OutDiskfile;

struct null_deleter {
    void operator()(void*) const {}
};

#ifndef GRAEHL__DEFAULT_IN_P
#define GRAEHL__DEFAULT_IN_P &std::cin
#endif

#ifndef GRAEHL__DEFAULT_OUT_P
#define GRAEHL__DEFAULT_OUT_P &std::cout
#endif

#ifndef GRAEHL__DEFAULT_LOG_P
#define GRAEHL__DEFAULT_LOG_P &std::cerr
#endif

static Infile default_in(GRAEHL__DEFAULT_IN_P,null_deleter());
static Outfile default_log(GRAEHL__DEFAULT_LOG_P,null_deleter());
static Outfile default_out(GRAEHL__DEFAULT_OUT_P,null_deleter());
static Infile default_in_none;
static Outfile default_out_none;
static InDiskfile default_in_disk_none;
static OutDiskfile default_out_disk_none;

inline bool is_default_in(const Infile &i) {
    return i.get() == GRAEHL__DEFAULT_IN_P;
}
inline bool is_default_out(const Outfile &o) {
    return o.get() == GRAEHL__DEFAULT_OUT_P;
}
inline bool is_default_log(const Outfile &o) {
    return o.get() == GRAEHL__DEFAULT_LOG_P;
}

template <class Stream>
inline bool valid(boost::shared_ptr<Stream> const&pfile)
{
    return pfile && *pfile;
}

template <class C>
inline bool throw_unless_valid(C const&pfile, std::string const& name="file") 
{
    if (!valid(pfile))
        throw std::runtime_error(name+" not valid");
}



inline Infile infile(const std::string &s) 
{
        if (s == "-") {
        boost::shared_ptr<std::istream> r(GRAEHL__DEFAULT_IN_P, null_deleter());
        return (r);
    } else if (s == "-0") {
        return default_in_none;
    } else if (match_end(s.begin(),s.end(),gz_ext,gz_ext+sizeof(gz_ext)-1)) {
        typedef igzstream igzs;
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
        boost::shared_ptr<std::ostream> w(GRAEHL__DEFAULT_OUT_P, null_deleter());
        return (w);
    } else if ( s== "-2") {
        boost::shared_ptr<std::ostream> w(GRAEHL__DEFAULT_LOG_P, null_deleter());
        return (w);
    } else if (s == "-0") {
        return default_out_none;
    } else if (match_end(s.begin(),s.end(),gz_ext,gz_ext+sizeof(gz_ext)-1)) {
        typedef ogzstream ogzs;
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

inline std::string const& get_single_arg(boost::any& v,std::vector<std::string> const& values) 
{
    boost::program_options::validators::check_first_occurrence(v);
    return boost::program_options::validators::get_single_string(values);
}

} //graehl

namespace boost {    namespace program_options {
inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              size_t* target_type, int)
{
    typedef size_t value_type;
    using namespace graehl;

    std::istringstream i(boost::program_options::validators::get_single_string(values));
    v=boost::any(graehl::parse_size<value_type>(i));
    char c;
    if (i >> c)
        throw std::runtime_error(std::string("Succesfully read a nonnegative size, but read extra characters after."));
    return;
}

/* Overload the 'validate' function for boost::shared_ptr<std::istream>. We use shared ptr
   to properly kill the stream when it's no longer used.
*/
inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::istream>* target_type, int)
{
    v=boost::any(graehl::infile(graehl::get_single_arg(v,values)));
}

inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ostream>* target_type, int)
{
    v=boost::any(graehl::outfile(graehl::get_single_arg(v,values)));
}

inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ofstream>* target_type, int)
{
    v=boost::any(graehl::outdiskfile(graehl::get_single_arg(v,values)));
}

inline void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ifstream>* target_type, int)
{
    v=boost::any(graehl::indiskfile(graehl::get_single_arg(v,values)));
}

}} // boost::program_options


#endif
