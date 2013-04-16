// wraps Boost or regular random number generators
#ifndef GRAEHL_SHARED__RANDOM_HPP
#define GRAEHL_SHARED__RANDOM_HPP

#ifndef GRAEHL_USE_RANDOM_DEVICE
# define GRAEHL_USE_RANDOM_DEVICE 1
#endif

#ifndef GRAEHL_GLOBAL_RANDOM_USE_STD
# define GRAEHL_GLOBAL_RANDOM_USE_STD 0
#endif


#include <cmath> // also needed for boost/random :( (pow)
#include <algorithm> // min for boost/random
//#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/lagged_fibonacci.hpp>
#include <boost/random/variate_generator.hpp>

#if GRAEHL_USE_RANDOM_DEVICE
#include <boost/random/random_device.hpp>
#endif

#if GRAEHL_GLOBAL_RANDOM_USE_STD
# include <cstdlib>
#endif

#include <boost/scoped_array.hpp>
#include <boost/optional.hpp>

#include <ctime>
#include <graehl/shared/os.hpp>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <cctype>
#endif

namespace graehl {

typedef unsigned random_seed_type; // this is what random_device actually returns.

/**
   for a series of runs with different, but reproducible, seeds.
*/
inline random_seed_type debugging_random_seed(random_seed_type n)
{
  return (n+1)*2654435769U;
}

inline random_seed_type default_random_seed()
{
  // long pid=get_process_id();
#if GRAEHL_USE_RANDOM_DEVICE
  return boost::random_device().operator()();
#else
  random_seed_type pid=(random_seed_type)get_process_id();
  return (random_seed_type)std::time(0) ^ pid ^ (pid << 17);
#endif
}

/**
   allow configuring constant initial seeds.
*/
struct random_seed {
  boost::optional<random_seed_type> seed;
  template <class Configure>
  void configure(Configure &c) {
    c("random-seed",&seed)("if set, use as initial seed for RNG");
  }
  operator random_seed_type() const { return seed ? *seed : default_random_seed(); }
};

typedef boost::lagged_fibonacci607 random_generator;
// lagged_fibonacci607 is the fastest for generating random floats and only 20% slower for ints - see http://www.boost.org/doc/libs/1_52_0/doc/html/boost_random/performance.html
typedef boost::uniform_01<double> uniform_01_dist;
typedef boost::variate_generator<random_generator,uniform_01_dist> random_01_generator;


#if !GRAEHL_GLOBAL_RANDOM_USE_STD
#if (!defined(GRAEHL__NO_RANDOM_MAIN) && defined(GRAEHL__SINGLE_MAIN)) || defined(GRAEHL__RANDOM_MAIN)
namespace {
//random_generator g_random_gen(default_random_seed());
}
random_01_generator g_random01((random_generator(default_random_seed())),uniform_01_dist());
#else
extern random_01_generator g_random01;
#endif
#endif

inline void set_random_seed(random_seed_type value=default_random_seed())
{
#if GRAEHL_GLOBAL_RANDOM_USE_STD
  srand(value);
#else
  g_random01.engine().seed(value);
#endif
}


//FIXME: use boost random? and can't necessarily port executable across platforms with different rand syscall :(
inline double random01() // returns uniform random number on [0..1)
{
# if GRAEHL_GLOBAL_RANDOM_USE_STD
  return ((double)std::rand()) * (1. /((double)RAND_MAX+1.));
# else
  return g_random01();
# endif
}

/**
   global (thread-unsafe).
*/
# include <graehl/shared/random.ipp>

struct set_random_pos_fraction {
  template <class C>
  void operator()(C &c) {
    c=random_pos_fraction();
  }
};

/**
   use explicit random state for thread safety.
*/
struct random {
  typedef double result_type;
  random_01_generator random01;
  result_type operator()() {
    return random01();
  }
  random(random_seed_type seed=default_random_seed())
      : random01(random_generator(seed),uniform_01_dist())
  {}
  inline void set_random_seed(random_seed_type value=default_random_seed())
  {
    random01.engine().seed(value);
  }
# include <graehl/shared/random.ipp>
};

#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE( TEST_RANDOM )
{
  using namespace std;
  const unsigned NREP=10000;
  for (unsigned i=1;i<NREP;++i) {
    unsigned ran_lt_i=random_less_than(i);
    BOOST_CHECK(0 <= ran_lt_i && ran_lt_i < i);
    BOOST_CHECK(std::isalpha(random_alpha()));
    char r_alphanum=random_alphanum();
    BOOST_CHECK(std::isalpha(r_alphanum) || std::isdigit(r_alphanum));
  }
}
#endif
}

#endif
