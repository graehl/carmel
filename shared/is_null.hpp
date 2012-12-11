#ifndef GRAEHL__SHARED__IS_NULL_HPP
#define GRAEHL__SHARED__IS_NULL_HPP

#ifdef GRAEHL_TEST
# include <graehl/shared/test.hpp>
# include <graehl/shared/debugprint.hpp>
# include <boost/lexical_cast.hpp>
# define IS_NULL_DEBUG(x) x
#else
# define IS_NULL_DEBUG(x)
#endif

//find is_null, set_null by ADL (Koenig lookup).

#include <graehl/shared/nan.hpp>

//#define FLOAT_NULL HUGE_VALF
//#define DOUBLE_NULL HUGE_VAL
#define FLOAT_NULL float(NAN)
#define DOUBLE_NULL double(NAN)

template <class C> inline
bool is_null(C const &c)
{ return !c; }

template <class C> inline
void set_null(C &c)
{ c=C(); }

inline bool is_null(float const& f)
{
    return GRAEHL_ISNAN(f);
}

inline void set_null(float &f)
{
    f=FLOAT_NULL;//0./0.;
}

inline bool is_null(double const& f)
{
    return GRAEHL_ISNAN(f);
}

inline void set_null(double &f)
{
    f=DOUBLE_NULL;//0./0.;
}

template <class C> inline
bool non_null(C const &c)
{ return !is_null(c); }

struct as_null {};
// tag for constructors

#define MEMBER_IS_SET_NULL MEMBER_SET_NULL MEMBER_IS_NULL

#define MEMBER_SET_NULL     friend bool is_null(self_type const& me) { return me.is_null(); }
#define MEMBER_IS_NULL     friend void is_null(self_type & me) { return me.set_null(); }


#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE(TEST_is_null) {
    using namespace std;
    using namespace boost;
    using namespace graehl;

    double d=std::numeric_limits<double>::infinity();
    DBP2(d,is_null(d));
    BOOST_CHECK_EQUAL(is_null(d),false);
    BOOST_CHECK_EQUAL(is_nan(d),false);
    set_null(d);
    DBP2(d,is_null(d));
    BOOST_CHECK_EQUAL(is_null(d),true);
    BOOST_CHECK_EQUAL(is_nan(d),true);

    float f=std::numeric_limits<float>::infinity();
    DBP2(f,is_null(f));
    BOOST_CHECK_EQUAL(is_null(f),false);
    BOOST_CHECK_EQUAL(is_nan(f),false);
    set_null(f);
    DBP2(f,is_null(f));
    BOOST_CHECK_EQUAL(is_null(f),true);
    BOOST_CHECK_EQUAL(is_nan(f),true);

}
#endif

#endif
