// misc. helpful template functions (TODO: categorize them)
#ifndef FUNCS_HPP
#define FUNCS_HPP

#include "memleak.hpp"

#include <iterator>

#include <cmath>
#include <string>
#include <boost/lexical_cast.hpp>

#include <boost/iterator/iterator_adaptor.hpp>
#include <stdexcept>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

#include "debugprint.hpp"

#include "myassert.h"
//#include "os.hpp"

#include <functional>
#include <algorithm>
#include <iostream>
#include <cstddef>

#include <vector>

#include "size_mega.hpp" //FIXME: remove (fixing users)

//#define TOKENIZE_KEY_VAL_DEBUG

#ifdef TOKENIZE_KEY_VAL_DEBUG
# define TOKENIZE_KEY_VAL_IF_DBG(a) a
#else
# define TOKENIZE_KEY_VAL_IF_DBG(a)
#endif

template <class V>
struct difference_f 
{
    V operator()(const V&l,const V&r) const
    {
        return l-r;
    }    
};

template <class V,class S,class F>
inline S transform2_array_coerce(const S&l,const S&r,F f) 
{
    const unsigned N=sizeof(S)/sizeof(V);
    S ret;
    const V *pl=(V*)&l;
    const V *pr=(V*)&r;
    V *pret=(V*)&ret;
    for (unsigned i=0;i<N;++i)
        pret[i]=f(pl[i],pr[i]);
    return ret;
}

template <class F>
void tokenize_key_val_pairs(const std::string &s, F &f,char pair_sep=',',char key_val_sep=':') 
{
    typedef typename F::key_type Key;
    typedef typename F::data_type Data;
    using namespace std;    
    typedef pair<Key,Data> Component;
    typedef pair<Key,Data> Component;
    typedef string::size_type Pos;
    typedef string::const_iterator It;
    Component to_add;
    for (It i=s.begin(),e=s.end();;) {
        for(It key_beg=i; ;++i) {
            if (i==e) return;
            TOKENIZE_KEY_VAL_IF_DBG(DBP2(*i,i-s.begin()););
            if (*i == key_val_sep) { // [last,i) is key
                TOKENIZE_KEY_VAL_IF_DBG(DBPC2("key",string(key_beg,i)););
                string_into(string(key_beg,i),to_add.first);
                break; // done key, expect val
            }
        }
        for (It val_beg=++i; ;++i) {
            TOKENIZE_KEY_VAL_IF_DBG(
                if (i==e) DBPC2("<END>",i-s.begin());                    
                DBP2(*i,i-s.begin());
                );
            if (i == e || *i == pair_sep) {
                TOKENIZE_KEY_VAL_IF_DBG(DBPC2("val",string(val_beg,i)););
                string_into(string(val_beg,i),to_add.second);
                f(to_add);
                if (i==e) return;
                ++i;
                break; // next key/val
            }
        }
    }
}


template <class T1, class T2>
T1 at_least(const T1 &val,const T2 &ensure_min) 
{
    return (val < ensure_min) ? ensure_min : val;
}

template <class T1, class T2>
T1 at_most(const T1 &val,const T2 &ensure_max) 
{
    return (val > ensure_max) ? ensure_max : val;
}


template <class Set>
struct not_in_set 
{
    const Set *set;
    not_in_set(const Set *set_) : set(set_) {}
    not_in_set(const not_in_set<Set> &o) : set(o.set) {}
    typedef bool result_type;
    template <class V>
    bool operator()(const V& v) const
    {
        return set->find(v)==set->end();
    }
};

template <class Set>
not_in_set<Set> make_not_in_set(Set *s) 
{
    return not_in_set<Set>(s);
}

template <class Container,class Predicate>
void remove_if(Container &cont,Predicate pred)
{
    cont.erase(std::remove_if(cont.begin(),cont.end(),pred),cont.end());
}

template <class Container,class Predicate>
void remove_if_stable(Container &cont,Predicate pred)
{
    cont.erase(std::remove_copy_if(cont.begin(),cont.end(),pred),cont.end());
}

template <class Container> inline
void trim(Container &cont,std::size_t size)
{
    if (size < cont.size())
        cont.resize(size); //        cont.erase(cont.begin()+size,cont.end());

}

template <class Container> inline
void grow(Container &cont,std::size_t size,const typename Container::value_type &default_value=typename Container::value_type())
{
    std::size_t contsz=cont.size();    
    if (size > contsz)
        cont.resize(size,default_value); //       cont.insert(cont.end()+size,size-contsz,default_value);
}

template <class Container,class Iter> inline
void append(Container &cont,Iter beg,Iter end) 
{
    cont.insert(cont.end(),beg,end);
}

template <class Container> inline
typename Container::value_type &at_expand(Container &vec,std::size_t index,const typename Container::value_type &default_value=typename Container::value_type()) 
{
    std::size_t sz=vec.size();
    if (index>=sz)
        vec.resize(index+1,default_value); //     vec.insert(vec.end(),(index-sz)+1,default_value);
    return vec[index];
}

template <class Container> inline
void maybe_decrease_min_at(Container &vec,unsigned index,const typename Container::value_type &decrease_to,const typename Container::value_type &infinity=HUGE_VAL) 
{
    typename Container::value_type &f=at_expand(vec,index,infinity);
    if (f > decrease_to)
        f = decrease_to;
}

#define FUNCTION_OBJ_X(name,return_type,expr)        \
struct name \
{ \
    typedef return_type result_type; \
    template <class T1> \
    result_type operator()(const T1 &x) const \
    { \
        return expr; \
    } \
}


#define FUNCTION_OBJ_X_Y(name,return_type,expr)        \
struct name \
{ \
    typedef return_type result_type; \
    template <class T1,class T2> \
    result_type operator()(const T1 &x,const T2 &y) const \
    { \
        return expr; \
    } \
}

#define PREDICATE_OBJ_X_Y(name,expr) FUNCTION_OBJ_X_Y(name,bool,expr)

PREDICATE_OBJ_X_Y(less_typeless,x<y);
PREDICATE_OBJ_X_Y(greater_typeless,x>y);
PREDICATE_OBJ_X_Y(equal_typeless,x==y);

// Order e.g. less_typeless
template <class Cont,class Order> inline
void sort(Cont &cont,Order order) 
{
    std::sort(cont.begin(),cont.end(),order);
}

template <class Cont,class Order> inline
void unique(Cont &cont,Order order)
{
    cont.erase(
        std::unique(cont.begin(),cont.end(),order),
        cont.end());
}
        
template <class Cont,class Order> inline
void sort_unique(Cont &cont,Order order) 
{
    sort(cont,order);
    unique(cont,order);
}

template <class Cont> inline
void sort(Cont &cont) 
{
    std::sort(cont.begin(),cont.end());
}

template <class Cont,class Func> inline
void for_each(Cont &cont,Func func)
{
    std::for_each(cont.begin(),cont.end(),func);
}

template <class Cont> inline
void unique(Cont &cont)
{
    cont.erase(
        std::unique(cont.begin(),cont.end()),
        cont.end());
}
        
template <class Cont> inline
void sort_unique(Cont &cont) 
{
    sort(cont);
    unique(cont);
}


template <class pointed_to>
void delete_now(std::auto_ptr<pointed_to> &p) {
//    std::auto_ptr<pointed_to> take_ownership_and_kill(p);
        delete p.release();
}

template <class Cont>
struct push_backer
{
    Cont *cont;
    typedef void result_type;
    typedef typename Cont::value_type argument_type;
    push_backer(Cont &container) : cont(&container) {}
    template <class V>
    void operator()(const V&v) const
    {
        cont->push_back(v);
    }
    void operator()() const
    {
        cont->push_back(argument_type());
    }
};

struct delete_anything
{
    template <class P>
    void operator()(P *p) 
    {
        delete p;
    }    
};

template <class Cont> inline
push_backer<Cont> make_push_backer(Cont &container) 
{
    return push_backer<Cont>(container);
}

// returns true and writes pos,n for substring between left-right brackets.  or false if brackets not found.
template <class Str,class size_type> inline
bool
substring_inside_pos_n(const Str &s,const Str &leftbracket,const Str &rightbracket,size_type &pos,size_type &n)
{
    size_type rightpos;
    const size_type npos=Str::npos;
    if (npos==(pos=s.find(leftbracket,0))) return false;
    pos+=leftbracket.length();
    if (npos==(rightpos=s.find(rightbracket,pos))) return false;
    n=rightpos-pos;
    return true;
}

// first is first substring (left->right) between leftbracket and rightbracket in s.
// second is true if found, false if none found
template <class Str> inline
std::pair <Str,bool>
substring_inside(const Str &s,const Str &leftbracket,const Str &rightbracket)
{
    typedef std::pair <Str,bool> Ret;
    typename Str::size_type pos,n;
    if (substring_inside_pos_n(s,leftbracket,rightbracket,pos,n))
        return Ret(Str(s,pos,n),true);
    else
        return Ret(Str(),false);
}

// parse both streams as a sequence of ParseAs, comparing for equality
template <class ParseAs,class Istream> inline
bool
equal_streams_as_seq(Istream &i1,Istream &i2)
{
    /* could almost write as istream_iterator<ParseAs>, std::equal - except that
     doesn't check both iterators' end
    */
    ParseAs v1,v2;
    for (;;) {
        bool got1=i1>>v1;
        bool got2=i2>>v2;
        if (got1) {
            if (!got2) return false; //2 ended first
        } else {
            if (!got2) return true; // both ended together
            return false; // 1 ended first
        }
        if (!(v1==v2)) return false; // value mismatch
    }
    //unreachable!
    assert(0);
}

template <class ParseAs,class Ch,class Tr> inline
bool
equal_strings_as_seq(const std::basic_string<Ch,Tr> &s1,const std::basic_string<Ch,Tr> &s2)
{
    std::basic_stringstream<Ch,Tr> i1(s1),i2(s2);
    return equal_streams_as_seq<ParseAs>(i1,i2);
}

//std::equal can only be called if sequences are same length!
template <class I1,class I2,class Equal> inline
bool equal_safe(I1 b1,I1 e1,I2 b2,I2 e2,Equal eq)
{
    while (b1 != e1) {
        if (b2 == e2) return false;
        if (*b2++ != *e2++)
            return false;
    }
    // now b1 == e1
    return b2==e2;
}

template <class I1,class I2> inline
bool equal_safe(I1 b1,I1 e1,I2 b2,I2 e2)
{
    return equal_safe(b1,e1,b2,e2,equal_typeless());
}

//oops: didn't notice that I'd already implemented this before starts_with.  two implementations for testing anyway ;)
template <class Istr, class Isubstr> inline
bool match_begin(Istr bstr,Istr estr,Isubstr bsub,Isubstr esub) 
{
    while (bsub != esub) {
        if (bstr == estr)
            return false;
        if (*bsub++ != *bstr++)
            return false;
    }
    return true;
}

template <class Istr, class Isubstr> inline
bool match_end(Istr bstr,Istr estr,Isubstr bsub,Isubstr esub) 
{
    while (bsub != esub) {
        if (bstr == estr)
            return false;
        if (*--esub != *--estr)
            return false;
    }
    return true;
}

template <class It1,class It2,class Pred> inline
bool starts_with(It1 str,It1 str_end,It2 prefix,It2 prefix_end,Pred equals)
{
    for(;;) {
        if (prefix==prefix_end) return true;
        if (str==str_end) return false;
        if (!equals(*prefix,*str)) return false;
        ++prefix;++str;
    }
    //unreachable
    assert(0);
}

template <class It1,class It2> inline
bool starts_with(It1 str,It1 str_end,It2 prefix,It2 prefix_end)
{
    return starts_with(str,str_end,prefix,prefix_end,equal_typeless());
}


//FIXME: provide skip-first-whitespace or skip-no-whitespace iterators.
template <class Ch,class Tr,class CharIt> inline
bool expect_consuming(std::basic_istream<Ch,Tr> &i,CharIt begin,CharIt end) 
{
    typedef std::istream_iterator<Ch> II;
    II ibeg(i),iend;
    return match_begin(ibeg,iend,begin,end);
}

template <class Ch,class Tr,class CharIt> inline
bool expect_consuming(std::basic_istream<Ch,Tr> &i,CharIt begin,CharIt end,bool skip_first_ws=true) 
{
    if (begin==end) return true;
    Ch c;
    if (skip_first_ws)
        i>>c;
    else
        i.get(c);
    if (!i) return false;
    while (begin!=end) {
        if (!i.get(c))
            return false;
        if (c!=*begin)
            return false;
    }
    return true;  
/* //NOTE: whitespace will be ignored!  so don't include space in expectation ...    
    typedef std::istream_iterator<Ch> II;
    II ibeg(i),iend;
    return match_begin(ibeg,iend,begin,end);
*/
}

template <class Ch,class Tr,class Str> inline
bool expect_consuming(std::basic_istream<Ch,Tr> &i,const Str &str,bool skip_first_ws=true)
{
    return expect_consuming(i,str.begin(),str.end(),skip_first_ws);
}


template <class Str>
inline
bool starts_with(const Str &str,const Str &prefix) 
{
    return starts_with(str.begin(),str.end(),prefix.begin(),prefix.end());
}

template <class Str>
inline
bool ends_with(const Str &str,const Str &suffix) 
{
//        return starts_with(str.rbegin(),str.rend(),suffix.rbegin(),suffix.rend());
        return match_end(str.begin(),str.end(),suffix.begin(),suffix.end());

}

template <class Str>
inline
bool starts_with(const Str &str,char *prefix) 
{
    return starts_with(str,std::string(prefix));
}

template <class Str>
inline
bool ends_with(const Str &str,char *suffix) 
{
    return ends_with(str,std::string(suffix));
}


#if 0
inline
bool starts_with(const std::string &str,const std::string &prefix) 
{
    return match_begin(str.begin(),str.end(),prefix.begin(),prefix.end());
//    return (str.find(prefix)==0);
}

inline
bool ends_with(const std::string &str,const std::string &suffix) 
{
    return match_end(str.begin(),str.end(),suffix.begin(),suffix.end());
//        return starts_with(str.rbegin(),str.rend(),suffix.rbegin(),suffix.rend());
/*
  const std::string::size_type slen=str.length();
    return (str.rfind(suffix)==len-suffix.length());
*/    
}
#endif 

template <class Str,class Data> inline
void string_into(const Str &str,Data &data) 
{
    std::istringstream i(str);
    if (!(i>>data))
        throw std::runtime_error("Couldn't convert (string_into): "+str);
}


template <class Data,class Str,class size_type> inline
Data string_to(const Str &str)
{
    Data ret;
    string_into(str,ret);
    return ret;
}
/*

template <class Str,class Data,class size_type> inline
void substring_into(const Str &str,size_type pos,size_type n,Data &data) 
{
//    std::istringstream i(str,pos,n); // doesn't exist!
    std::istringstream i(str.substr(pos,n));
    if (!(i>>*data))
        throw std::runtime_error("Couldn't convert (string_into): "+str);
}

template <class Data,class Str,class size_type> inline
Data string_to(const Str &str,size_type pos,size_type n)
{
    Data ret;
    substring_into(str,pos,n,ret);
    return ret;
}

*/

template <class Str> inline
void erase_begin(Str &s,unsigned n) 
{
    s.erase(0,n);
}

template <class Str> inline
void erase_end(Str &s,unsigned n) 
{
    s.erase(s.length()-n,n);
}

// func(const Func::argument_type &val) - assumes val can be parsed from string tokenization (no whitespace)
template <class In,class Func> inline
void parse_until(const std::string &term,In &in,Func func)
{
    std::string s;
    bool last=false;
    while(!last && (in>>s) ) {
        if (!term.empty() && ends_with(s,term)) {
            last=true;
            erase_end(s,term.length());
        }
        if (s.empty())
            break;
        typename Func::argument_type val;
        string_into(s,val);
        func(val);
    }
};

template <class In,class Cont> inline
void push_back_until(const std::string &term,In &in,Cont & cont) 
{
    parse_until(term,in,make_push_backer(cont));
}


// note: google groups on "vector clear deallocate" - you'll see that clear() doesn't free up memory - not so helpful when you hit an OOM exception and want to use a singleton vector instead
template <class Container> inline
void reconstruct(Container &v,size_t n,const typename Container::value_type &val)
{
    /*
    v.clear();
    v.insert(v.end(),n,val);
    */
    v.~Container();
    new(&v)Container(n,val);
}

template <class Container> inline
void reconstruct(Container &v,size_t n)
{
    /*
    v.clear();
    v.insert(v.end(),n,val);
    */
    v.~Container();
    new(&v)Container(n);
}

template <class V>
void reconstruct(V &v) {
    v.~V();
    new (&v)V();
}


template <class Ck,class Cv,class Cr> inline
void zip_lists_to_pairlist(const Ck &K,const Cv &V,Cr &result)
{
    result.clear();
    typename Ck::const_iterator ik=K.begin(),ek=K.end();
    typename Cv::const_iterator iv=V.begin(),ev=V.end();
    for(;ik<ek && iv<ev;++ik,++iv)
        result.push_back(typename Cr::value_type(*ik,*iv));    
    assert(ik==ek && iv==ev);
}

template <class Ck,class Cv,class Cr> inline
std::vector<std::pair<typename Ck::value_type,typename Cv::value_type> > zip_lists(const Ck &K,const Cv &V)
{
    std::vector<std::pair<typename Ck::value_type,typename Cv::value_type> > result;
    zip_lists_to_pairlist(K,V,result);
    return result;
}

template <bool descending=false>
struct compare_fabs 
{
    template <class T1,class T2>
    bool operator()(const T1&a,const T2&b) const 
    {
        return (descending) ?
            fabs(b) < fabs(a)
            :
            fabs(a) < fabs(b);
    }    
};

template <class compare=compare_fabs<true> >
struct select1st_compare : public compare
{
    template <class T1,class T2>
    bool operator()(const T1&a,const T2&b) const 
    {
        return compare::operator()(a.first,b.first);
    }
};

template <class T,class Comp>
struct compare_indirect_array : public Comp
{
    const T *base;
    typedef compare_indirect_array<T,Comp> Self;
    compare_indirect_array(T *base_=NULL,const Comp &comp=Comp()) : base(base_),Comp(comp) {}
    compare_indirect_array(const Self &s) : base(s.base),Comp(s) {}
    template <class Vec,class VecTo>
    compare_indirect_array(const Vec &from,VecTo &to,const Comp &comp=Comp()) : Comp(comp) 
    {
        init(from,to);
    }
    template <class Vec,class VecTo>
    void init(const Vec &from,VecTo &to)
    {
        base=&*from.begin();
        to.clear();
        for (unsigned i=0,e=from.size();i<e;++i)
            to.push_back(i);
    }    
    bool operator()(unsigned a,unsigned b) const
    {
        return Comp::operator()(base[a],base[b]);
    }
};

    
template <class FloatVec> inline
typename FloatVec::value_type
norm(const FloatVec &vec) 
{
    typedef typename FloatVec::const_iterator Iter;
    typedef typename FloatVec::value_type Float;
    Float sumsq=0;
    for (Iter i=vec.begin(),e=vec.end();i!=e;++i) {
        Float v=*i;
        sumsq+=v*v;        
    }
    return sqrt(sumsq);
}

template <class FloatVec,class Scalar> inline
void
scale(FloatVec &vec,const Scalar &scale) 
{
    typedef typename FloatVec::iterator Iter;
    for (Iter i=vec.begin(),e=vec.end();i!=e;++i)
        *i*=scale;
}

template <class FloatVec,class Scalar> inline
void
unscale(FloatVec &vec,const Scalar &scale) 
{
    typedef typename FloatVec::iterator Iter;
    for (Iter i=vec.begin(),e=vec.end();i!=e;++i)
        *i/=scale;
}

template <class FloatVec> inline
typename FloatVec::value_type
normalize(FloatVec &vec) 
{
    typedef typename FloatVec::iterator Iter;
    typedef typename FloatVec::value_type Float;
    Float n=norm(vec);
    if (n > 0)
        unscale(vec,n);
    return n;
}

template <class FloatVec,class V2> inline
typename FloatVec::value_type
norm_of_inner_product(const FloatVec &vec,const V2 &v2)
{
    typedef typename FloatVec::const_iterator Iter;
    typedef typename V2::const_iterator Iter2;
    typedef typename FloatVec::value_type Float;
    Float sumsq=0;
    Iter2 i2=v2.begin();
    assert(vec.size()==v2.size());
    for (Iter i=vec.begin(),e=vec.end();i!=e;++i,++i2) {
        Float v=*i * *i2;
        sumsq+=v*v;   
    }
    return sqrt(sumsq);
}

template <class V1,class V2,class Vout> inline
void inner_product(const V1 &v1,const V2 &v2,Vout &vout) 
{
    vout.clear();
    vout.reserve(v1.size());
    assert(v1.size()==v2.size());
    typename V1::const_iterator iv1=v1.begin(),ev1=v1.end();
    typename V2::const_iterator iv2=v2.begin();//,ev=V.end();    
    for(;iv1<ev1;++iv1,++iv2)
        vout.push_back(*iv1 * *iv2);
}

template <class V1,class V2,class Vout> inline
std::vector<typename V1::value_type> inner_product(const V1 &v1,const V2 &v2) 
{
    std::vector<typename V1::value_type> ret;
    inner_product(v1,v2,ret);
    return ret;
}

template <class C> inline
void resize_up_for_index(C &c,size_t i) 
{
    const size_t newsize=i+1;
    if (newsize > c.size())
        c.resize(newsize);
}

// note: not finished - see boost::iterator_facade
template <class P>
struct null_terminated_iterator
{
    P *me;    
};

    

// I hope you have an efficient swap :)
template <class Container> inline
void compact(Container &c) {
    Container(c).swap(c); // copy (-> temp), swap, temp destructed = old destructed
}

template <class c,class t,class a> inline
void compact(std::basic_string<c,t,a> &s) {
    s.reserve(0); // suggested by std to shrink size to actual amount used.
}

#include <string>

inline std::string subspan(const std::string &s,std::string::size_type begin,std::string::size_type end) 
{
    return std::string(s,begin,end-begin);
}

template <typename Container> inline
void stringtok (Container &container, std::string const &in, const char * const delimiters = " \t\n")
{
    const std::string::size_type len = in.length();
    std::string::size_type i = 0;
    while ( i < len )
    {
        // eat leading whitespace
        i = in.find_first_not_of (delimiters, i);
        if (i == std::string::npos)
            return;

        // find the end of the token
        std::string::size_type j = in.find_first_of (delimiters, i);

        // push token
        if (j == std::string::npos) {
            container.push_back (in.substr(i));
            return;
        } else
            container.push_back (in.substr(i, j-i));
        i = j + 1;
    }
}

// requires Val::operator += as well as copy ctor.  TODO: version that takes InPlaceFold functor.
template <class AssocContainer,class Key,class Val>
inline void accumulate(AssocContainer *table,const Key &key,const Val &val) {
    std::pair<typename AssocContainer::iterator,bool> was_inserted=table->insert(typename AssocContainer::value_type(key,val));
    if (!was_inserted.second) {
        typename AssocContainer::value_type::second_type &old_val=was_inserted.first->second;
        old_val += val;
    }
}

template <class AssocContainer,class Key,class Val>
inline void maybe_decrease_min(AssocContainer *table,const Key &key,const Val &val) {
    std::pair<typename AssocContainer::iterator,bool> was_inserted=table->insert(typename AssocContainer::value_type(key,val));
    if (!was_inserted.second) {
        typename AssocContainer::value_type::second_type &old_val=was_inserted.first->second;
//        INFOL(29,"maybe_decrease_min",key << " val=" << val << " oldval=" << old_val);
        if (val < old_val)
            old_val=val;
    } else {
//                INFOL(30,"maybe_decrease_min",key << " val=" << val);
    }
    //    return was_inserted->first->second;
}

template <class AssocContainer,class Key,class Val>
inline void maybe_increase_max(AssocContainer *table,const Key &key,const Val &val) {
    std::pair<typename AssocContainer::iterator,bool> was_inserted=table->insert(typename AssocContainer::value_type(key,val));
    if (!was_inserted.second) {
        typename AssocContainer::value_type::second_type &old_val=was_inserted.first->second;
//        INFOL(29,"maybe_increase_max",key << " val=" << val << " oldval=" << old_val);
        //!FIXME: no idea how to get this << to use ns_Syntax::operator <<
        if (old_val < val)
            old_val=val;
    } else {
//                INFOL(30,"maybe_increase_max",key << " val=" << val);
    }
//    return was_inserted->first->second;
    
}

template <class To,class From>
inline void maybe_increase_max(To &to,const From &from) {
    if (to < from)
        to=from;
}

template <class To,class From>
inline void maybe_decrease_min(To &to,const From &from) {
    if (from < to)
        to=from;
}

#ifndef ONE_PLUS_EPSILON
# ifndef FLOAT_EPSILON
#  define FLOAT_EPSILON .00001
# endif
# ifndef EPSILON
static const double EPSILON=FLOAT_EPSILON;
# endif 
static const double ONE_PLUS_EPSILON=1+EPSILON;
#endif

//#define ONE_PLUS_EPSILON (1+EPSILON)


/*
  The simple solution like abs(f1-f2) <= e does not work for very small or very big values. This floating-point comparison algorithm is based on the more confident solution presented by Knuth in [1]. For a given floating point values u and v and a tolerance e:

| u - v | <= e * |u| and | u - v | <= e * |v|
defines a "very close with tolerance e" relationship between u and v
        (1)

| u - v | <= e * |u| or   | u - v | <= e * |v|
defines a "close enough with tolerance e" relationship between u and v
        (2)

Both relationships are commutative but are not transitive. The relationship defined by inequations (1) is stronger that the relationship defined by inequations (2) (i.e. (1) => (2) ). Because of the multiplication in the right side of inequations, that could cause an unwanted underflow condition, the implementation is using modified version of the inequations (1) and (2) where all underflow, overflow conditions could be guarded safely:

| u - v | / |u| <= e and | u - v | / |v| <= e
| u - v | / |u| <= e or   | u - v | / |v| <= e
        (1`)
(2`)
*/


  //intent: if you want to be conservative about an assert of a<b, test a<(slightly smaller b)
  // if you want a<=b to succeed when a is == b but there were rounding errors so that a+epsilon=b, test a<(slightly larger b)
template <class Float>
inline Float slightly_larger(Float target) {
    return target * ONE_PLUS_EPSILON;
}

template <class Float>
inline Float slightly_smaller(Float target) {
    return target * (1. / ONE_PLUS_EPSILON);
}

// note, more meaningful tests exist for values near 0, see Knuth
// (but for logs, near 0 should be absolute-compared)
inline bool same_within_abs_epsilon(double a,double b,double epsilon=1e-4) {
    return fabs(a-b) < epsilon;
}

// requirement: P::return_type value semantics, default initializes to boolean false (operator !), and P itself copyable (value)
// can then pass finder<P>(P()) to enumerate() just like find(beg,end,P())
template <class P>
struct finder
{
    typedef typename P::return_type return_type;
    typedef finder<P> Self;

    P pred;
    return_type ret;

    finder(const P &pred_): pred(pred_),ret() {}
    finder(const Self &o) : pred(o.pred),ret() {}

    void operator()(const return_type& u)
    {
        if (!ret)
            ret = pred(u);
    }

};

template <class P>
finder<P> make_finder(const P& pred)
{
    return finder<P>(pred);
}


#define INDIRECT_STRUCT_COMMON(type,method) \
    struct indirect_ ## type ## _ ## method                  \
    {                                         \
        typedef type indirect_type;           \
        indirect_ ## type ## _ ## method *pimp;              \
        indirect_ ## type ## _ ## method(type &t) : pimp(&t) {} \
        indirect_ ## type ## _ ## method(const indirect_ ## type ## _ ## method &t) : pimp(t.pimp) {} \

#define INDIRECT_STRUCT(type,method)                         \
    INDIRECT_STRUCT_COMMON(type,method) \
            void operator()() const         \
            {                                   \
                pimp->method();               \
            }                                   \
    };

#define INDIRECT_STRUCT1(type,method,t1,v1)      \
    INDIRECT_STRUCT_COMMON(type,method) \
            void operator()(t1 v1) const         \
            {                                   \
                pimp->method(v1);               \
            }                                   \
    };


#define INDIRECT_STRUCT1(type,method,t1,v1)      \
    INDIRECT_STRUCT_COMMON(type,method) \
            void operator()(t1 v1) const         \
            {                                   \
                pimp->method(v1);               \
            }                                   \
    };



#define INDIRECT_PROTO(type,method) INDIRECT_STRUCT(type,method)    \
        void method()

#define INDIRECT_PROTO1(type,method,t1,v1) INDIRECT_STRUCT1(type,method,t1,v1)    \
        void method(t1 v1)

#define INDIRECT_PROTO2(type,method,t1,v1,t2,v2) INDIRECT_STRUCT2(type,method,t2,v2)   \
        void method(t1 v1,t2 v2)

/*
template <typename T,size_t n>
struct fixed_single_allocator {
//    enum make_not_anon_13 { size = n };
    T space[n];
    T *allocate(size_t _n) {
        Assert(n==_n);
        return space;
    }
    void deallocate(T* tp,size_t _n) {
        Assert(n==_n && tp == space);
    }
};
*/

template <class T>
struct bounded_iterator : public boost::iterator_adaptor<bounded_iterator<T>,T> {
private:
    struct enabler {};
    typedef boost::iterator_adaptor<bounded_iterator<T>,T> Super;
    friend class boost::iterator_core_access;
    void increment() { if (this->base() == end) throw std::out_of_range("bounded iterator incremented past end"); ++this->base_reference(); }
    T end;
public:
    bounded_iterator(const T& begin,const T &end_) : Super(begin), end(end_) {}
/*    template <class O>
    bounded_iterator(bounded_iter<O> const& other, typename boost::enable_if<boost::is_convertible<O*,T*>, enabler>::type = enabler())
    ) : Super(other.base()) {}*/
    bounded_iterator(bounded_iterator<T> const &other) : Super(other),end(other.end) {}
};

namespace nonstd {

// bleh, std::construct also was nonstandard and removed
template <class P,class V>
void construct(P *to,const V& from)
{
  PLACEMENT_NEW(to) P(from);
}

// uninitialized_copy_n was removed from std:: - we don't reuse the name because you might use namespace std; in an old compiler
// plus our version doesn't return the updated iterators
template <class I,class F>
void uninitialized_copy_n(I from, unsigned n, F to)
{
  for ( ; n > 0 ; --n, ++from, ++to)
    //PLACEMENT_NEW(&*to) iterator_traits<F>::value_type(*from);
    construct(&*to,*from);
}
};

struct dummy_nullary_func {
    typedef void result_type;
    void operator()() {}
};

struct tick_writer
{
    typedef void result_type;
    std::ostream *pout;
    std::string tick;
    tick_writer() {}
    tick_writer(const tick_writer &t) : pout(t.pout),tick(t.tick) {}
    tick_writer(std::ostream *o_,const std::string &tick_=".") { init(o_,tick_); }
    void init(std::ostream *o_,const std::string &tick_=".")
    {
        pout=o_;
        tick=tick_;
    }
    void operator()()
    {
        if (pout)
            *pout << tick;
    }
    template <class C>
    void operator()(const C& unused)
    {
        return operator()();
    }
};


struct Periodic {
    unsigned period,left;
    bool enabled;
    Periodic(unsigned period_) {
        set_period(period_);
    }
    void enable() {
        enabled=true;
    }
    void disable() {
        enabled=false;
    }
    void set_period(unsigned period_=0) {
        enabled=true;
        period=period_;
        trigger_reset();
    }
    void trigger_reset() {
        left=period;
    }
    void trigger_next() {
        left=0;
    }
    bool check() {
        DBP2(period,left);
        if (period && enabled) {
            if (!--left) {
                left=period;
                return true;
            }
        }
        return false;
    }
};

template <class C>
struct periodic_wrapper : public C
{
    typedef C Imp;
    typedef typename Imp::result_type result_type;
    Periodic period;
    periodic_wrapper(unsigned period_=0,const Imp &imp_=Imp()) : period(period_), Imp(imp_) {}
    void set_period(unsigned period_=0)
    {
        period.set_period(period_);
    }
    result_type operator()() {
        if (period.check())
            return Imp::operator()();
        return result_type();
    }
    template <class C2>
    result_type operator()(const C2& val) {
        return operator()();
    }
};


// deprecated: use periodic_wrapper<tick_writer> instead
/*
struct progress_ticker {
    typedef void result_type;
    std::ostream *pout;
    unsigned period,left;
    std::string tick;
    progress_ticker() { init(); }
    progress_ticker(std::ostream &o_,unsigned period_=1,const std::string &tick_=".")  { init(&o_); }
    void init(std::ostream *o_=(std::ostream *)NULL,unsigned period_=1,const std::string &tick_=".")  {
        pout=o_;
        period=period_;
        tick=tick_;
        left=period;
    }
    void operator()() {
        DBP4(pout,period,left,tick);
        if (pout) {
            if (!--left) {
                *pout << tick;
                left=period;
            }
        }
    }
};
*/



template <class Forest>
struct self_destruct {
    Forest *me;
    bool safety;
    self_destruct(Forest *me_) : me(me_),safety(false) {}
    void cancel() { safety=true; }
    ~self_destruct() { if (!safety) me->safe_destroy(); }
};

template <class Size=size_t>
struct size_accum {
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
        size=max_size=0;
    }    
    template <class T>
    void operator()(const T& t) {
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
    operator Size() const { return total(); }
};

template <class A,class B>
struct both_functors_byref {
    A &a;
    B &b;
    both_functors_byref(A &a_,B &b_) : a(a_),b(b_) {}
    both_functors_byref(const both_functors_byref<A,B> &self) : a(self.a),b(self.b) {}
    template <class C>
    void operator()(C &c)
    {
        a(c);
        b(c);
    }
};

template <class A,class B>
both_functors_byref<A,B> make_both_functors_byref(A &a_,B &b_)
{
    return both_functors_byref<A,B>(a_,b_);
}


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
        return ::variance(sumsq,sum,N);
    }
    T stddev() const
    {
        return ::stddev(sumsq,sum,N);
    }
    T stderror() const
    {
        return ::stderror(sumsq,sum,N);
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
};

    

template <class c,class t,class T>
std::basic_ostream<c,t> & operator <<(std::basic_ostream<c,t> &o,const stat_accum<T> &v) 
{
    if (v.anyseen())
        return o <<"{{{"<<v.minimum<<'/'<<v.avg()<<"(~"<<v.stddev()<<")/"<<v.maximum<<"}}}";
    else
        return o <<"<<<?/?/?>>>";
}



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


template <class T>
inline std::string concat(const std::string &s,const T& suffix) {
    return s+boost::lexical_cast<std::string>(suffix);
}

template <class S,class T>
inline std::string concat(const S &s,const T& suffix) {
    return boost::lexical_cast<std::string>(s)+boost::lexical_cast<std::string>(suffix);
}



struct set_one {
    template <class C>
    void operator()(C &c) {
        c=1;
    }
};

struct set_zero {
    template <class C>
    void operator()(C &c) {
        c=0;
    }
};

template <class V>
struct set_value {
    V v;
    set_value(const V& init_value) : v(init_value) {}
    template <class C>
    void operator()(C &c) {
        c=v;
    }
};

template <class V>
set_value<V> value_setter(V &init_value) {
    return set_value<V>(init_value);
}


template <class I, class B,class C>
struct indirect_cmp : public C {
    typedef C Comp;
    typedef I Index;
    typedef B Base;
    typedef indirect_cmp<I,B,C> Self;

    B base;

    indirect_cmp(const B &b,const Comp&comp=Comp()) : base(b), Comp(comp) {}
    indirect_cmp(const Self &o): base(o.base), Comp((Comp &)o) {}

    bool operator()(const I &a, const I &b) const {
        return Comp::operator()(base[a], base[b]);
    }
};

template <class I,class B,class C>
void top_n(unsigned n,I begin,I end,const B &base,const C& comp)
{
    indirect_cmp<I,B,C> icmp(base,comp);
    I mid=begin+n;
    if (mid >= end)
        sort(begin,end,icmp);
    else
        partial_sort(begin,mid,end,icmp);
}

// useful for sorting; could parameterize on predicate instead of just <=lt, >=gt
template <class I, class B>
struct indirect_lt {
    typedef I Index;
    typedef B Base;
    B base;
    indirect_lt(const B &b) : base(b) {}
    indirect_lt(const indirect_lt<I,B> &o): base(o.base) {}

    bool operator()(const I &a, const I &b) const {
        return base[a] < base[b];
    }
};

template <class I, class B>
struct indirect_gt {
    typedef I Index;
    typedef B Base;
    B base;
    indirect_gt(const B &b) : base(b) {}
    indirect_gt(const indirect_gt<I,B> &o): base(o.base) {}
    bool operator()(const I &a, const I &b) const {
        return base[a] > base[b];
    }
};

template <class ForwardIterator>
bool is_sorted(ForwardIterator begin, ForwardIterator end)
{
    if (begin == end) return true;

    ForwardIterator next = begin ;
    ++next ;
    for ( ; next != end; ++begin , ++next) {
        if (*next < *begin) return false;
    }

    return true;
}

template <class ForwardIterator, class StrictWeakOrdering>
bool is_sorted(ForwardIterator begin, ForwardIterator end,
               StrictWeakOrdering comp)
{
    if (begin == end) return true;

    ForwardIterator next = begin ;
    ++next ;
    for ( ; next != end ; ++begin, ++next) {
        if ( comp(*next, *begin) ) return false;
    }

    return true;
}

template <class ForwardIterator, class ValueType >
void iota(ForwardIterator begin, ForwardIterator end, ValueType value)
{
    while ( begin != end ) {
        *begin = value ;
        ++begin ;
        ++value ;
    }
}

#ifdef TEST
#include "test.hpp"
#include <cctype>
#include "debugprint.hpp"
#endif

#ifdef TEST
const char *TEST_starts_with[]={
    "s",
    "st",
    "str",
    "str1"
};

const char *TEST_ends_with[]={
    "1",
    "r1",
    "tr1",
    "str1"
};
// NOTE: could use substring but that's more bug-prone ;D

BOOST_AUTO_UNIT_TEST( TEST_FUNCS )
{
    using namespace std;
    string s1("str1"),emptystr;
    BOOST_CHECK(starts_with(s1,emptystr));
    BOOST_CHECK(starts_with(emptystr,emptystr));
    BOOST_CHECK(ends_with(s1,emptystr));
    BOOST_CHECK(ends_with(emptystr,emptystr));
    BOOST_CHECK(!starts_with(s1,string("str11")));
    BOOST_CHECK(!ends_with(s1,string("sstr1")));
    BOOST_CHECK(!starts_with(s1,string("str*")));
    BOOST_CHECK(!ends_with(s1,string("*tr1")));
    BOOST_CHECK(!ends_with(s1,string("str*")));
    BOOST_CHECK(!starts_with(s1,string("*tr1")));
    for (unsigned i=0;i<4;++i) {
        string starts(TEST_starts_with[i]),ends(TEST_ends_with[i]);        
        BOOST_CHECK(starts_with(s1,starts));
        BOOST_CHECK(ends_with(s1,ends));
        BOOST_CHECK(match_begin(s1.begin(),s1.end(),starts.begin(),starts.end()));
        BOOST_CHECK(match_end(s1.begin(),s1.end(),ends.begin(),ends.end()));
        if (i!=3) {            
            BOOST_CHECK(!starts_with(s1,ends));
            BOOST_CHECK(!ends_with(s1,starts));        
            BOOST_CHECK(!match_end(s1.begin(),s1.end(),starts.begin(),starts.end()));
            BOOST_CHECK(!match_begin(s1.begin(),s1.end(),ends.begin(),ends.end()));
        }        
    }
    string s2(" s t  r1");
    BOOST_CHECK(equal_strings_as_seq<char>(s1,s2));
    BOOST_CHECK(!equal_strings_as_seq<string>(s1,s2));
    string s3(" s \nt  \tr1 ");
    BOOST_CHECK(equal_strings_as_seq<char>(s2,s3));
    BOOST_CHECK(equal_strings_as_seq<string>(s2,s3));
    string s4("str1a");
    BOOST_CHECK(!equal_strings_as_seq<string>(s1,s4));
    BOOST_CHECK(!equal_strings_as_seq<char>(s1,s4));
    BOOST_CHECK(!equal_strings_as_seq<char>(s4,s1));    
}

#endif

#endif
