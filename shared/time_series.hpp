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
#include <cstddef>
#include <cmath>
#include <cassert>
#include <functional>

namespace graehl {

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
    typedef Returns return_type;
    typedef double time_type;
    typedef time_type first_argument_type;
    value_type x0;
    value_type k;
    value_type x_origin;
    time_type t_max;
    clamped_time_series(value_type start,value_type end,// start and end should both be positive.  at t<=0, return start, at t>=duration-1, return end.  in between, depends on alpha
                        time_type duration, // varies over t in [0,duration);
                        double curvature=0)
    // curvature=0->regular exponential decay (x[n+1]=k*x[n]).  curvature->(-infty) -> (nearly) linear.  curvature>=1 -> impossible. curvature->1 -> quickly drops to end and stays nearly constant
    {
        assert(curvature<=1);
        set(start,end,duration,curvature);
    }

    // duration <= 0 -> constant series (=start)
    void set(value_type start,value_type end,time_type duration,double curvature=0)
    {
#ifdef DEBUG_TIME_SERIES
        DBP4(start,end,duration,curvature);
#endif
        if (duration <= 0) { //set constant fn
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
    value_type start() const
    {
        return x0+x_origin;
    }
    value_type end() const
    {
        return x0*k+x_origin;
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
    
    return_type operator()(time_type t) const
    {
        return static_cast<return_type>(value(t));
    }

};

} //graehl

#ifdef SAMPLE
# define TIME_SERIES_SAMPLE
#endif

#ifdef TIME_SERIES_SAMPLE
# include <iostream>
int main(int argc, char *argv[])
{
    using namespace std;
    using namespace graehl;
    double s=8,e=2,t_max=3;
    {
        
    clamped_time_series<double>
        s1(s,e,t_max,0),
        s2(s,e,t_max,.9),
        s3(s,e,t_max,-1e5);
    for (double t=0;t<=t_max+1;t+=.5) {
        cout << "t="<<t<<" "<<s1(t);
        cout <<" "<<s2(t)<<" "<<s3(t)<<endl;
    }
    }
    {
        
    clamped_time_series<double,unsigned>
        s1(s,e,t_max,0),
        s2(s,e,t_max,.9),
        s3(s,e,t_max,-1e5);
    for (double t=0;t<=t_max+1;t+=.5) {
        cout << "t="<<t<<" "<<s1(t);
        cout <<" "<<s2(t)<<" "<<s3(t)<<endl;
    }   
    
    }
    
    return 0;
}

#endif 

    

#endif
