#ifndef GRAEHL__SHARED__TIME_SERIES_HPP
#define GRAEHL__SHARED__TIME_SERIES_HPP

//FIXME: curvature does nothing if <=0 for score_t, otherwise results in +INF (bug) if positive

#ifdef SAMPLE
//# define DEBUG_TIME_SERIES
#endif

#ifdef DEBUG_TIME_SERIES
#define GRAEHL__DEBUG_PRINT
# include <graehl/shared/debugprint.hpp>
#endif
#include <boost/config.hpp>
#include <cstddef>
#include <cmath>
#include <cassert>
#include <functional>
#include <ostream>

namespace graehl {

inline double pow(double x,double y)
{
    return std::pow(x,y);  // this isn't found with using namespace std, so ...
}

/*
template <class V>
struct time_series
{
    typedef V value_type;
    typedef std::size_t time_type;
    virtual V series_value_at(time_type t)=0;
};
*/

/// Computes must support: Computes=pow(Value,double),+, -,*,/.
 /// adaptable unary function: time_type(=double) -> Returns
template <class Computes=double,class Returns=Computes>
struct clamped_time_series : public std::unary_function<double,Returns>
{
    typedef Computes value_type;
    typedef Returns result_type;
    typedef double time_type;
    typedef time_type first_argument_type;
    value_type x0;
    value_type k;
    value_type x_origin;
    time_type t_max;
//    BOOST_STATIC_CONSTANT(double,linear= -1.7976931348623157e308); // bad answers (all 0) for anything near -INF.  fast math lib failure?
    BOOST_STATIC_CONSTANT(int,linear=-100000000);
    BOOST_STATIC_CONSTANT(int,exponential=0);
    BOOST_STATIC_CONSTANT(int,constant=0);

    clamped_time_series()
    {
        set(0,0);
    }

    clamped_time_series(value_type start,value_type end,// start and end should both be positive.  at t<=0, return start, at t>=duration-1, return end.  in between, depends on alpha
                        time_type duration=constant, // varies over t in [0,duration);
                        double curvature=exponential)
    // curvature=0->regular exponential decay (x[n+1]=k*x[n]).  curvature->(-infty) -> (nearly) linear.  curvature>=1 -> impossible. curvature->1 -> quickly drops to end and stays nearly constant
    {
        assert(curvature<=1);
        /* //FIXME: disabled because decoder_filters fixing end to 2 (start at 0)
        if (false && duration && start!=end) {
            assert(start>0);
            assert(end>0);
        }
        */
        set(start,end,duration,curvature);
    }

    // duration <= 0 -> constant series (=start)
    void set(value_type start,value_type end,time_type duration=constant,double curvature=exponential)
    {
#ifdef DEBUG_TIME_SERIES
        DBP4(start,end,duration,curvature);
#endif
        if (duration <= 0 || start == end) { //set constant fn
            k=1;
            t_max=1;
            x0=start;
            x_origin=0;
            return;
        }
        t_max=duration;
        value_type xN;
        if (curvature) {
            x_origin=(end*static_cast<value_type>(curvature));
            x0=start-x_origin;
            xN=end-x_origin;
        } else {
            x_origin=0;
            x0=start;
            xN=end;
        }
        k=xN/x0;
#ifdef DEBUG_TIME_SERIES
        DBP3(x0,xN,k);
#endif
    }
    bool is_constant() const
    {
        return k==1;
    }
    value_type start() const
    {
        return x0+x_origin;
    }
    value_type end() const
    {
        return x0*k+x_origin; // because k=xN/x0, and xN=end-x_origin
    }
    value_type curvature() const
    {
        return x_origin/end();
    }

    value_type value(time_type t) const
    {
        if (t<=0)
            return start();
        else if (t>=t_max)
            return end();
        else
            return x0*pow(k,t/t_max)+x_origin;
    }

    result_type operator()(time_type t) const
    {
        return static_cast<result_type>(value(t));
    }

    void print(std::ostream &o) const
    {
        if (is_constant())
            o<<"[constant]"<<start();
        else
            o<<"[0.."<<t_max<<"]="<<start()<<".."<<end()<<";c="<<curvature();
    }

    typedef clamped_time_series<Computes,Returns> self_type;
    inline friend std::ostream & operator <<(std::ostream &o,self_type const& s)
    {
        s.print(o);
        return o;
    }
};

} //graehl

#ifdef SAMPLE
# define TIME_SERIES_SAMPLE
#endif

#ifdef TIME_SERIES_SAMPLE
# include <iostream>

using namespace graehl;

typedef clamped_time_series<double> dser;

double s=8,e=.2,t_max=3;
double linear=dser::linear;
double curves[] = {
    0,.9,linear
};

dser series[] = {
 dser(s,e,t_max,0)
};

int main(int argc, char *argv[])
{
    using namespace std;
    for (unsigned i=0;i<sizeof(curves)/sizeof(curves[0]);++i) {
        dser d(s,e,t_max,curves[i]);
        cout << d << "\n";
        for (double t=0;t<=t_max+1;t+=.5)
            cout << "t="<<t<<"\t"<<d(t)<<endl;
    }

    cout <<"\n\n";

    {

    clamped_time_series<double>
        s1(s,e,t_max,0),
        s2(s,e,t_max,.9),
        s3(s,e,t_max,linear);


    for (double t=0;t<=t_max+1;t+=.5) {
        cout << "t="<<t<<"\t"<<s1(t);
        cout <<"\t"<<s2(t)<<"\t"<<s3(t)<<endl;
    }
    }

    cout<<"\n\n";

    {

    clamped_time_series<double,unsigned>
        s1(s,e,t_max,0),
        s2(s,e,t_max,.9),
        s3(s,e,t_max,linear);
    for (double t=0;t<=t_max+1;t+=.5) {
        cout << "t="<<t<<"\t"<<s1(t);
        cout <<"\t"<<s2(t)<<"\t"<<s3(t)<<endl;
    }

    }

    return 0;
}

#endif



#endif
