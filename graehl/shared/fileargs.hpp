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

    given a filename, creates a (reference counted) input/output file/stream object, with "-" = STDIN/STDOUT,
   and ".gz" appropriately (de)compressed using gzstream.h - also, parameter parsing for Boost (command-line)
   Options library

    '-' means stdin/stdout, '-2' stderr, '-0' /dev/null

    all files are opened in binary mode (though cin/cout are the default)
*/

//
#ifndef GRAEHL__SHARED__FILEARGS_HPP
#define GRAEHL__SHARED__FILEARGS_HPP
#pragma once

#ifndef GRAEHL_FILEARGS_DEFAULT_LARGE_BUF
#define GRAEHL_FILEARGS_DEFAULT_LARGE_BUF 1
#endif
#ifndef GRAEHL_FILEARGS_LARGE_BUF_BYTES
#define GRAEHL_FILEARGS_LARGE_BUF_BYTES (64 * 1024)
#endif
#ifndef GRAEHL__VALIDATE_INFILE
// if 1, program options validation for shared_ptr<Stream>
#define GRAEHL__VALIDATE_INFILE 0
// else, just for file_arg<Stream>
#endif

#ifndef GRAEHL_USE_GZSTREAM
#define GRAEHL_USE_GZSTREAM 1
#endif
#ifndef GRAEHL_USE_LZ4
#define GRAEHL_USE_LZ4 0
#endif

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_FILESYSTEM_NO_DEPRECATED
#endif
#ifndef BOOST_FILESYSTEM_VERSION
#define BOOST_FILESYSTEM_VERSION 3
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
#include <boost/config.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <graehl/shared/large_streambuf.hpp>
#include <graehl/shared/null_deleter.hpp>
#include <graehl/shared/null_ostream.hpp>
#include <graehl/shared/program_options.hpp>
#include <graehl/shared/shared_ptr.hpp>
#include <graehl/shared/size_mega.hpp>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/string_match.hpp>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/warn.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#ifdef _MSC_VER
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
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

namespace {
static std::string const stdin_filename("-");
static std::string const stdout_filename("-");
static std::string const stderr_filename("-2");
static std::string const null_filename("-0");
const char gz_ext[] = ".gz";
const char lz4_ext[] = ".lz4";
const char bz2_ext[] = ".bz2";
}

inline bool special_output_filename(std::string const& s) {
  std::size_t sz = s.size();
  return sz == 1 ? s[0] == '-' : sz == 2 ? s[0] == '-' && (s[1] == '0' || s[1] == '2') : false;
}

inline bool special_input_filename(std::string const& s) {
  std::size_t sz = s.size();
  return (sz == 1 && s[0] == '-') || (sz == 2 && s[0] == '-' && s[1] == '0');
}

inline bool gz_filename(std::string const& s) {
  return match_end(s.begin(), s.end(), gz_ext, gz_ext + sizeof(gz_ext) - 1);
}

inline bool lz4_filename(std::string const& s) {
  return match_end(s.begin(), s.end(), lz4_ext, lz4_ext + sizeof(lz4_ext) - 1);
}

inline bool bz2_filename(std::string const& s) {
  return match_end(s.begin(), s.end(), bz2_ext, bz2_ext + sizeof(bz2_ext) - 1);
}

namespace fs = boost::filesystem;

#if GRAEHL_USE_GZSTREAM
#if GRAEHL_USE_LZ4
#define GRAEHL_GZ_USAGE "(.gz or .lz4 allowed)"
#else
#define GRAEHL_GZ_USAGE "(.gz allowed)"
#endif
#else
#define GRAEHL_GZ_USAGE ""
#endif
inline std::string file_arg_usage(std::string const& prefix = "filename: ") {
  return prefix+"- for STDIN/STDOUT, -2 for STDERR, -0 for none"
#if GRAEHL_USE_GZSTREAM
", X.gz for gzipped"
#endif
#if GRAEHL_USE_LZ4
  ", X.lz4 for lz4"
#endif
  ;
}

inline std::string general_options_desc() {
  return "Options (" + file_arg_usage() + "):";
}


template <class Stream>
struct stream_traits {
  static bool is_default(std::string const& s) { return s == stdin_filename; }
  BOOST_STATIC_CONSTANT(bool, file_only = false);
  BOOST_STATIC_CONSTANT(bool, read = true);
  template <class Filearg>
  static void call_set_default(Filearg& x, std::string const& s) {
    x.set_checked(GRAEHL__DEFAULT_IN, s);
  }
  template <class Filearg>
  static void call_set_file(Filearg& x, std::string const& s, bool large_buf) {
    x.template set_new_buf<std::ifstream>(s, "Couldn't open input file", large_buf);
  }
  static char const* type_string() { return "input filename " GRAEHL_GZ_USAGE; }
};

template <>
struct stream_traits<std::ifstream> : stream_traits<std::istream> {
  static bool is_default(std::string const& s) { return false; }
  BOOST_STATIC_CONSTANT(bool, file_only = true);
  static char const* type_string() { return "input filename"; }
};

template <>
struct stream_traits<std::ofstream> {
  static bool is_default(std::string const& s) { return false; }
  BOOST_STATIC_CONSTANT(bool, file_only = true);
  BOOST_STATIC_CONSTANT(bool, read = false);
  static char const* type_string() { return "output filename"; }
  template <class Filearg>
  static void call_set_default(Filearg& x, std::string const& s) {
    x.set_checked(s == stderr_filename ? GRAEHL__DEFAULT_LOG : GRAEHL__DEFAULT_OUT, s);
  }
  template <class Filearg>
  static void call_set_file(Filearg& x, std::string const& s, bool large_buf) {
    x.template set_new_buf<std::ofstream>(s, "Couldn't open output file", large_buf);
  }
};

template <>
struct stream_traits<std::ostream> : stream_traits<std::ofstream> {
  static bool is_default(std::string const& s) { return s == stdin_filename || s == stderr_filename; }
  BOOST_STATIC_CONSTANT(bool, file_only = false);
  static char const* type_string() { return "output filename " GRAEHL_GZ_USAGE; }
};

template <class S>
inline void set_null_file_arg(shared_ptr<S>& p) {
  p.reset();
}

inline void set_null_file_arg(shared_ptr<std::ostream>& p) {
  p.reset(new null_ostream());  //(&the_null_ostream, null_deleter());
}


// copyable because it's a shared ptr to an ostream, and holds shared ptr to a larger buffer used by it (for
// non-.gz file input/output) - make sure file is flushed before member buffer is destroyed, though!
template <class Stream>
struct file_arg {
 protected:
  typedef large_streambuf<GRAEHL_FILEARGS_LARGE_BUF_BYTES> buf_type;
  typedef stream_traits<Stream> traits;
  // hold large streambuf separately since iostreams don't destroy their streambufs
  typedef shared_ptr<buf_type> buf_p_type;
  typedef shared_ptr<Stream> pointer_type;
  buf_p_type buf;  // this will be destroyed *after* stream pointer (and constructed before).
  pointer_type pointer;  // this will get destroyed before buf (constructed after), which may be necessary
  // since we don't require explicit flush.
  bool none;

 public:
  std::size_t filesize() { return name.empty() ? 0 : (std::size_t)fs::file_size(fs::path(name)); }

  typedef void leaf_configure;  // configure.hpp
  friend std::string const& to_string_impl(file_arg const& fa) { return fa.name; }
  friend void string_to_impl(std::string const& name, file_arg& fa) { fa.set(name); }
  friend char const* type_string(file_arg const&) { return traits::type_string(); }

  string_consumer get_consumer() const {
    return none ? string_consumer()
                : string_consumer(*this);  // so you can check return as bool to avoid generating string
  }

  // string_consumer (Warn.hpp):
  void operator()(std::string const& s) const {
    // TODO: assert or traits so only for ostreams
    if (!none) *pointer << s << '\n';
  }

  void set(Stream* newstream, std::string const& name) {
    none = false;
    this->pointer.reset(newstream);
    this->name = name;
  }

  template <class Val>
  void setPtr(shared_ptr<Val> const& ptr, std::string const& name) {
    set(dynamic_pointer_cast<Stream>(ptr), name);
  }

  template <class Val>
  void setPtr(Val* ptr, std::string const& name) {
    set(dynamic_pointer_cast<Stream>(ptr), name);
  }

  void set(pointer_type const& pstream, std::string const& name) {
    none = false;
    this->pointer = pstream;
    this->name = name;
  }


  operator pointer_type() const { return pointer; }
  void operator=(std::string const& name) { set(name); }
  bool operator==(Stream const& s) const { return get() == &s; }
  bool operator==(file_arg const& s) const { return get() == s.get(); }
  bool operator!=(Stream const& s) const { return get() != &s; }
  bool operator!=(file_arg const& s) const { return get() != s.get(); }
  Stream& operator*() const { return *pointer; }
  Stream* get() const { return pointer.get(); }
  Stream* operator->() const { return get(); }

  std::string name;

  char const* desc() const { return name.c_str(); }

  std::string const& str() const { return name; }

  void flush() {
    if (!none) pointer->flush();
  }

  void close() { set_none(); }

  typedef file_arg<Stream> self_type;

  void reset() { set_none(); }

#if GRAEHL_FILEARGS_DEFAULT_LARGE_BUF
  enum { kDefaultLargeBuf = true };
#else
  enum { kDefaultLargeBuf = false };
#endif

  file_arg() { set_none(); }
  explicit file_arg(fs::path const& path, bool null_allowed = ALLOW_NULL, bool large_buf = kDefaultLargeBuf) {
    set(path.string(), null_allowed, large_buf);
  }
  explicit file_arg(char const* s, bool null_allowed = ALLOW_NULL, bool large_buf = kDefaultLargeBuf) {
    set(s, null_allowed, large_buf);
  }
  explicit file_arg(std::string const& s, bool null_allowed = ALLOW_NULL, bool large_buf = kDefaultLargeBuf) {
    set(s, null_allowed, large_buf);
  }
  explicit file_arg(Stream& s, std::string const& name) : pointer(&s, null_deleter()), name(name) {
    none = (name == null_filename);
  }

  void swapWith(file_arg& o) {
    using namespace std;
    swap(buf, o.buf);
    swap(pointer, o.pointer);
    bool t = none;
    none = o.none;
    o.none = t;
    swap(name, o.name);
  }

  void operator=(file_arg const& o) {
    buf = o.buf;
    pointer = o.pointer;
    none = o.none;
    name = o.name;
  }
  file_arg(file_arg const& o) : buf(o.buf), pointer(o.pointer), none(o.none), name(o.name) {}
#if GRAEHL_CPP11
  file_arg(file_arg&& o)
      : buf(std::move(o.buf)), pointer(std::move(o.pointer)), none(o.none), name(std::move(o.name)) {}
  file_arg& operator=(file_arg&& o) {
    assert(&o != this);
    buf = std::move(o.buf);
    pointer = std::move(o.pointer);
    none = o.none;
    name = std::move(o.name);
    return *this;
  }
#endif

  void throw_fail(std::string const& filename, std::string const& msg = "") {
    name = filename;
    throw std::runtime_error("FAILED(" + filename + "): " + msg);
  }

  enum { delete_after = 1, no_delete_after = 0 };

  bool is_file() const { return !none && !special_output_filename(name); }

 protected:
  void clear() {
    none = true;
    pointer.reset();  // pointer to stream must go before its large buf goes
    buf.reset();
    name = "";
  }

 public:
  void set(Stream& s, std::string const& filename = "", bool destroy = no_delete_after,
           std::string const& fail_msg = "invalid stream") {
    clear();
    if (s.bad()) throw_fail(filename, fail_msg);
    if (destroy)
      pointer.reset(&s);
    else
      pointer.reset(&s, null_deleter());
    /*  The object pointed to is guaranteed to be deleted when the last shared_ptr pointing to it is destroyed
     * or reset */
    name = filename;
  }

  template <class filestream>
  void set_checked(filestream& fs, std::string const& filename = "", bool destroy = no_delete_after,
                   std::string const& fail_msg = "invalid stream") {
    try {
      set(dynamic_cast<Stream&>(fs), filename, destroy, fail_msg);
    } catch (std::bad_cast&) {
      throw_fail(filename, " was not of the right stream type");
    }
  }
  BOOST_STATIC_CONSTANT(std::size_t, bufsize = 256 * 1024);

  // warning: if you call with incompatible filestream type ... crash!
  template <class filestream>
  void set_new(std::string const& filename, std::string const& fail_msg = "Couldn't open file") {
#if !GRAEHL_CPP11
    std::auto_ptr
#else
    std::unique_ptr
#endif
        <filestream>
            f(new filestream(filename.c_str(), std::ios::binary));  // will delete if we have an exception
    set_checked(*f, filename, delete_after, fail_msg);
    f.release();  // w/o delete
  }

  // set_new_buf is nearly a copy of set_new, except that apparently you can't open the file first then set
  // buffer:
  /*
    Yes --- this was what Jens found. So, for the following code

    ifstream in;
    char buffer[1 << 20];
    in.rdbuf()->pubsetbuf(buffer, 1 << 20);
    in.open("bb");
    string str;
    while (getline(in, str)) {   }

    the buffer will be set to be 1M. But if I change it to the following code,

    ifstream in("bb");
    char buffer[1 << 20];
    in.rdbuf()->pubsetbuf(buffer, 1 << 20);
    string str;
    while (getline(in, str)) {   }

    the buffer will be back to 8k.
  */

  template <class filestream>
  void set_new_buf(std::string const& filename, std::string const& fail_msg = "Couldn't open file",
                   bool large_buf = kDefaultLargeBuf) {
    filestream* f = new filestream();
#if !GRAEHL_CPP11
    std::auto_ptr
#else
    std::unique_ptr
#endif
        <filestream>
            fa(f);
    set_checked(*f, filename, delete_after, fail_msg);  // exception safety provided by f
    fa.release();  // now owned by smart ptr
    buf.reset();
    if (large_buf) give_large_buf();
    typedef stream_traits<filestream> traits;
    const bool read = traits::read;
    f->open(
#if GRAEHL_CPP11
        filename,
#else
        filename.c_str(),
#endif
        std::ios::binary | (read ? std::ios::in : (std::ios::out | std::ios::trunc)));
    if (!*f) throw_fail(filename, read ? "Couldn't open for input." : "Couldn't open for output.");
  }

  void give_large_buf() { buf.reset(new buf_type(*pointer)); }

  enum { ALLOW_NULL = 1, NO_NULL = 0 };

  void set_gzfile(std::string const& s, bool large_buf = kDefaultLargeBuf);

  void set_bz2(std::string const& s, bool large_buf = kDefaultLargeBuf);

  void set_lz4(std::string const& s, bool large_buf = kDefaultLargeBuf) {
    throw_fail(".lz4 support unimplimented");
  }

  // warning: if you specify the wrong values for read and file_only, you could assign the wrong type of
  // pointer and crash!
  void set(std::string const& s, bool null_allowed = ALLOW_NULL, bool large_buf = kDefaultLargeBuf) {
    typedef stream_traits<Stream> Traits;
    if (s.empty()) {
      throw_fail("<EMPTY FILENAME>", "Can't open an empty filename.  Use \"-0\" if you really mean no file");
    }
    if (Traits::is_default(s)) {
      // note that we don't attempt to supply a large buffer for either cin or cout,
      // since they're likely to already be in use by others
      Traits::call_set_default(*this, s);
    } else if (null_allowed && s == null_filename) {
      set_none();
      return;
    }
#if GRAEHL_USE_GZSTREAM
    else if (!Traits::file_only && gz_filename(s)) {
      set_gzfile(s);
    }
#endif
#if USE_BOOST_BZ2STREAM
    else if (!Traits::file_only && bz2_filename(s)) {
      set_bz2(s);
    }
#endif
#if GRAEHL_USE_LZ4
    else if (!Traits::file_only && lz4_filename(s)) {
      set_lz4(s);
    }
#endif
    else {
      Traits::call_set_file(*this, s, large_buf);
    }
    none = false;
  }

  bool is_stdin() const { return name == stdin_filename; }

  void set_none() {
    set_none_keep_name();
    name = null_filename;
  }

  void set_none_keep_name() {
    none = true;
    set_null_file_arg(pointer);
    buf.reset();
  }

  bool is_none() const { return none; }

  operator bool() const { return !is_none(); }

  bool is_default_in() const { return pointer.get() == &GRAEHL__DEFAULT_IN; }

  bool is_default_out() const { return pointer.get() == &GRAEHL__DEFAULT_OUT; }

  bool is_default_log() const { return pointer.get() == &GRAEHL__DEFAULT_LOG; }

  bool valid() const {
    if (is_none()) return false;
    if (!stream()) const_cast<file_arg*>(this)->set_none_keep_name();
    return true;
  }

  friend bool valid(self_type const& f) { return f.valid(); }

  Stream& stream() const { return *pointer; }

  Stream* get_nonnull() const { return is_none() ? 0 : pointer.get(); }

  template <class O>
  void print(O& o) const {
    o << name;
  }
  template <class I>
  void read(I& i) {
    std::string name;
    i >> name;
    set(name);
  }
  TO_OSTREAM_PRINT
  FROM_ISTREAM_READ
};

template <class Stream>
void swap(file_arg<Stream>& a, file_arg<Stream>& b) {
  a.swapWith(b);
}

typedef file_arg<std::istream> istream_arg;
typedef file_arg<std::ostream> ostream_arg;
typedef file_arg<std::ifstream> ifstream_arg;
typedef file_arg<std::ofstream> ofstream_arg;

inline istream_arg stdin_arg() {
  return istream_arg(stdin_filename);
}

inline ostream_arg stdout_arg() {
  return ostream_arg(stdout_filename);
}

inline ostream_arg stderr_arg() {
  return ostream_arg(stderr_filename);
}

inline std::string file_bytes(std::istream& i) {
  typedef std::istreambuf_iterator<char> I;
  return std::string(I(i.rdbuf()), I());
}

template <class OBytesIter>
OBytesIter copy_bytes(std::istream& i, OBytesIter o) {
  typedef std::istreambuf_iterator<char> I;
  return std::copy(I(i.rdbuf()), I(), o);
}


typedef shared_ptr<std::istream> Infile;
typedef shared_ptr<std::ostream> Outfile;
typedef shared_ptr<std::ifstream> InDiskfile;
typedef shared_ptr<std::ofstream> OutDiskfile;

static Infile default_in(&GRAEHL__DEFAULT_IN, null_deleter());
static Outfile default_log(&GRAEHL__DEFAULT_LOG, null_deleter());
static Outfile default_out(&GRAEHL__DEFAULT_OUT, null_deleter());
static Infile default_in_none;
static Outfile default_out_none;
static InDiskfile default_in_disk_none;
static OutDiskfile default_out_disk_none;

inline bool is_default_in(Infile const& i) {
  return i.get() == &GRAEHL__DEFAULT_IN;
}

inline bool is_default_out(Outfile const& o) {
  return o.get() == &GRAEHL__DEFAULT_OUT;
}

inline bool is_default_log(Outfile const& o) {
  return o.get() == &GRAEHL__DEFAULT_LOG;
}

inline bool is_none(Infile const& i) {
  return i.get() == NULL;
}

inline bool is_none(Outfile const& o) {
  return o.get() == NULL;
}

struct tee_file {
  tee_file() { reset_no_tee(); }

  void reset_no_tee(std::ostream& o = std::cerr) {
    teestreamptr.reset();
    teebufptr.reset();
    log_stream = &o;
  }

  /// must call before you get any tee behavior (without, will go to default log = cerr)!
  void set(std::ostream& other_output) {
    if (file) {
      teebufptr.reset(new graehl::teebuf(file->rdbuf(), other_output.rdbuf()));
      teestreamptr.reset(log_stream = new std::ostream(teebufptr.get()));
    } else {
      log_stream = &other_output;
    }
  }
  ostream_arg file;  // can set this directly, then call init.  if unset, then don't tee.

  std::ostream& stream() const { return *log_stream; }
  operator std::ostream&() const { return stream(); }

 private:
  std::ostream* log_stream;
  shared_ptr<graehl::teebuf> teebufptr;
  shared_ptr<std::ostream> teestreamptr;
};

template <class Stream>
inline bool valid(shared_ptr<Stream> const& pfile) {
  return pfile && *pfile;
}

template <class C>
inline void throw_unless_valid(C const& pfile, std::string const& name = "file") {
  if (!valid(pfile)) throw std::runtime_error(name + " not valid");
}

template <class C>
inline void throw_unless_valid_optional(C const& pfile, std::string const& name = "file") {
  if (pfile && !valid(pfile)) throw std::runtime_error(name + " not valid");
}

inline Infile infile(std::string const& s) {
  return istream_arg(s);
}

inline InDiskfile indiskfile(std::string const& s) {
  return ifstream_arg(s);
}

inline fs::path full_path(std::string const& relative) {
  return fs::system_complete(fs::path(relative));  // fs::path( relative, fs::native ) //V2
}

inline bool directory_exists(fs::path const& possible_dir) {
  return fs::exists(possible_dir) && fs::is_directory(possible_dir);
}

// works on .gz files!
inline size_t count_newlines(std::string const& filename) {
  Infile i = infile(filename);
  char c;
  size_t n_newlines = 0;
  while (i->get(c)) {
    if (c == '\n') ++n_newlines;
  }
  return n_newlines;
}

inline void native_filename_check() {
  //  fs::path::default_name_check(fs::native); // V2
}


// like V2 normalize
inline fs::path resolve(fs::path const& p) {
  fs::path result;
  for (fs::path::iterator i = p.begin(); i != p.end(); ++i) {
    if (*i == "..") {
      // /a/b/.. is not necessarily /a if b is a symbolic link
      if (fs::is_symlink(result)) result /= *i;
      // /a/b/../.. is not /a/b/.. under most circumstances
      // We can end up with ..s in our result because of symbolic links
      else if (result.filename() == "..")
        result /= *i;
      // Otherwise i should be safe to resolve the parent
      else
        result = result.parent_path();
    } else if (*i != ".") {
      // Just cat other path entries
      result /= *i;
    }
  }
  return result;
}

// return the absolute filename that would result from "cp source dest" (and write to *dest_exists whether
// dest exists) - throws error if source is the same as dest
inline std::string output_name_like_cp(std::string const& source, std::string const& dest,
                                       bool* dest_exists = NULL) {
  fs::path full_dest = full_path(dest);
  fs::path full_source = full_path(source);

  if (directory_exists(full_dest)) full_dest /= full_source.filename();
  if (dest_exists && fs::exists(full_dest)) *dest_exists = 1;


  full_dest = resolve(full_dest);
  full_source = resolve(full_source);
  //  full_dest.normalize();
  //  full_source.normalize();
  // v2 only

  if (fs::equivalent(full_dest, full_source)) throw std::runtime_error("Destination file is same as source!");
#ifdef _MSC_VER
  boost::filesystem::detail::utf8_codecvt_facet utf8;
//  boost::path::imbue(); // to get utf8 string
#endif
  return full_dest.string<std::string>(
#ifdef _MSC_VER
      utf8
#endif
      );  // v2: native_file_string() //TODO(windows): codecvt -> utf8, or is default fine? issue is that file
  // opens on win32 expect UTF-16 wstrings.
}

inline Outfile outfile(std::string const& s) {
  return ostream_arg(s);
}

inline OutDiskfile outdiskfile(std::string const& s) {
  return ofstream_arg(s);
}


inline shared_ptr<std::string> streambufContents(std::streambuf* sbuf) {
  typedef std::istreambuf_iterator<char> Iter;
  return shared_ptr<std::string>(new std::string(Iter(sbuf), Iter()));
}

inline shared_ptr<std::string> streamContents(std::istream& istream) {
  return streambufContents(istream.rdbuf());
}

template <class Istream>
inline shared_ptr<std::string> fileContents(file_arg<Istream> const& file) {
  std::size_t sz = file.filesize();
  if (sz) {
    shared_ptr<std::string> r(new std::string(sz, '\0'));
    file->rdbuf()->sgetn(&(*r)[0], sz);
  } else
    return streamContents(*file);
}

inline bool is_fstream_no_rtti(std::istream& in) {
  if (&in == &(std::istream&)std::cin || in.tellg() == (std::streampos)-1 || !in) return false;
  try {
    throw & in;
  } catch (std::ifstream*) {
    return true;
  } catch (...) {
    return false;
  }
}

}  // graehl

#if (!defined(GRAEHL__NO_GZSTREAM_MAIN) && defined(GRAEHL__SINGLE_MAIN)) || defined(GRAEHL__GZSTREAM_MAIN)
#include <graehl/shared/fileargs.cpp>
#endif

namespace boost {
namespace program_options {

#if GRAEHL__VALIDATE_INFILE
/* Overload the 'validate' function for shared_ptr<std::istream>. We use shared ptr
   to properly kill the stream when it's no longer used.
*/
inline void validate(boost::any& v, std::vector<std::string> const& values,
                     shared_ptr<std::istream>* target_type, int) {
  v = boost::any(graehl::infile(graehl::get_single_arg(v, values)));
}

inline void validate(boost::any& v, std::vector<std::string> const& values,
                     shared_ptr<std::ostream>* target_type, int) {
  v = boost::any(graehl::outfile(graehl::get_single_arg(v, values)));
}

inline void validate(boost::any& v, std::vector<std::string> const& values,
                     shared_ptr<std::ofstream>* target_type, int) {
  v = boost::any(graehl::outdiskfile(graehl::get_single_arg(v, values)));
}

inline void validate(boost::any& v, std::vector<std::string> const& values,
                     shared_ptr<std::ifstream>* target_type, int) {
  v = boost::any(graehl::indiskfile(graehl::get_single_arg(v, values)));
}

#else

#ifdef _MSC_VER
inline void validate(boost::any& v, std::vector<std::string> const& values,
                     graehl::file_arg<std::istream>* target_type, int) {
  v = boost::any(graehl::file_arg<std::istream>(graehl::get_single_arg(v, values)));
}

inline void validate(boost::any& v, std::vector<std::string> const& values,
                     graehl::file_arg<std::ostream>* target_type, int) {
  v = boost::any(graehl::file_arg<std::ostream>(graehl::get_single_arg(v, values)));
}

inline void validate(boost::any& v, std::vector<std::string> const& values,
                     graehl::file_arg<std::ifstream>* target_type, int) {
  v = boost::any(graehl::file_arg<std::ifstream>(graehl::get_single_arg(v, values)));
}


inline void validate(boost::any& v, std::vector<std::string> const& values,
                     graehl::file_arg<std::ofstream>* target_type, int) {
  v = boost::any(graehl::file_arg<std::ofstream>(graehl::get_single_arg(v, values)));
}

#else
template <class Stream>
inline void validate(boost::any& v, std::vector<std::string> const& values,
                     graehl::file_arg<Stream>* target_type, int) {
  v = boost::any(graehl::file_arg<Stream>(graehl::get_single_arg(v, values)));
}
#endif
#endif


}}

#endif
