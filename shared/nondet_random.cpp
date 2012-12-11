/* boost random_device.cpp implementation
 *
 * Copyright Jens Maurer 2000
 * Permission to use, copy, modify, sell, and distribute this software
 * is hereby granted without fee provided that the above copyright notice
 * appears in all copies and that both that copyright notice and this
 * permission notice appear in supporting documentation,
 *
 * Jens Maurer makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * $Id: nondet_random.cpp 3574 2011-12-22 01:56:57Z graehl $
 *
 */

#include <boost/nondet_random.hpp>
#include <string>
#include <cassert>


#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
const bool boost::random_device::has_fixed_range;
const boost::random_device::result_type boost::random_device::min_value;
const boost::random_device::result_type boost::random_device::max_value;
#endif


#ifdef __linux__

// the default is the unlimited capacity device, using some secure hash
// try "/dev/random" for blocking when the entropy pool has drained
const char * const boost::random_device::default_token = "/dev/urandom";

/*
 * This uses the POSIX interface for unbuffered reading.
 * Using buffered std::istream would consume entropy which may
 * not actually be used.  Entropy is a precious good we avoid
 * wasting.
 */

#if defined(__GNUC__) && defined(_CXXRT_STD_NAME)
// I have severe difficulty to get the POSIX includes to work with
// -fhonor-std and Dietmar Kühl's standard C++ library.  Hack around that
// problem for now.
extern "C" {
static const int O_RDONLY = 0;
extern int open(const char *__file, int __oflag, ...);
extern int read(int __fd, __ptr_t __buf, size_t __nbytes);
extern int close(int __fd);
}
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>    // open
#include <unistd.h>   // read, close
#endif

#include <errno.h>    // errno
#include <string.h>   // strerror
#include <stdexcept>  // std::invalid_argument


class boost::random_device::impl
{
public:
  impl(const std::string & token) : path(token) {
    fd = open(token.c_str(), O_RDONLY);
    if(fd < 0)
      error("cannot open");
  }

  ~impl() { if(close(fd) < 0) error("could not close"); }

  unsigned int next() {
    unsigned int result;
    long sz = read(fd, reinterpret_cast<char *>(&result), sizeof(result));
    if(sz == -1)
      error("error while reading");
    else if(sz != sizeof(result)) {
      errno = 0;
      error("EOF while reading");
    }
    return result;
  }

private:
  void error(const std::string & msg) {
    throw std::invalid_argument("boost::random_device: " + msg +
                                " random-number pseudo-device " + path +
                                ": " + strerror(errno));
  }
  const std::string path;
  int fd;
};

#endif // __linux__

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef VC_EXTRALEAN
# define VC_EXTRALEAN 1
#endif
#ifndef NOMINMAX
# define NOMINMAX 1
#endif
#include <windows.h>
#include <wincrypt.h>
#undef max
#undef min
#undef DELETE
#include <boost/lexical_cast.hpp>

const char* const boost::random_device::default_token = MS_DEF_PROV;

class boost::random_device::impl
{
public:
  impl(const std::string& token)
  {
    if (!CryptAcquireContext(&provider, 0, token.c_str(), PROV_RSA_FULL, CRYPT_SILENT)) {
      DWORD error_code = GetLastError();

      if (error_code == NTE_BAD_KEYSET) {
        // try requesting a new keyset
        if (!CryptAcquireContext(&provider, 0, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET | CRYPT_SILENT))
            error("cannot open", GetLastError());
      }
      else
        error("cannot open", error_code);
    }
  }

  ~impl()
  {
    if (!CryptReleaseContext(provider, 0))
      error("cannot close", GetLastError());
  }

  unsigned int next()
  {
    unsigned int result;

    if (!CryptGenRandom(provider, sizeof(result), reinterpret_cast<BYTE*>(&result)))
      error("error while generating", GetLastError());

    return result;
  };

private:
  HCRYPTPROV provider;

  void error(const std::string& msg, DWORD error) {
    throw std::invalid_argument("boost::random_device: " + msg +
        ": " + boost::lexical_cast<std::string>(errno));
  }
};

#endif // Windows

boost::random_device::random_device(const std::string& token)
  : pimpl(new impl(token))
{
  assert(std::numeric_limits<result_type>::max() == max_value);
}

boost::random_device::~random_device()
{
  // the complete class impl is now visible, so we're safe
  // (see comment in random.hpp)
  delete pimpl;
}

double boost::random_device::entropy() const
{
  return 10;
}

unsigned int boost::random_device::operator()()
{
  return pimpl->next();
}
