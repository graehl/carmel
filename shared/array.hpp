#ifndef GRAEHL_SHARED__ARRAY_HPP
#define GRAEHL_SHARED__ARRAY_HPP

#ifdef TEST
#include <graehl/shared/test.hpp>
#include <vector>
#endif

#include <algorithm> //swap
namespace graehl {

///WARNING: only use for std::vector and similar
template <class Vec>
typename Vec::value_type * array_begin(Vec & v) 
{
    return &*v.begin();
}

template <class Vec>
typename Vec::value_type const* array_begin(Vec const& v) 
{
    return &*v.begin();
}

template <class Vec>
unsigned index_of(Vec const& v,typename Vec::value_type const* p) 
{
    return p-array_begin(v);
}

template <class C> inline
void resize_up_for_index(C &c,size_t i) 
{
    const size_t newsize=i+1;
    if (newsize > c.size())
        c.resize(newsize);
}

template <class Vec>
void remove_marked_swap(Vec & v,bool marked[]) {
    using std::swap;
    typedef typename Vec::value_type V;
    unsigned sz=v.size();
    if ( !sz ) return;
    unsigned to, i = 0;
    while ( i < sz && !marked[i] ) ++i;
    to = i; // find first marked (don't need to move anything below it)
    while (i<sz)
        if (!marked[i]) {
            swap(v[to++],v[i++]);
        } else {
            ++i;
        }
    v.resize(to);
}

// outputs sequence to iterator out, of new indices for each element i, corresponding to deleting element i from an array when remove[i] is true (-1 if it was deleted, new index otherwise), returning one-past-end of out (the return value = # of elements left after deletion)
template <class AB,class ABe,class O>
unsigned new_indices(AB i, ABe end,O out) {
    int f=0;
    while (i!=end)
        *out++ = *i++ ? -1 : f++;
    return f;
};

template <class AB,class O>
unsigned new_indices(AB remove,O out) {
    return new_indices(remove.begin(),remove.end());
}

template <class Vec,class P>
void remove_if_swap(Vec & v,P const& pred) {
    using std::swap;
    typedef typename Vec::value_type V;
    unsigned sz=v.size();
    if ( !sz ) return;
    unsigned to, i = 0;
    while ( i < sz && !pred(v[i]) ) ++i;
    to = i; // find first marked (don't need to move anything below it)
    while (i<sz)
        if (pred(v[i])) {
            swap(v[to++],v[i++]);
        } else {
            ++i;
        }
    v.resize(to);
}

template <class C>
void push_back(C &c)
{
    c.push_back(typename C::value_type());
}


#if 0
#ifdef TEST

namespace array_test {

bool rm1[] = { 0,1,1,0,0,1,1 };
bool rm2[] = { 1,1,0,0,1,0,0 };
int a[] = { 1,2,3,4,5,6,7 };
int a1[] = { 1, 4, 5 };
int a2[] = {3,4,6,7};
}

#include <algorithm>
#include <iterator>
BOOST_AUTO_UNIT_TEST( dynarray )
{
    using namespace std;
    {
        const int N=10;

    StackAlloc al;
    int aspace[N];
    al.init(aspace,aspace+N);
    istringstream ina("(1 2 3 4)");
    array<int> aint;
    read(ina,aint,al);
    BOOST_CHECK(aint.size()==4);
    BOOST_CHECK(aint[3]==4);
    BOOST_CHECK(al.top=aspace+4);
    }

    {
        fixed_array<fixed_array<int> > aa,ba;
        std::string sa="(() (1) (1 2 3) () (4))";
        BOOST_CHECK(test_extract_insert(sa,aa));
        IndirectReader<plus_one_reader> reader;
        istringstream ss(sa);

        ba.read(ss,reader);

//        DBP(ba);
        BOOST_REQUIRE(aa.size()==5);
        BOOST_CHECK(aa[2].size()==3);
        BOOST_REQUIRE(ba.size()==5);
        BOOST_CHECK(ba[2].size()==3);
        BOOST_CHECK(aa[1][0]==1);
        BOOST_CHECK(ba[1][0]==2);
    }

    {
        fixed_array<fixed_array<int> > aa,ba;
        std::string sa="(() (1) (1 2 3) () (4))";
        BOOST_CHECK(test_extract_insert(sa,aa));
        IndirectReader<plus_one_reader> reader;
        istringstream ss(sa);

        ba.read(ss,reader);

//        DBP(ba);
        BOOST_REQUIRE(aa.size()==5);
        BOOST_CHECK(aa[2].size()==3);
        BOOST_REQUIRE(ba.size()==5);
        BOOST_CHECK(ba[2].size()==3);
        BOOST_CHECK(aa[1][0]==1);
        BOOST_CHECK(ba[1][0]==2);
    }
    {
        dynamic_array<int> a;
        a.at_grow(5)=1;
        BOOST_CHECK(a.size()==5+1);
        BOOST_CHECK(a[5]==1);
        for (int i=0; i < 5; ++i)
            BOOST_CHECK(a.at(i)==0);
    }
    const int sz=7;
    {
        dynamic_array<int> a(sz);
        a.push_back_n(sz,sz*3);
        BOOST_CHECK(a.size() == sz*3);
        BOOST_CHECK(a.capacity() >= sz*3);
        BOOST_CHECK(a[sz]==sz);
    }

    {
        dynamic_array<int> a(sz*3,sz);
        BOOST_CHECK(a.size() == sz*3);
        BOOST_CHECK(a.capacity() == sz*3);
        BOOST_CHECK(a[sz]==sz);
    }

    using namespace std;
    array<int> aa(sz);
    BOOST_CHECK(aa.capacity() == sz);
    dynamic_array<int> da;
    dynamic_array<int> db(sz);
    BOOST_CHECK(db.capacity() == sz);
    copy(a,a+sz,aa.begin());
    copy(a,a+sz,back_inserter(da));
    copy(a,a+sz,back_inserter(db));
    BOOST_CHECK(db.capacity() == sz); // shouldn't have grown
    BOOST_CHECK(search(a,a+sz,aa.begin(),aa.end())==a); // really just tests begin,end as proper iterators
    BOOST_CHECK(da.size() == sz);
    BOOST_CHECK(da.capacity() >= sz);
    BOOST_CHECK(search(a,a+sz,da.begin(),da.end())==a); // tests push_back
    BOOST_CHECK(search(a,a+sz,db.begin(),db.end())==a); // tests push_back
    BOOST_CHECK(search(da.begin(),da.end(),aa.begin(),aa.end())==da.begin());
    for (int i=0;i<sz;++i) {
        BOOST_CHECK(a[i]==aa.at(i));
        BOOST_CHECK(a[i]==da.at(i));
        BOOST_CHECK(a[i]==db(i));
    }
    BOOST_CHECK(da==aa);
    BOOST_CHECK(db==aa);
    const int sz1=3,sz2=4;;
    da.removeMarked(rm1); // removeMarked
    BOOST_REQUIRE(da.size()==sz1);
    for (int i=0;i<sz1;++i)
        BOOST_CHECK(a1[i]==da[i]);
    db.removeMarked(rm2);
    BOOST_REQUIRE(db.size()==sz2);
    for (int i=0;i<sz2;++i)
        BOOST_CHECK(a2[i]==db[i]);
    array<int> d1map(sz),d2map(sz);
    BOOST_CHECK(3==new_indices(rm1,rm1+sz,d1map.begin()));
    BOOST_CHECK(4==new_indices(rm2,rm2+sz,d2map.begin()));
    int c=0;
    for (unsigned i=0;i<d1map.size();++i)
        if (d1map[i]==-1)
            ++c;
        else
            BOOST_CHECK(da[d1map[i]]==aa[i]);
    BOOST_CHECK(c==4);
    db(10)=1;
    BOOST_CHECK(db.size()==11);
    BOOST_CHECK(db.capacity()>=11);
    BOOST_CHECK(db[10]==1);
    aa.dealloc();

    std::string emptya=" ()";
    std::string emptyb="()";
    {
        array<int> a;
        dynamic_array<int> b;
        istringstream iea(emptya);
        iea >> a;
        stringstream o;
        o << a;
        BOOST_CHECK(o.str()==emptyb);
        o >> b;
        BOOST_CHECK(a==b);
        BOOST_CHECK(a.size()==0);
        BOOST_CHECK(b.size()==0);
    }

    string sa="( 2 ,3 4\n \n\t 5,6)";
    string sb="(2 3 4 5 6)";

#define EQIOTEST(A,B)  do { A<int> a;B<int> b;stringstream o;istringstream isa(sa);isa >> a; \
        o << a;BOOST_CHECK(o.str() == sb);o >> b;BOOST_CHECK(a==b);} while(0)

    EQIOTEST(array,array);
    EQIOTEST(array,dynamic_array);
    EQIOTEST(dynamic_array,array);
    EQIOTEST(dynamic_array,dynamic_array);
}
#endif
#endif

}



#endif
