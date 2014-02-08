/** \file

    given a filename, creates a (reference counted) input/output file/stream object, with "-" = STDIN/STDOUT, and ".gz" appropriately (de)compressed using gzstream.h - also, parameter parsing for Boost (command-line) Options library

    '-' means stdin/stdout, '-2' stderr, '-0' /dev/null

    all files are opened in binary mode (though cin/cout are the default)
*/

//
#ifndef GRAEHL__SHARED__FILEARGS_HPP
#define GRAEHL__SHARED__FILEARGS_HPP

#ifndef GRAEHL__VALIDATE_INFILE
// if 1, program options validation for shared_ptr<Stream>
# define GRAEHL__VALIDATE_INFILE 0
// else, just for file_arg<Stream>
#endif

#ifndef GRAEHL_USE_GZSTREAM
# define GRAEHL_USE_GZSTREAM 1
#endif
#ifndef GRAEHL_USE_LZ4
# define GRAEHL_USE_LZ4 0
#endif

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
# define BOOST_FILESYSTEM_NO_DEPRECATED
#endif
#ifndef BOOST_FILESYSTEM_VERSION
# define BOOST_FILESYSTEM_VERSION 3
#endif


/*
  Boost.Filesystem.v2 (afaik removed completely in
1.48 so you'll need to merge from the old release), it is better designed
in the sense that it has a templatized basic_path that allows you to store
utf-8 encoding internally (once you imbue the correct locale) and convert
to UTF-16 on demand.


#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
    ...
    std::locale global_loc = std::locale();
    std::locale loc(global_loc, new
boost::filesystem::detail::utf8_codecvt_facet);
    boost::filesystem::path::imbue(loc);

* If you only want one specific path to treat its narrow character
arguments and returns as UTF-8, do this:

    boost::filesystem::detail::utf8_codecvt_facet utf8;
    ...
    boost::filesystem::path p;
    ...
    p.assign(u8"...", utf8);  // many other path functions can take a
codecvt argument, too

 */
#include <algorithm>
#include <graehl/shared/large_streambuf.hpp>
#include <graehl/shared/null_deleter.hpp>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/null_ostream.hpp>
#include <graehl/shared/warn.hpp>
#include <graehl/shared/size_mega.hpp>
#include <graehl/shared/string_match.hpp>
#include <graehl/shared/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/config.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <memory>
#ifdef _MSC_VER
# include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#endif
#ifndef GRAEHL__DEFAULT_IN
#define GRAEHL__DEFAULT_IN std::cin
#endif

#ifndef GRAEHL__DEFAULT_OUT
#define GRAEHL__DEFAULT_OUT std::cout
#endif

#ifndef GRAEHL__DEFAULT_LOG
#define GRAEHL__DEFAULT_LOG std::cerr
#endif


namespace graehl {

namespace fs = boost::filesystem;

#if GRAEHL_USE_GZSTREAM
# if GRAEHL_USE_LZ4
#  define GRAEHL_GZ_USAGE "(.gz or .lz4 allowed)"
# else
#  define GRAEHL_GZ_USAGE "(.gz allowed)"
# endif
#else
# define GRAEHL_GZ_USAGE ""
#endif
inline std::string file_arg_usage(std::string prefix="filename: ")
{
  return prefix+"- for STDIN/STDOUT, -2 for STDERR, -0 for none"
#if GRAEHL_USE_GZSTREAM
", X.gz for gzipped"
#endif
#if GRAEHL_USE_LZ4
  ", X.lz4 for lz4"
#endif
  ;
}

inline std::string general_options_desc()
{
  return "Options ("+file_arg_usage()+"):";
}


template <class Stream>
struct stream_traits
{
};


template<>
struct stream_traits<std::ifstream>
{
  BOOST_STATIC_CONSTANT(bool,file_only=true);
  BOOST_STATIC_CONSTANT(bool,read=true);
  static char const* type_string() { return "input filename " GRAEHL_GZ_USAGE; }
};

template<>
struct stream_traits<std::ofstream>
{
  BOOST_STATIC_CONSTANT(bool,file_only=true);
  BOOST_STATIC_CONSTANT(bool,read=false);
  static char const* type_string() { return "output filename " GRAEHL_GZ_USAGE; }
};

template<>
struct stream_traits<std::ostream>
{
  BOOST_STATIC_CONSTANT(bool,file_only=false);
  BOOST_STATIC_CONSTANT(bool,read=false);
  static char const* type_string() { return "output filename or - for stdout, -0 for none, -2 for stderr " GRAEHL_GZ_USAGE; }
};

template<>
struct stream_traits<std::istream>
{
  BOOST_STATIC_CONSTANT(bool,file_only=false);
  BOOST_STATIC_CONSTANT(bool,read=true);
  static char const* type_string() { return "input filename or - for stdin " GRAEHL_GZ_USAGE; }
};

static const char gz_ext[]=".gz";
static const char lz4_ext[]=".lz4";

template <class S>
inline void set_null_file_arg(boost::shared_ptr<S> &p)
{
  p.reset();
}

inline void set_null_file_arg(boost::shared_ptr<std::ostream> &p)
{
  p.reset(new null_ostream()); //(&the_null_ostream,null_deleter());
}


// copyable because it's a shared ptr to an ostream, and holds shared ptr to a larger buffer used by it (for non-.gz file input/output) - make sure file is flushed before member buffer is destroyed, though!
template <class Stream>
struct file_arg
{
protected:
  typedef large_streambuf<> buf_type;
  typedef boost::shared_ptr<buf_type> buf_p_type; // because iostreams don't destroy their streambufs
  buf_p_type buf; // this will be destroyed *after* stream pointer (and constructed before).
  typedef stream_traits<Stream> traits;
  typedef boost::shared_ptr<Stream> pointer_type;
  pointer_type pointer; // this will get destroyed before buf (constructed after), which may be necessary since we don't require explicit flush.
  bool none;
public:
  std::size_t filesize() {
    return name.empty() ? 0 : (std::size_t)fs::file_size(fs::path(name));
  }

  typedef void leaf_configure; // configure.hpp
  friend std::string const& to_string_impl(file_arg const& fa)
  {
    return fa.name;
  }
  friend void string_to_impl(std::string const& name,file_arg & fa)
  {
    fa.set(name);
  }
  friend char const* type_string(file_arg const&) { return traits::type_string(); }

  string_consumer get_consumer() const
  {
    return none ?  string_consumer() : string_consumer(*this); // so you can check return as bool to avoid generating string
  }

  // string_consumer (Warn.hpp):
  void operator()(std::string const& s) const
  {
    //TODO: assert or traits so only for ostreams
    if (!none)
      *pointer<<s<<std::endl;
  }

  void set(Stream * newstream, std::string const& name) {
    none = false;
    this->pointer.reset(newstream);
    this->name = name;
  }

  template <class Ptr>
  void setPtr(Ptr const& ptr, std::string const& name) {
    set(boost::dynamic_pointer_cast<Stream>(ptr), name);
  }

  void set(pointer_type const& pstream, std::string const& name) {
    none = false;
    this->pointer = pstream;
    this->name = name;
  }

  operator pointer_type() const { return pointer; }
  void operator=(std::string const& name) { set(name); }

  bool operator==(Stream const& s) const { return get()==&s; }
  bool operator==(file_arg const& s) const { return get()==s.get(); }
  bool operator!=(Stream const& s) const { return get()!=&s; }
  bool operator!=(file_arg const& s) const { return get()!=s.get(); }
  Stream &operator *() const { return *pointer; }
  Stream *get() const
  {
    return pointer.get();
  }
  Stream *operator ->() const { return get(); }

  std::string name;

  char const* desc() const
  {
    return name.c_str();
  }

  std::string const& str() const
  {
    return name;
  }

  void close()
  {
    set_none();
  }

  typedef file_arg<Stream> self_type;

  void reset()
  {
    set_none();
  }

  file_arg() { set_none(); }
  explicit file_arg(fs::path const& path,bool null_allowed=ALLOW_NULL,bool large_buf=true)
  {  set(path.string(),null_allowed,large_buf); }
  explicit file_arg(char const* s,bool null_allowed=ALLOW_NULL,bool large_buf=true)
  {  set(s,null_allowed,large_buf); }
  explicit file_arg(std::string const& s,bool null_allowed=ALLOW_NULL,bool large_buf=true)
  {  set(s,null_allowed,large_buf); }
  void throw_fail(std::string const& filename,std::string const& msg="")
  {
    name=filename;
    throw std::runtime_error("FAILED("+filename+"): "+msg);
  }

  enum { delete_after=1,no_delete_after=0 };

  void clear() {
    pointer.reset(); // pointer to stream must go before its large buf goes
    buf.reset();
    name="";
  }
  void set(Stream &s,std::string const& filename="",bool destroy=no_delete_after,std::string const& fail_msg="invalid stream")
  {
    clear();
    if (!s)
      throw_fail(filename,fail_msg);
    if (destroy)
      pointer.reset(&s);
    else
      pointer.reset(&s,null_deleter());
    /*  The object pointed to is guaranteed to be deleted when the last shared_ptr pointing to it is destroyed or reset */
    name=filename;
  }

  template <class filestream>
  void set_checked(filestream &fs,std::string const& filename="",bool destroy=no_delete_after,std::string const& fail_msg="invalid stream")
  {
    try {
      set(dynamic_cast<Stream &>(fs),filename,destroy,fail_msg);
    } catch (std::bad_cast &) {
      throw_fail(filename," was not of the right stream type");
    }
  }
  BOOST_STATIC_CONSTANT(std::size_t,bufsize=256*1024);

  // warning: if you call with incompatible filestream type ... crash!
  template <class filestream>
  void set_new(std::string const& filename,std::string const& fail_msg="Couldn't open file")
  {
    std::auto_ptr<filestream> f(new filestream(filename.c_str(),std::ios::binary)); // will delete if we have an exception
    set_checked(*f,filename,delete_after,fail_msg);
    f.release(); // w/o delete
  }

  // set_new_buf is nearly a copy of set_new, except that apparently you can't open the file first then set buffer:
  /*
    Yes --- this was what Jens found. So, for the following code

    ifstream in;
    char buffer[1<<20];
    in.rdbuf()->pubsetbuf(buffer, 1<<20);
    in.open("bb");
    string str;
    while(getline(in, str)){   }

    the buffer will be set to be 1M. But if I change it to the following code,

    ifstream in("bb");
    char buffer[1<<20];
    in.rdbuf()->pubsetbuf(buffer, 1<<20);
    string str;
    while(getline(in, str)){   }

    the buffer will be back to 8k.
  */

  template <class filestream>
  void set_new_buf(std::string const& filename,std::string const& fail_msg="Couldn't open file",bool large_buf=true)
  {
    filestream *f=new filestream();
    std::auto_ptr<filestream> fa(f);
    set_checked(*f,filename,delete_after,fail_msg); // exception safety provided by f
    fa.release(); // now owned by smart ptr
    if (large_buf) give_large_buf();
    typedef stream_traits<filestream> traits;
    const bool read=traits::read;
    f->open(filename.c_str(),std::ios::binary | (read ? std::ios::in : (std::ios::out|std::ios::trunc)));
    if (!*f)
      throw_fail(filename,read?"Couldn't open for input.":"Couldn't open for output.");
  }

  void give_large_buf()
  {
    buf.reset(new buf_type(*pointer));
  }

  enum { ALLOW_NULL=1,NO_NULL=0 };

  void set_gzfile(std::string const&s,bool large_buf=true);

  void set_lz4(std::string const&s,bool large_buf=true) { throw_fail(".lz4 support unimplimented"); }

  // warning: if you specify the wrong values for read and file_only, you could assign the wrong type of pointer and crash!
  void set(std::string const& s,
           bool null_allowed=ALLOW_NULL, bool large_buf=true
    )
  {
    const bool read=stream_traits<Stream>::read;
    const bool file_only=stream_traits<Stream>::file_only;
    if (s.empty()) {
      throw_fail("<EMPTY FILENAME>","Can't open an empty filename.  Use \"-0\" if you really mean no file");
    } if (!file_only && s == "-") {
      // note that we don't attempt to supply a large buffer for either cin or cout,
      // since they're likely to already be in use by others
      if (read) {
        set_checked(GRAEHL__DEFAULT_IN,s);
      } else {
        set_checked(GRAEHL__DEFAULT_OUT,s);
      }
    } else if (!file_only && !read && s== "-2") {
      set_checked(GRAEHL__DEFAULT_LOG,s);
    } else if (null_allowed && s == "-0") {
      set_none();
      return;
    }
#if GRAEHL_USE_GZSTREAM
    else if (match_end(s.begin(),s.end(),gz_ext,gz_ext+sizeof(gz_ext)-1)) {
      set_gzfile(s);
    }
#endif
#if GRAEHL_USE_LZ4
    else if (match_end(s.begin(),s.end(),lz4_ext,lz4_ext+sizeof(lz4_ext)-1)) {
      set_lz4(s);
    }
#endif
    else {
      if (read)
        set_new_buf<std::ifstream>(s,"Couldn't open input file",large_buf);
      else
        set_new_buf<std::ofstream>(s,"Couldn't create output file",large_buf);
    }
    none=false;
  }

  explicit file_arg(Stream &s,std::string const& name) :
    pointer(&s,null_deleter()),name(name) {
    none = name == "-0";
  }

  template <class Stream2>
  file_arg(file_arg<Stream2> const& o) :
    pointer(o.pointer),name(o.name),buf(o.buf) {}

  bool is_stdin() const {
    return name == "-";
  }

  void set_none()
  { none=true;set_null_file_arg(pointer);buf.reset();name="-0"; }

  bool is_none() const
  { return none; }

  operator bool() const
  {
    return !is_none();
  }

  bool is_default_in() const {
    return pointer.get() == &GRAEHL__DEFAULT_IN; }

  bool is_default_out() const {
    return pointer.get() == &GRAEHL__DEFAULT_OUT; }

  bool is_default_log() const {
    return pointer.get() == &GRAEHL__DEFAULT_LOG; }

  bool valid() const
  {
    return !is_none() && stream();
  }
  friend
  bool valid(self_type const& f)
  {
    return f.valid();
  }

  Stream &stream() const
  {
    return *pointer;
  }

  Stream *get_nonnull() const
  {
    return is_none() ? 0 : pointer.get();
  }

  template<class O>
  void print(O &o) const { o << name;}
  template <class I>
  void read(I &i)
  {
    std::string name;
    i>>name;
    set(name);
  }
  TO_OSTREAM_PRINT
  FROM_ISTREAM_READ
};

typedef file_arg<std::istream> istream_arg;
typedef file_arg<std::ostream> ostream_arg;
typedef file_arg<std::ifstream> ifstream_arg;
typedef file_arg<std::ofstream> ofstream_arg;

inline
istream_arg stdin_arg()
{
  return istream_arg("-");
}

inline
ostream_arg stdout_arg()
{
  return ostream_arg("-");
}

inline
ostream_arg stderr_arg()
{
  return ostream_arg("-2");
}

inline std::string file_bytes(std::istream &i)
{
  typedef std::istreambuf_iterator<char> I;
  return std::string(I(i.rdbuf()),I());
}

template <class OBytesIter>
OBytesIter copy_bytes(std::istream &i,OBytesIter o)
{
  typedef std::istreambuf_iterator<char> I;
  return std::copy(I(i.rdbuf()),I(),o);
}


typedef boost::shared_ptr<std::istream> Infile;
typedef boost::shared_ptr<std::ostream> Outfile;
typedef boost::shared_ptr<std::ifstream> InDiskfile;
typedef boost::shared_ptr<std::ofstream> OutDiskfile;

static Infile default_in(&GRAEHL__DEFAULT_IN,null_deleter());
static Outfile default_log(&GRAEHL__DEFAULT_LOG,null_deleter());
static Outfile default_out(&GRAEHL__DEFAULT_OUT,null_deleter());
static Infile default_in_none;
static Outfile default_out_none;
static InDiskfile default_in_disk_none;
static OutDiskfile default_out_disk_none;

inline bool is_default_in(const Infile &i) {
  return i.get() == &GRAEHL__DEFAULT_IN; }

inline bool is_default_out(const Outfile &o) {
  return o.get() == &GRAEHL__DEFAULT_OUT; }

inline bool is_default_log(const Outfile &o) {
  return o.get() == &GRAEHL__DEFAULT_LOG; }

inline bool is_none(const Infile &i)
{ return i.get()==NULL; }

inline bool is_none(const Outfile &o)
{ return o.get()==NULL; }

struct tee_file
{
  tee_file() {
    reset_no_tee();
  }

  void reset_no_tee(std::ostream &o=std::cerr)
  {
    teestreamptr.reset();
    teebufptr.reset();
    log_stream=&o;
  }

  /// must call before you get any tee behavior (without, will go to default log = cerr)!
  void set(std::ostream &other_output)
  {
    if (file) {
      teebufptr.reset(
        new graehl::teebuf(file->rdbuf(),other_output.rdbuf()));
      teestreamptr.reset(
        log_stream=new std::ostream(teebufptr.get()));
    } else {
      log_stream=&other_output;
    }
  }
  ostream_arg file; // can set this directly, then call init.  if unset, then don't tee.

  std::ostream &stream() const
  { return *log_stream; }
  operator std::ostream &() const
  { return stream(); }

private:
  std::ostream *log_stream;
  boost::shared_ptr<graehl::teebuf> teebufptr;
  boost::shared_ptr<std::ostream> teestreamptr;
};

template <class Stream>
inline bool valid(boost::shared_ptr<Stream> const& pfile)
{
  return pfile && *pfile;
}

template <class C>
inline void throw_unless_valid(C const& pfile, std::string const& name="file")
{
  if (!valid(pfile))
    throw std::runtime_error(name+" not valid");
}

template <class C>
inline void throw_unless_valid_optional(C const& pfile, std::string const& name="file")
{
  if (pfile && !valid(pfile))
    throw std::runtime_error(name+" not valid");
}

inline Infile infile(const std::string &s)
{
  return istream_arg(s);
}

inline InDiskfile indiskfile(const std::string &s)
{
  return ifstream_arg(s);
}

inline fs::path full_path(const std::string &relative)
{
  return fs::system_complete(fs::path(relative)); //fs::path( relative, fs::native ) //V2
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

inline void native_filename_check()
{
//  fs::path::default_name_check(fs::native); // V2
}


// like V2 normalize
inline fs::path resolve(const fs::path& p)
{
  fs::path result;
  for(fs::path::iterator i=p.begin();i!=p.end();++i)
  {
    if(*i == "..") {
      // /a/b/.. is not necessarily /a if b is a symbolic link
      if(fs::is_symlink(result) )
        result /= *i;
      // /a/b/../.. is not /a/b/.. under most circumstances
      // We can end up with ..s in our result because of symbolic links
      else if(result.filename() == "..")
        result /= *i;
      // Otherwise i should be safe to resolve the parent
      else
        result = result.parent_path();
    } else if(*i != ".") {
      // Just cat other path entries
      result /= *i;
    }
  }
  return result;
}

// return the absolute filename that would result from "cp source dest" (and write to *dest_exists whether dest exists) - throws error if source is the same as dest
inline std::string output_name_like_cp(const std::string &source,const std::string &dest,bool *dest_exists=NULL)
{
  fs::path full_dest=full_path(dest);
  fs::path full_source=full_path(source);

  if (directory_exists(full_dest))
    full_dest /= full_source.filename();
  if (dest_exists && fs::exists(full_dest))
    *dest_exists=1;


  full_dest=resolve(full_dest);
  full_source=resolve(full_source);
//  full_dest.normalize();
//  full_source.normalize();
  //v2 only

  if (fs::equivalent(full_dest,full_source))
    throw std::runtime_error("Destination file is same as source!");
#ifdef _MSC_VER
  boost::filesystem::detail::utf8_codecvt_facet utf8;
//  boost::path::imbue(); // to get utf8 string
#endif
  return full_dest.string<std::string>(
#ifdef _MSC_VER
    utf8
#endif
    ); // v2: native_file_string() //TODO(windows): codecvt -> utf8, or is default fine? issue is that file opens on win32 expect UTF-16 wstrings.
}

inline Outfile outfile(const std::string &s)
{
  return ostream_arg(s);
}

inline OutDiskfile outdiskfile(const std::string &s)
{
  return ofstream_arg(s);
}


inline boost::shared_ptr<std::string> streambufContents(std::streambuf *sbuf) {
  typedef std::istreambuf_iterator<char> Iter;
  return boost::shared_ptr<std::string>(new std::string(Iter(sbuf), Iter()));
}

inline boost::shared_ptr<std::string> streamContents(std::istream &istream) {
  return streambufContents(istream.rdbuf());
}

template <class Istream>
inline boost::shared_ptr<std::string> fileContents(file_arg<Istream> const& file) {
  std::size_t sz = file.filesize();
  if (sz) {
    boost::shared_ptr<std::string> r(new std::string(sz, '\0'));
    file->rdbuf()->sgetn(&(*r)[0], sz);
  } else
    return streamContents(*file);
}


} //graehl

#if (!defined(GRAEHL__NO_GZSTREAM_MAIN) && defined(GRAEHL__SINGLE_MAIN)) || defined(GRAEHL__GZSTREAM_MAIN)
# include <graehl/shared/fileargs.cpp>
#endif

namespace boost { namespace program_options {

#if GRAEHL__VALIDATE_INFILE
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

#else

# ifdef _MSC_VER
inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     graehl::file_arg<std::istream>* target_type, int)
{
  v = boost::any(graehl::file_arg<std::istream>(graehl::get_single_arg(v, values)));
}

inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     graehl::file_arg<std::ostream>* target_type, int)
{
  v = boost::any(graehl::file_arg<std::ostream>(graehl::get_single_arg(v, values)));
}

inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     graehl::file_arg<std::ifstream>* target_type, int)
{
  v = boost::any(graehl::file_arg<std::ifstream>(graehl::get_single_arg(v, values)));
}


inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     graehl::file_arg<std::ofstream>* target_type, int)
{
  v = boost::any(graehl::file_arg<std::ofstream>(graehl::get_single_arg(v, values)));
}

# else
template <class Stream>
inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     graehl::file_arg<Stream>* target_type, int)
{
  v = boost::any(graehl::file_arg<Stream>(graehl::get_single_arg(v, values)));
}
# endif
#endif

}} // boost::program_options

#endif
