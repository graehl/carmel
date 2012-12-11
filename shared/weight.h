#ifndef GRAEHL_SHARED__WEIGHT_H
#define GRAEHL_SHARED__WEIGHT_H

/*
All these are legal input (and output) from Carmel, and ForestEM:

0
e^-2.68116e+11
-2.68116e+11ln
0.0952381
e^-4086.42
-4086.42ln
0.047619
1

UPDATE: so that nobody has to be confused about what the "ln" suffix means, the new default output shall be "e^n" (except in case of zero, which is just "0" and not "e^-INF", since "-INF" doesn't read/write properly using the standard I/O libraries). I suggest everyone only produce such outputs, but be prepared to read arbitrary real numbers, as well as those starting with "e^".

That is: either a number in the form supported by C/C++ i/o of double floating point numbers, OR such a number (call it N) written "e^N", OR (deprecated) written "Nln", indicating that the number is actually e^N. e.g. e^0=1.

\forall N,Nln=eN

There is well-tested log-rep addition/subtraction code in GraehlCVS (graehl/shared/weight.h).

Carmel optionally supports the use of base 10 instead: \forall N,Nlog=10^N, but that is no longer tolerated - e is the only natural base (and "log" sometimes means base 2, like in information theory, so it's confusing).
*/

#include <boost/lexical_cast.hpp>
#include <graehl/shared/nan.hpp>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/config.h>
#include <graehl/shared/myassert.h>
#include <graehl/shared/genio.h>
#include <graehl/shared/funcs.hpp>
#include <graehl/shared/threadlocal.hpp>
#include <graehl/shared/random.hpp>
#include <cstdlib>
#include <limits>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <cctype>
#include <graehl/shared/debugprint.hpp>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#endif
#include <cmath>
#include <iostream>

#ifdef _MSC_VER
// Microsoft C++: conversion from double to FLOAT_TYPE:
#pragma warning(disable:4244)
#endif


namespace graehl {
#ifdef GRAEHL_USE_LOG1P
# include <math.h>
#endif

struct quiet_NaN_weight {};
struct signaling_NaN_weight {};
struct zero_weight {};
struct one_weight {};
struct inf_weight {};
struct neglog10_weight {};
struct negln_weight {};
struct log10_weight {};
struct ln_weight {};
struct cost_weight {}; // impl. base, which happens to be negln

static const double HUGE_FLOAT = (HUGE_VAL*HUGE_VAL);

//! warning: unless #ifdef WEIGHT_CORRECT_ZERO
// Weight(0) will may give bad results when computed with, depending on math library behavior
// defining WEIGHT_CORRECT_ZERO will incur a performance penalty

template<class Real>
struct logweight;

#define MUCH_BIGGER_LN (sizeof(Real)==4? (Real)16. : (Real)36.)
// represents BIG=10^MUCH_BIGGER_LN - if X > BIG*Y, then X+Y =~ X 32 bit Real
// IEEE Real has 23 binary digit mantissa, so can represent about 16 base E
// digits only if you use 64-bit doubles instead of floats, 52 binary digits
// note that you'll start to see underflow slightly earlier - this is a
// conservative value.  subtract one if you *want* the result of addition to be
// not lose tons of information
/* in other words log(exp1p(exp(a))-1 = 0 for a>=MUCH_BIGGER_LN
 */
#define UNDERFLOW_LN (sizeof(Real)==4? (Real)73. : (Real)82.)

//#define MAKENOTANON name ## __LINE__
template<class Real>
struct logweight {                 // capable of representing nonnegative reals
    // internal implementation note: by their base e logarithm
    typedef logweight<Real> self_type;
    Real weight;
    typedef Real float_type;
    void swap(self_type &a) throw() {
        using std::swap;
        swap(weight,a.weight);
    }
    inline friend void swap(self_type &a,self_type &b) throw()
    {
        a.swap(b);
    }
 private:
    enum MAKENOTANON  { DEFAULT_BASE=0,LN=1,LOG10=2,EXP=3 }; // EXP is same as LN but write e^10e-6 not 10e-6ln
    enum name2 { DEFAULT_LOG=0,ALWAYS_LOG=1, SOMETIMES_LOG=2, NEVER_LOG=3 };
    // IEE float safe till about 10^38, loses precision earlier (10^32?) or 2^127 -> 2^120
    // 32 * ln 10 =~ 73
    // double goes up to 2^1027, loses precision at say 2^119?  119 * ln 2 = 82
//  enum make_not_anon_26 {LN_TILL_UNDERFLOW=} ;
    static inline Real LN_TILL_UNDERFLOW()
    {
        return UNDERFLOW_LN;
    }
    static inline Real LN_MUCH_LARGER()
    {
        return MUCH_BIGGER_LN;
    }
  static const Real neg_ln10;
  static const Real ln10;
  static const Real neg_oo_ln10;
  static const Real oo_ln10;
    static const int base_index; // handle to ostream iword for LogBase enum (initialized to 0)
    static const int thresh_index; // handle for OutThresh
    static THREADLOCAL int default_base;
    static THREADLOCAL int default_thresh;
 public:
    // linux g++ 3.2 didn't like static self-class member
    static const Real FLOAT_INF() {
        return HUGE_FLOAT;
    }

    // output format manipulators: cout << Weight::out_log10;
    static void default_log10() {
        default_base=LOG10;
    }
    static void default_ln() {
        default_base=LN;
    }
    static void default_exp() {
        default_base=EXP;
    }
    static void default_sometimes_log() {
        default_thresh=SOMETIMES_LOG;
    }
    static void default_always_log() {
        default_thresh=ALWAYS_LOG;
    }
    static void default_never_log() {
        default_thresh=NEVER_LOG;
    }
    static Real getlogreal(double d) {
        return log(d);
    }
    static Real getlogreal(float d) {
        return log(d);
    }
    template<class charT, class Traits>
    static int get_log(std::basic_ostream<charT,Traits>& o) {
        int thresh=o.iword(thresh_index);
        if (thresh == DEFAULT_LOG)
            thresh=default_thresh;
        return thresh;
    }

    template<class charT, class Traits>
    static int get_log_base(std::basic_ostream<charT,Traits>& o) {
        int thresh=o.iword(base_index);
        if (thresh == DEFAULT_BASE)
            thresh=default_base;
        return thresh;
    }


    template<class A,class B> static std::basic_ostream<A,B>&
    out_default_base(std::basic_ostream<A,B>& os) { os.iword(base_index) = DEFAULT_BASE; return os; }
    template<class A,class B> static std::basic_ostream<A,B>&
    out_log10(std::basic_ostream<A,B>& os)  { os.iword(base_index) = LOG10; return os; }

    template<class A,class B> static std::basic_ostream<A,B>&
    out_exp(std::basic_ostream<A,B>& os)  { os.iword(base_index) = EXP; return os; }

    template<class A,class B> static std::basic_ostream<A,B>&
    out_ln(std::basic_ostream<A,B>& os) { os.iword(base_index) = LN; return os; }

    template<class A,class B> static std::basic_ostream<A,B>&
    out_sometimes_log(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = SOMETIMES_LOG; return os; }

    template<class A,class B> static std::basic_ostream<A,B>&
    out_always_log(std::basic_ostream<A,B>& os)  { os.iword(thresh_index) = ALWAYS_LOG; return os; }

    template<class A,class B> static std::basic_ostream<A,B>&
    out_never_log(std::basic_ostream<A,B>& os)  { os.iword(thresh_index) = NEVER_LOG; return os; }

    template<class A,class B> static std::basic_ostream<A,B>&
    out_default_log(std::basic_ostream<A,B>& os)  { os.iword(thresh_index) = DEFAULT_LOG; return os; }


//  static self_type result;
    // default = operator:

    //double toFloat() const {
    //return getReal();
    //}

    //  *this > o <=>  returned ratio > 1 (bigger is worse).  unlike *this/o, this is scale invariant
    self_type relative_perplexity_ratio(self_type const& o) const
    {
        return (*this/o).root(std::fabs(getLogImp())); // EM delta=(L'-L)/abs(L')
    }

    double getReal() const {
        return std::exp(weight);
    }
    // weight -> (double -> weight  xtor) problem?  make it explicit?
    operator double() const
    {
        return getReal();
    }

    void setRandomFraction() {
        setReal(random_pos_fraction());
    }
    Real getLog(Real base) const {
        return weight / std::log(base);
    }
    Real getLogImp() const {
        return weight;
    }
    typedef Real cost_type;
    cost_type getCost() const {
        return -weight;
    }
    void setCost(Real f) {
        weight=-f;
    }

    Real getLn() const {
        return weight;
    }
    Real getNegLn() const
    {
        return -weight;
    }
    Real getNegLog10() const
    {
        return neg_oo_ln10 * weight;
    }
    Real getLog10() const {
        return oo_ln10 * weight;
    }
    bool fitsInReal() const {
        return isZero() || (getLn() < LN_TILL_UNDERFLOW() && getLn() > -LN_TILL_UNDERFLOW());
    }
    bool isInfinity() const {
        return weight == FLOAT_INF();
    }
    void setInfinity() {
        weight = FLOAT_INF();
    }

    bool isZero() const {
//    return weight == -FLOAT_INF();
        return !isPositive();
    }
    bool isPositive() const {
        return weight > -FLOAT_INF();
    }
    static self_type much_larger(const self_type o=1.) {
        return MUCH_BIGGER_LN + o.weight;
    }
    template <class Real2>
    bool isMuchLargerThan(const logweight<Real2> &o) const {
        return weight-o.weight > MUCH_BIGGER_LN;
    }
#ifndef NEAR_LN_PROXIMITY
# define NEAR_LN_PROXIMITY 2
# endif
    bool isNearAddOneLimit() const {
# ifdef TEST_ADD_ONE_LIMIT
        return weight > .0001;
#  else
        return weight > (MUCH_BIGGER_LN - NEAR_LN_PROXIMITY);
#  endif
    }
    void setZero() {
        weight = -FLOAT_INF();
    }
    bool isOne() const {
        return weight==0;
    }
    void setOne() {
        weight=0;
    }
    void setReal(double f) {
        if (f > 0)
            weight=(Real)log(f);
        else
            setZero();
    }
    void NaNCheck() const {
      assert(!is_nan(weight));
    }
    void setLn(Real w) {
        weight=w;
    }
    void setLog10(Real w) {
        weight=w*ln10;
    }
    void setNegLog10(Real w) {
        weight=w*neg_ln10;
    }
    static self_type ZERO() {
        return self_type();
    }
    static self_type ONE()
    {
        return self_type(one_weight());
    }

    static self_type  INF() {
        return self_type(false,false);
    }

    // print return.as_base(2)
    self_type ppxper(double n=1) const
    {
        return root(-n);
    }


    std::ostream &print_ppx_symbol(std::ostream &o,double n_isymbol,double n_osymbol,double n_example
                            ,char const* symbolname="per-symbol-perplexity"
                            ,char const* examplename="per-example-perplexity"
                            , char const* probname="probability"
        ) const
    {
        return print_ppx(o,std::max(n_osymbol,n_isymbol),n_example,symbolname,examplename,probname);
    }

    std::ostream &print_ppx(std::ostream &o,double n_symbol,double n_example
                            ,char const* symbolname="per-output-symbol-perplexity"
                            ,char const* examplename="per-example-perplexity"
                            , char const* probname="probability"
        ) const
    {
        o<<probname<<"="<<as_base(2);
        if (n_symbol)
            o<<" "<<symbolname<<"(N="<<n_symbol<<")="<<ppxper(n_symbol).as_base(2);
        if (n_example)
            o<<" "<<examplename<<"(N="<<n_example<<")="<<ppxper(n_example).as_base(2);
        return o;
    }

    std::ostream &print_ppx_example(std::ostream &o,double n_example
                            ,char const* examplename="per-example-perplexity"
                            , char const* probname="probability"
        ) const
    {
        o<<probname<<"="<<as_base(2);
        if (n_example>0)
            o<<" "<<examplename<<"(N="<<n_example<<")="<<ppxper(n_example).as_base(2);
        return o;
    }

    logweight() { setZero(); }
    logweight(quiet_NaN_weight t) {weight=std::numeric_limits<Real>::quiet_NaN(); }
    logweight(signaling_NaN_weight t) {weight=std::numeric_limits<Real>::signaling_NaN(); }
    logweight(zero_weight t) {setZero();}
    logweight(one_weight t) {setOne(); }
    logweight(inf_weight t) {setInfinity(); }
    logweight(Real w,ln_weight t) {setLn(w);}
    logweight(Real w,negln_weight t) {setLn(-w);}
    logweight(Real w,log10_weight t) {setLog10(w);}
    logweight(Real w,neglog10_weight t) {setNegLog10(w);}
    logweight(Real w,cost_weight t) {setCost(w);}
    logweight(bool,bool) { setInfinity(); }
    logweight(Real log_weight,bool dummy) : weight(log_weight) {}
    template <class Real2>
    logweight(const logweight<Real2> &o) : weight(o.weight) {}
#if 0
    template <class numeric>
    logweight(numeric n) {
        setReal(n);
    }
#else
    logweight(double f) {setReal(f);} // interesting to allow bigger float types than Real since exponent on double is bigger and can fit in Real=float when log'd
//    logweight(Real f) {setReal(f);}
//    logweight(float f) {setReal(f);}
//    logweight(std::size_t f) {setReal(f);}
    logweight(int f) {setReal(f);}
    logweight(unsigned f) {setReal(f);}
    logweight(self_type const& base,Real power)
    {
        weight=base.weight*power;
    }

    logweight(const std::string &str)
    {
        /*
          std::istringstream is(str);
          if (read(is) == GENIOBAD) {
          throwbadweight();
          }
        */
        if (!setString(str))
            throwbadweight();
    }
    explicit logweight(const char *str)
    {
        if (!setString(str))
            throwbadweight();
    }
    logweight(const char *begin, const char *end)
    {
        if (!setString(begin,end))
            throwbadweight();
    }

#endif
    self_type &operator += (self_type w)
    {
        *this = *this + w;
        return *this;
    }
    self_type &operator -= (self_type w)
    {
        *this = *this - w;
        return *this;
    }
    self_type &operator *= (self_type w)
    {
#ifdef WEIGHT_CORRECT_ZERO
        if (!isZero())
#endif
            weight += w.weight;
        return *this;
    }
    self_type &operator /= (self_type w)
    {
        Assert(!w.isZero());
#ifdef WEIGHT_CORRECT_ZERO
//              if (w.isZero())
        //      weight = FLOAT_INF();
        //else
        if (!isZero())
#endif
            weight -= w.weight;

        return *this;
    }
    self_type &raisePower(Real power) {
#ifdef WEIGHT_CORRECT_ZERO
        if (!isZero())
#endif
            weight *= power;
        return *this;
    }
    self_type &invert() {
        weight = -weight;
        return *this;
    }
    self_type inverse() {
        return self_type(-weight,false);
    }
    self_type &takeRoot(Real nth) {
#ifdef WEIGHT_CORRECT_ZERO
        if (!isZero())
#endif
            weight /= nth;
        return *this;
    }
    self_type root(Real nth) const
    {
#ifdef WEIGHT_CORRECT_ZERO
        if (isZero())
            return *this;
#endif
        return self_type(weight/nth,false);
    }

    self_type pow(Real nth) const
    {
#ifdef WEIGHT_CORRECT_ZERO
        if (isZero())
            return *this;
#endif
        return self_type(weight*nth,false);
    }

    // return -1 if this is less than o, 0 if equal, 1 otherwise
    int cmp(self_type const& o) const
    {
        Real diff=o.weight-weight;
        return diff < 0 ? -1 : (diff ? 1 : 0);
    }

    // -1: a<b 0: a==b 1: a>b
    inline friend int cmp(self_type const& a,self_type const& b)
    {
        return a.cmp(b);
    }

    self_type &operator ^= (Real power) { // raise self_type^power
        raisePower(power);
        return *this;
    }

    template<class charT, class Traits>
    inline static std::streamsize set_precision(std::basic_ostream<charT,Traits>& o) {
        return o.precision(sizeof(Real) > 4 ? 15 : 7);
    }

    template<class charT, class Traits>
    std::ios_base::iostate print(std::basic_ostream<charT,Traits>& o) const {
        std::streamsize old_precision=set_precision(o);
        int base=get_log_base(o);
        if ( isZero() )
            o << "0";
        else {
            int log=get_log(o);
            if ( (log == SOMETIMES_LOG && fitsInReal()) || log == NEVER_LOG ) {
                o << getReal();
            } else { // out of range or ALWAYS_LOG
                if ( base == LN) {
                    o << getLn() << "ln";
                } else if (base == LOG10) {
                    o << getLog10() << "log";
                } else {
                    o << "e^" << getLn();
                }
            }
        }
        o.precision(old_precision);
        return GENIOGOOD;
    }
    void throwbadweight()
    {
        throw "bad logweight";
    }

    bool setString(const std::string &str)
    {
        const char *beg=str.c_str(), *end=beg+str.length();
        return setStringPartial(beg,end)==end;
    }
    bool setString(const char *str)
    {
        const char *end = str+std::strlen(str);
        return setStringPartial(str,end)==end;
    }
    bool setString(const char *str, const char *end)
    {
        return setStringPartial(str,end)==end;
    }
    // reads from b up to as much as end, returning one past last read character, or NULL if error
    char *setStringPartial(const char *b, const char *end)
    {
        char *e;
        if (b+1<end && b[0]=='e' && b[1] == '^') {
            setLn(std::strtod(b+2,&e));
            return e;
        } else if (b+2<end && b[0]=='1'&&b[1]=='0'&&b[2]=='^') {
            setLog10(std::strtod(b+3,&e));
            return e;
        } else {
            double d=std::strtod(b,&e);
            if (e[0]=='l') {
                if (e[1]=='n') {
                    setLn(d);
                    return e+2;
                } else if (e[1]=='o' && e[2]=='g') {
                    setLog10(d);
                    return e+3;
                } else
                    return 0;
            } else {
                setReal(d);
                return e;
            }
        }
        return 0;
    }
    template <class O>
    void print_base(O &o,Real base) const
    {
        o << base << '^' << getLog(base);
    }


    template<class charT, class Traits>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in) {
        char c;
        double f=0;
        EXPECTI_FIRST(in >> c);
        if (c != 'e')
            in.unget();
        else {
            EXPECTCH('^');
            EXPECTI(in >> f);
            setLn((Real)f);
            return GENIOGOOD;
        }
        in >> f;
        if (f==10) {
            if (in >> c) {
                if (c=='^') {
                    EXPECTI(in>>f);
                    setLog10((Real)f);
                } else {
                    in.unget();
                    setReal(f);
                    return GENIOGOOD;
                }
            } else {
                setReal(f);
                return GENIOGOOD;
            }
        }

        if (in.eof())
            setReal(f);
        else if (in.fail())
            goto fail;
        else if ( (c = in.get()) == 'l' ) {
            char n = in.get();
            if ( n == 'n')
                setLn((Real)f);
            else if ( n == 'o' && in.get() == 'g' )
                setLog10(f);
            else {
                setZero();
                return GENIOBAD;
            }
        } else {
            in.unget();
            setReal(f);
        }
        return GENIOGOOD;
    fail:
        setZero();
        return GENIOBAD;
    }
    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ_GEN
    struct base_printer
    {
        typedef base_printer self_type;
        Real base;
        typedef logweight<Real> Weight;
        Weight w;
        base_printer(Real base,Weight const& w) : base(base),w(w) {}
//    base_printer(base_printer const& o) : base(o.base) {}
        template <class O>
        void print(O &o) const
        {
            w.print_base(o,base);
        }
        TO_OSTREAM_PRINT
    };
    base_printer as_base(Real base) const
    {
        return base_printer(base,*this);
    }

};


template<class Real>
inline Real log(logweight<Real> a) {
    return a.getLn();
}

template<class T> inline T exponential(typename T::float_type exponent) {
    T r;
    r.setLn(exponent);
    return r;
}

template<class Real>
inline logweight<Real> pow_logexponent(logweight<Real> a, logweight<Real> b) {
    a.raisePower(b.weight);
    return a;
}

template<class Real>
inline logweight<Real> root(logweight<Real> w, Real nth) {
    w.takeRoot(nth);
    return w;
}

template<class Real>
inline logweight<Real> pow(logweight<Real> w, Real nth) {
    w.raisePower(nth);
    return w;
}

template<class Real>
inline logweight<Real> operator ^(logweight<Real> const& base,Real exponent) {
    return pow(base,exponent);
}


/*
  template<class Real,class charT, class Traits>
  std::ios_base::iostate logweight<Real>::print(std::basic_ostream<charT,Traits>& o) const
  {
  }

  template<class Real,class charT, class Traits>
  std::ios_base::iostate logweight<Real>::read(std::basic_istream<charT,Traits>& in)
  {
  }
*/


/*
  template<class Real,class charT, class Traits>
  std::basic_istream<charT,Traits>&
  operator >>
  (std::basic_istream<charT,Traits>& is, logweight<Real> &arg)
  {
  return gen_extractor(is,arg);
  }

  template<class Real,class charT, class Traits>
  std::basic_ostream<charT,Traits>&
  operator <<
  (std::basic_ostream<charT,Traits>& os, const logweight<Real> &arg)
  {
  return gen_inserter(os,arg);
  }
*/
//std::ostream& operator << (std::ostream &o, logweight<Real> weight);

//std::istream& operator >> (std::istream &i, logweight<Real> &weight);

/*
  bool operator == (logweight<Real> lhs, logweight<Real> rhs);
  bool operator != (logweight<Real> lhs, logweight<Real> rhs);

//using namespace std;


logweight<Real> operator *(logweight<Real> lhs, logweight<Real> rhs);

logweight<Real> operator /(logweight<Real> lhs, logweight<Real> rhs);

logweight<Real> operator +(logweight<Real> lhs, logweight<Real> rhs);

logweight<Real> operator -(logweight<Real> lhs, logweight<Real> rhs);

bool operator <(logweight<Real> lhs, logweight<Real> rhs);
bool operator >(logweight<Real> lhs, logweight<Real> rhs);
bool operator <=(logweight<Real> lhs, logweight<Real> rhs);
bool operator >=(logweight<Real> lhs, logweight<Real> rhs);
*/

template<class Real>
inline bool operator == (logweight<Real> lhs, logweight<Real> rhs) { return lhs.weight == rhs.weight; }

template<class Real>
inline bool operator != (logweight<Real> lhs, logweight<Real> rhs) { return lhs.weight != rhs.weight; }

#define WEIGHT_DEFINE_OP(op,impl)                                                                                                               \
    template <class Real> inline logweight<Real> operator op(logweight<Real> lhs, logweight<Real> rhs) { impl; }                                \
    template <class Real,class T> inline logweight<Real> operator op(logweight<Real> lhs, const T& rhs_) { logweight<Real> rhs(rhs_); impl; }   \
    template <class T,class Real> inline logweight<Real> operator op(const T& lhs_, logweight<Real> rhs) { logweight<Real> lhs(lhs_); impl; }


#define WEIGHT_FORWARD_OP_RET(op,rettype)                                                                                                               \
    template <class Real,class T> inline rettype operator op(logweight<Real> lhs, const T& rhs_) { logweight<Real> rhs(rhs_); return lhs op rhs; }      \
    template <class T,class Real> inline rettype operator op(const T& lhs_, logweight<Real> rhs) { logweight<Real> lhs(lhs_); return lhs op rhs; }

#define WEIGHT_FORWARD_OP(op) WEIGHT_FORWARD_OP_RET(op,logweight<Real>)

WEIGHT_DEFINE_OP(*,return logweight<Real>(lhs.weight+rhs.weight,false))
    WEIGHT_DEFINE_OP(/,return logweight<Real>(lhs.weight-rhs.weight,false))

/*
//FIXME: doesn't implicitly convert!
template<class Real,class T>
inline logweight<Real> operator *(logweight<Real> lhs, logweight<Real> rhs) {
logweight<Real> result;
result.weight =  lhs.weight + rhs.weight;
return result;
}

template<class Real,class T>
inline logweight<Real> operator *(logweight<Real> lhs, const T &rhs) {
logweight<Real> result;
result.weight =  lhs.weight + logweight<Real>(rhs).weight;
return result;
}

template<class Real>
inline logweight<Real> operator /(logweight<Real> lhs, logweight<Real> rhs) {
logweight<Real> result;
result.weight =  lhs.weight - rhs.weight;
return result;
}
*/


    template<class Real>
inline logweight<Real> operator +(logweight<Real> lhs, logweight<Real> rhs) {
    //fixme: below test is needed with glibc without -ffast-math to compute 0+0 properly (?)
    //  if (lhs == 0.0)
    //  return rhs;
#ifdef WEIGHT_CORRECT_ZERO
    if (lhs.isZero())
        return rhs;
    if (rhs.isZero())
        return lhs;
#endif

    Real diff = lhs.weight - rhs.weight;
    if ( diff > MUCH_BIGGER_LN )
        return lhs;
    if ( diff < -MUCH_BIGGER_LN )
        return rhs;

    logweight<Real> result;

    //TODO: is log1p(x)=log(1+x) faster?  or just more accurate near x=0?
    if ( diff < 0 ) { // rhs is bigger
        result.weight = (Real)(rhs.weight +
#ifdef GRAEHL_USE_LOG1P
                               log1p(std::exp(diff))
#else
                               std::log(1 + std::exp(diff))
#endif
            );
        return result;
    }
    // lhs is bigger
    result.weight = (Real)( lhs.weight +
#ifdef GRAEHL_USE_LOG1P
                               log1p(std::exp(-diff))
#else
                            std::log(1 + std::exp(-diff))
#endif
        );
    return result;
}

template<class Real>
inline logweight<Real> operator -(logweight<Real> lhs, logweight<Real> rhs) {

#ifdef WEIGHT_CORRECT_ZERO
    if (rhs.isZero())
        return lhs;
#endif

    logweight<Real> result;
    Real rdiff=rhs.weight-lhs.weight;
    if ( rdiff >= 0 )        // lhs <= rhs
        // clamp to zero as minimum without giving exception (not mathematically correct!)
    {
        //result.weight = -FLOAT_INF(); // default constructed to this already
        return result;
    }


    if ( rdiff < -MUCH_BIGGER_LN ) // lhs >> rhs
        return lhs;

    // lhs > rhs
#ifdef GRAEHL_USE_LOG1P
    result.weight = (Real)(lhs.weight + log1p(-std::exp(rdiff)));
#else
    result.weight = (Real)(lhs.weight + std::log(1 - std::exp(rdiff)));
#endif
    return result;
}

WEIGHT_FORWARD_OP(+)
    WEIGHT_FORWARD_OP(-)

//FIXME: why can't second arg be double? typedef?
    template<class Real>
inline logweight<Real> absdiff(logweight<Real> lhs, logweight<Real> rhs) {
#if 0
    // UNTESTED
    Real diff=lhs.weight-rhs.weight;
    if ( diff > MUCH_BIGGER_LN )
        return lhs;
    if ( diff < -MUCH_BIGGER_LN )
        return rhs;
    logweight<Real> result;
    if ( diff < 0 )
        result.weight = (Real)(rhs.weight + log(1 - std::exp(diff)));
    else
        result.weight = (Real)(lhs.weight + log(1 - std::exp(-diff)));
#else
    if (lhs.weight > rhs.weight)
        return lhs-rhs;
    else
        return rhs-lhs;
#endif
}

template<class Real>
inline bool operator <(logweight<Real> lhs, logweight<Real> rhs) { return lhs.weight < rhs.weight; }
template<class Real>
inline bool operator >(logweight<Real> lhs, logweight<Real> rhs) { return lhs.weight > rhs.weight; }
template<class Real>
inline bool operator <=(logweight<Real> lhs, logweight<Real> rhs) { return lhs.weight <= rhs.weight; }
template<class Real>
inline bool operator >=(logweight<Real> lhs, logweight<Real> rhs) { return lhs.weight >= rhs.weight; }
/*
//FIXME: why doesn't this automatically convert?
template<class Real>
inline bool operator <(logweight<Real> lhs, double rhs) { return lhs.weight < logweight<Real>::getlogreal(rhs); }
template<class Real>
inline bool operator >(logweight<Real> lhs, double rhs) { return lhs.weight > logweight<Real>::getlogreal(rhs); }
template<class Real>
inline bool operator <=(logweight<Real> lhs, double rhs) { return lhs.weight <= logweight<Real>::getlogreal(rhs); }
template<class Real>
inline bool operator >=(logweight<Real> lhs, double rhs) { return lhs.weight >= logweight<Real>::getlogreal(rhs); }
*/
WEIGHT_FORWARD_OP_RET(<,bool)
    WEIGHT_FORWARD_OP_RET(<=,bool)
    WEIGHT_FORWARD_OP_RET(>,bool)
    WEIGHT_FORWARD_OP_RET(>=,bool)
    WEIGHT_FORWARD_OP_RET(==,bool)
    WEIGHT_FORWARD_OP_RET(!=,bool)

#ifdef _MSC_VER
#pragma warning(pop)
#endif


    template <class Real>
void inline dbgout(std::ostream &o,logweight<Real> w) {
    logweight<Real>::out_never_log(o);
    o << w;
#ifdef VERBOSE_DEBUG
    o << '=';
    logweight<Real>::out_always_log(o)
        o << w;
#endif
}


#ifdef DEBUGNAN
#define NANCHECK(w) w.NaNCheck()
#else
#define NANCHECK(w)
#endif

#ifdef WEIGHT_FLOAT_TYPE
typedef logweight<WEIGHT_FLOAT_TYPE> Weight;
#endif

#undef WEIGHT_FORWARD_OP_RET
#undef WEIGHT_FORWARD_OP
#undef WEIGHT_DEFINE_OP


#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE( TEST_WEIGHT )
{
    typedef logweight<float> W;
    W a(1),b("1"),c("e^0"),d("0ln"),e("0log");
    BOOST_CHECK(a==b);
    BOOST_CHECK(a==c);
    BOOST_CHECK(a==d);
    BOOST_CHECK(a==e);
}
#endif

} //ns

namespace std {
template<class Real>
class numeric_limits<graehl::logweight<Real> > : public std::numeric_limits<Real> {
    typedef graehl::logweight<Real> W;
 public:
    BOOST_STATIC_CONSTANT(bool,has_infinity=true);
    BOOST_STATIC_CONSTANT(bool,is_exact=false);
    BOOST_STATIC_CONSTANT(bool,is_integer=false);
    BOOST_STATIC_CONSTANT(bool,is_signed=false);
    BOOST_STATIC_CONSTANT(bool,is_modulo=false);
    static W denorm_min() { return W(graehl::zero_weight()); }
    static W min() { return W(graehl::zero_weight()); }
//    static W max() { return W(std::numeric_limits<Real>::max(),graehl::ln_weight()); }
    static W max() { return W(graehl::inf_weight()); }
    static W infinity() { return W(graehl::inf_weight()); }
    static W quiet_NaN () { return W(graehl::quiet_NaN_weight()); }
    static W signaling_NaN () { return W(graehl::signaling_NaN_weight()); }
    // BOOST_STATIC_CONSTANT(int,extra_digits_factor=10); // so boost::lcast_precision doesn't lose info.
//    BOOST_STATIC_CONSTANT(int,radix=10);
//    BOOST_STATIC_CONSTANT(int,digits=std::numeric_limits<Real>::digits10*extra_digits_factor);
//    BOOST_STATIC_CONSTANT(int,digits10=std::numeric_limits<Real>::digits10*extra_digits_factor);

    //FIXME: add rest
    /*
      namespace std {

  template <class T>
  class numeric_limits {

    public:

    // General -- meaningful for all specializations.
    static const bool is_specialized ;
    static T min () throw();
    static T max () throw();
    static const int radix ;
    static const int digits ;
    static const int digits10 ;
    static const bool is_signed ;
    static const bool is_integer ;
    static const bool is_exact ;
    static const bool traps ;
    static const bool is_modulo ;
    static const bool is_bounded ;

    // Floating point specific.
    static T epsilon () throw();
    static T round_error () throw();
    static const int min_exponent10 ;
    static const int max_exponent10 ;
    static const int min_exponent ;
    static const int max_exponent ;
    static const bool has_infinity ;
    static const bool has_quiet_NaN ;
    static const bool has_signaling_NaN ;
    static const bool is_iec559 ;
    static const float_denorm_style has_denorm ;
    static const bool has_denorm_loss;
    static const bool tinyness_before ;
    static const float_round_style round_style ;
    static T denorm_min () throw();
    static T infinity () throw();
    static T quiet_NaN () throw();
    static T signaling_NaN () throw();
  };

  enum float_round_style {
    round_indeterminate       = -1,
    round_toward_zero         =  0,
    round_to_nearest          =  1,
    round_toward_infinity     =  2,
    round_toward_neg_infinity =  3
  };
  enum float_denorm_style {
    denorm_indeterminate      = -1,
    denorm_absent             =  0,
    denorm_present            =  1
  };
}
    */
};
}


namespace boost {

}


#ifdef GRAEHL__SINGLE_MAIN
#include <graehl/shared/weight.cc>
#endif

#endif
/*
  #include <graehl/shared/semiring.hpp>
  template<nnn>
  struct semiring_traits<Weight> {
  typedef Weight value_type;
  static inline value_type exponential(Real exponent) {
  return exponential<C>(exponent);
  }
  static inline void set_one(Weight &w) { w.setOne(); }
  static inline void set_zero(Weight &w) { w.setZero(); }
  static inline bool is_one(const Weight &w) { w.isOne(); }
  static inline bool is_zero(const Weight &w) { w.isZero(); }
  static inline void addto(Weight &w,Weight p) {
  w += p;
  }

  };
*/
