#ifndef GRAEHL__SHARED__STATISTICS_HPP
#define GRAEHL__SHARED__STATISTICS_HPP

#include <graehl/shared/stream_util.hpp>

namespace graehl {

template <class T>
struct max_in_accum {
    T maximum;
    max_in_accum() : maximum() {}
    template <class F>
    void operator()(const F& t) {
        for (typename F::const_iterator i=t.begin(),e=t.end();i!=e;++i)
            if (maximum < *i)
                maximum = *i;
    }
    operator T &() { return maximum; }
    operator const T &() const { return maximum; }
};

template <class Size=size_t>
struct size_accum {
    Size N;
    Size size;
    Size max_size;
    struct ref
    {
        size_accum<Size> *p;
        template <class T>
        void operator()(const T& t) {
            (*p)(t);
        }
        ref(const size_accum<Size> &r) : p(&r) {}        
    };
    
        
    size_accum() { reset(); }
    void reset() 
    {
        N=size=max_size=0;
    }    
    template <class T>
    void operator()(const T& t) {
        ++N;
        Size tsize=t.size();
        size += tsize;
        if (max_size < tsize)
            max_size = tsize;
    }
    Size total() const
    {
        return size;
    }
    Size maximum() const
    {
        return max_size;
    }
    double average() const
    {
        return (double)size/(double)N;
    }
    operator Size() const { return total(); }
};


/*
      template <class T>
    struct max_accum {
        T m;
        max_accum() : m() {}
        template <class T2>
        void operator()(const T2& t) {
            if (m < t)
                m = t;
        }
        operator T &() { return m; }
    };
*/
template <class T>
struct max_accum {
    T maximum;
    max_accum() : maximum() {}
    template <class F>
    void operator()(const F& t) {
        if (maximum < t)
            maximum = t;
    }
    operator T &() { return maximum; }
    operator const T &() const { return maximum; }
};

template <class T>
struct min_max_accum {
    T maximum;
    T minimum;
    bool seen;
    min_max_accum() : seen(false) {}
    template <class F>
    void operator()(const F& t) {
//        DBP3(minimum,maximum,t);
        if (seen) {
            if (maximum < t)
                maximum = t;
            else if (minimum > t)
                minimum = t;
        } else {
            minimum=maximum=t;
            seen=true;
        }
//        DBP2(minimum,maximum);
    }
    bool anyseen() const 
    {
        return seen;
    }
    T maxdiff() const
    {
        return this->anyseen()?maximum-minimum:T();
    }    
};

//(unbiased) sample variance
template <class T,class U>
inline T variance(T sumsq, T sum, U N) 
{
    if (N<2)
        return 0;
    T mean=sum/N;
    T diff=(sumsq-sum*mean);
    return diff > 0 ? diff/(N-1) : 0;
}

template <class T,class U>
inline T stddev(T sumsq, T sum, U N) 
{
    using std::sqrt;
    return sqrt(variance(sumsq,sum,N));    
}

template <class T,class U>
inline T stderror(T sumsq, T sum, U N) 
{
    using std::sqrt;
    if (N<2)
        return 0;
    return stddev(sumsq,sum,N)/sqrt(N);
}


template <class T>
struct avg_accum {
    T sum;
    size_t N;
    avg_accum() : N(0),sum() {}
    template <class F>
    void operator()(const F& t) {
        ++N;
        sum+=t;
    }
    bool anyseen() const
    {
        return N;
    }
    T avg() const
    {
        return sum/(double)N;
    }
    operator T() const 
    {
        return avg();
    }
    void operator +=(const avg_accum &o)
    {
        sum+=o.sum;
        N+=o.N;
    }    
};

template <class T>
struct stddev_accum {
    T sum;
    T sumsq;
    size_t N;
    stddev_accum() : N(0),sum(),sumsq() {}
    bool anyseen() const
    {
        return N;
    }
    T avg() const
    {
        return sum/(double)N;
    }
    T variance() const 
    {
        return graehl::variance(sumsq,sum,N);
    }
    T stddev() const
    {
        return graehl::stddev(sumsq,sum,N);
    }
    T stderror() const
    {
        return graehl::stderror(sumsq,sum,N);
    }
    operator T() const 
    {
        return avg();
    }
    void operator +=(const stddev_accum &o)
    {
        sum+=o.sum;
        sumsq+=o.sumsq;
        N+=o.N;
    }
    template <class F>
    void operator()(const F& t) {
        ++N;
        sum+=t;
        sumsq+=t*t;
    }
};

template <class T>
struct stat_accum : public stddev_accum<T>,min_max_accum<T> {
    template <class F>
    void operator()(const F& t) {
        stddev_accum<T>::operator()(t); //        ((Avg &)*this)(t);
        min_max_accum<T>::operator()(t);
    }
    operator T() const 
    {
        return this->maxdiff();
    }
    typedef stat_accum<T> self_type;
    template <class O> void print(O&o) const
    {
        if (this->anyseen())
            return o <<"{{{"<<this->minimum<<'/'<<this->avg()<<"(~"<<this->stddev()<<")/"<<this->maximum<<"}}}";
        else
            return o <<"<<<?/?/?>>>";
    }
    TO_OSTREAM_PRINT
};

}


#endif
