// wraps Boost or regular random number generators
#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <graehl/shared/myassert.h>

#ifdef SOLARIS
//# include <sys/int_types.h>
//FIXME: needed for boost/random - report bug to boost.org? (actually no, had uint32_t, just bug in pass_through_engine.hpp trying to get uint32_t::base_type
#endif 

#include <cmath> // also needed for boost/random :( (pow)
#include <algorithm> // min for boost/random
#include <boost/random.hpp>
#ifdef USE_NONDET_RANDOM
# ifndef LINUX
#  undef USE_NONDET_RANDOM
# else 
#  include <boost/nondet_random.hpp>
# if defined(SINGLE_MAIN) || defined (RANDOM_SINGLE_MAIN)
#  include "nondet_random.cpp"
# endif
#endif 
#endif

#ifdef USE_STD_RAND
# include <cstdlib>
#endif 

#include <boost/scoped_array.hpp>

#include <ctime>
#include <graehl/shared/os.hpp>

inline unsigned default_random_seed()
{
//    long pid=get_process_id();
# ifdef USE_NONDET_RANDOM
    return boost::random_device().operator()();
# else
        unsigned pid=get_process_id();
    return std::time(0) + pid + (pid << 17);
# endif
}

#ifndef USE_STD_RAND
//FIXME: maybe use faster integer type rng?  then maybe faster random ints
typedef boost::lagged_fibonacci607 G_rgen;

typedef boost::uniform_01<G_rgen> G_rdist;
#ifdef SINGLE_MAIN
static G_rgen g_random_gen(default_random_seed());
G_rdist g_random01(g_random_gen);
#else
extern G_rdist g_random01;
#endif
#endif

inline void set_random_seed(boost::uint32_t value=default_random_seed())
{
#ifdef USE_STD_RAND
    srand(value);
#else
    g_random01.base().seed(value);
#endif
}


//FIXME: use boost random?  and can't necessarily port executable across platforms with different rand syscall :(
inline double random01() // returns uniform random number on [0..1)
{
# ifdef USE_STD_RAND

    return ((double)std::rand()) *        (1. /((double)RAND_MAX+1.));
# else
    return g_random01();
# endif
}

inline double random_pos_fraction() // returns uniform random number on (0..1]
{
#ifdef  USE_STD_RAND
    return ((double)std::rand()+1.) *
        (1. / ((double)RAND_MAX+1.));
#else
    return 1.-random01();
#endif
}

template <class V1,class V2>
inline V1 random_half_open(const V1 &v1, const V2 &v2) 
{
    return v1+random01()*(v2-v1);
}

struct set_random_pos_fraction {
    template <class C>
    void operator()(C &c) {
        c=random_pos_fraction();
    }
};

inline size_t random_less_than(size_t limit) {
    Assert(limit!=0);
    if (limit <= 1)
        return 0;
#ifdef USE_STD_RAND
// correct against bias (which is worse when limit is almost RAND_MAX)
    const size_t randlimit=(RAND_MAX / limit)*limit;
    size_t r;
    while ((r=std::rand()) >= randlimit) ;
    return r % limit;
#else
    return (size_t)(random01()*limit);
#endif
}

inline size_t random_up_to(size_t limit) {
    return random_less_than(limit+1);
}



#define NLETTERS 26
// works for only if a-z A-Z and 0-9 are contiguous
inline char random_alpha() {
    unsigned r=random_less_than(NLETTERS*2);
    return (r < NLETTERS) ? 'a'+r : ('A'-NLETTERS)+r;
}

inline char random_alphanum() {
    unsigned r=random_less_than(NLETTERS*2+10);
    return r < NLETTERS*2 ? ((r < NLETTERS) ? 'a'+r : ('A'-NLETTERS)+r) : ('0'-NLETTERS*2)+r;
}
#undef NLETTERS

inline std::string random_alpha_string(unsigned len) {
    boost::scoped_array<char> s(new char[len+1]);
    char *e=s.get()+len;
    *e='\0';
    while(s.get() < e--)
        *e=random_alpha();
    return s.get();
}

template <class It>
void randomly_permute(It begin,It end) 
{
    using std::swap;
    size_t N=end-begin;
    for (size_t i=0;i<N;++i) {
        swap(*(begin+i),*(begin+random_up_to(i)));
    }
}

template <class V>
void randomly_permute(V &vec)
{
    using std::swap;
    size_t N=vec.size();
    for (size_t i=0;i<N;++i) {        
        swap(vec[i],vec[random_up_to(i)]);
    }
}

#ifdef TEST
#include <graehl/shared/test.hpp>
#include <cctype>
#include <graehl/shared/debugprint.hpp>
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_RANDOM )
{
    using namespace std;
    const int NREP=10000;
    for (int i=1;i<NREP;++i) {
        unsigned ran_lt_i=random_less_than(i);
        BOOST_CHECK(0 <= ran_lt_i && ran_lt_i < i);
        BOOST_CHECK(isalpha(random_alpha()));
        char r_alphanum=random_alphanum();
        BOOST_CHECK(isalpha(r_alphanum) || isdigit(r_alphanum));
    }
}
#endif

#endif
