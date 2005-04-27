#ifndef TWO_HASH_H
#define TWO_HASH_H

#define GOLDEN_MEAN_FRACTION 2654435769U

#include "config.h"
#include <ostream>
#include <memory>
#include "byref.hpp"
#ifdef DEBUG
//# define SUPERDEBUG
# endif
#ifdef SUPERDEBUG
#include "debugprint.hpp"
#endif

inline size_t cstr_hash (const char *p)
{
        size_t h=0;
#ifdef OLD_HASH
        unsigned int g;
        while (*p != 0) {
                h = (h << 4) + *p++;
                if ((g = h & 0xf0000000) != 0)
                        h = (h ^ (g >> 24)) ^ g;
        }
        return (h >> 4);
#else
        // google for g_str_hash X31_HASH to see why this is better (less collisions, good performance for short strings, faster)
        while (*p != '\0')
                h = 31 * h + *p++; // should optimize to ( h << 5 ) - h if faster
        return h;
#endif
}

#ifndef GOLDEN_MEAN_FRACTION
#define GOLDEN_MEAN_FRACTION=2654435769U;
#endif
inline size_t uint_hash(unsigned int key)
{
  /*
In Knuth's "The Art of Computer Programming", section 6.4, a multiplicative hashing scheme is introduced as a way to write hash function. The key is multiplied by the golden ratio of 2^32 (2654435761) to produce a hash result.

Since 2654435761 and 2^32 has no common factors in common, the multiplication produces a complete mapping of the key to hash result with no overlap. This method works pretty well if the keys have small values. Bad hash results are produced if the keys vary in the upper bits. As is true in all multiplications, variations of upper digits do not influence the lower digits of the multiplication result.
*/
// (sqrt(5)-1)/2 = .6180339887.., * 2^32=~2654435769.4972302964775847707926

// HOWEVER (not in Knuth) ... the higher order bits after multiplication are determined by all the bits below it as well.  the "good" part of the hash is lost in the higher bits if you are using power-of-2 buckets (prime # buckets is fine), therefore, shift some of those good bits over and combine them with the lower (by using xor instead of addition, this should continue to make the function reversible)
//      return key * 2654435767U;
//      return key * 2654435761U;
        //return key*GOLDEN_MEAN_FRACTION;

// feel free to define TRIVIAL_INT_HASH if you're hashing into prime-numbers of buckets
// but first FIXME: we're using the same primes that are in boost::unordered_map bucketlist for multiplying out int-pair hashvals
    #ifdef EXPENSIVE_INT_HASH
  int tmp = key;

  if (key&0x1) {
    tmp^= 1798151309;
  }

  tmp += ~(tmp << 15);
  tmp ^=  ((unsigned)tmp >> 10);
  tmp +=  (tmp << 3);
  tmp ^=  ((unsigned)tmp >> 6);
  tmp += ~(tmp << 11);
  tmp ^=  ((unsigned)tmp >> 16);
  tmp +=  (tmp << 7);
  tmp ^= ((unsigned)tmp>> 22);
  return (size_t)tmp;
# else

#ifndef TRIVIAL_INT_HASH
  key *= GOLDEN_MEAN_FRACTION; // mixes the lower bits into the upper bits, reversible
  key ^= (key >> 16); // gets some of the goodness back into the lower bits, reversible
  return (size_t)key;
#else
  return (size_t)key;
#endif
#endif
  /*
    key -= (key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key -= (key << 11);
    key ^=  (key >> 16);
    return key;
  */

}

inline size_t mix_hash(size_t a, size_t b)
{
    size_t tmp;
    tmp = a;
    tmp ^= (b<<1);
    if (sizeof(size_t) ==4)
        tmp ^= (b>>31 & 0x1); // you could just not do this and lose the MSB of b.
        // faster and probably not more collisions.  I
        // suppose with multiple mixhash it becomes an issue
    return tmp;

//     size_t tmp;
//     tmp = a;
//     tmp ^= (b<<1);
// #if sizeof(size_t)==4
//         tmp ^= (b>>31 & 0x1); // you could just not do this and lose the MSB of b.
//         // faster and probably not more collisions.  I
//         // suppose with multiple mixhash it becomes an issue
// # else
// #  if sizeof(size_t)!=8
// #   error please adjust bit rotation for size_t (which is not 4 or 8 bytes)
// #  endif
// //    else if (sizeof(size_t)==8)
//         tmp ^= (b>>63 & 0x1);
// #  endif
//     return tmp;
}



#ifdef GNU_HASH_MAP
# define USE_HASH_MAP
# ifndef __NO_GNU_NAMESPACE__
//using namespace __gnu_cxx;
# endif
# include <ext/hash_map>
# define HashTable __gnu_cxx::hash_map
# define HASHNS_B namespace __gnu_cxx {
# define HASHNS_E }
#endif

#ifdef UNORDERED_MAP
# define USE_HASH_MAP
//#error
#include <boost/unordered_map.hpp>
// no template typedef so ...
#define HashTable boost::unordered_map
#define HASHNS_B namespace boost {
#define HASHNS_E }

/*
template <class K,class V,class H,class P,class A>
inline typename HashTable<K,V,H,P,A>::iterator find_value(const HashTable<K,V,H,P,A>& ht,const K& first) {
  return ht.find(first);
}
*/


template <class K,class V,class H,class P,class A>
inline V *add(HashTable<K,V,H,P,A>& ht,const K&k,const V& v=V())
{
  return &(ht[k]=v);
}
#endif


#ifndef USE_HASH_MAP // graehl hash map

#define HASHNS_B
#define HASHNS_E

template <class T> struct hash;

  template <class C>
  struct hash { size_t operator()(const C &c) const { return c.hash(); } };

#include <iostream>
#include <cstdlib>
#include <new>
#include "myassert.h"
#include <utility>

const float DEFAULTHASHLOAD = 0.9f;
template <class K,class V,class H,class P,class A> class HashTable ;
template <typename K, typename V> class HashIter ;
//template <typename K, typename V> class HashConstIter ;

//int pow2Bound(int request);

inline int pow2Bound(int request) {
  Assert(request < (2 << 29));
  int mask = 2;
  for ( ; mask < request ; mask <<= 1 ) ;
  return mask;
}

template <typename K, typename V> class HashEntry {
  HashEntry operator =(HashEntry &);    //disallow
  //operator =(HashEntry &);    //disallow
 public:
  //const
        K first;
  V second;
  typedef K key_type;
  typedef V mapped_type;
//private:
   HashEntry<K,V> *next;
public:
  /*  HashEntry() : next(NULL) { }*/
  HashEntry(const K & k, const V& v) : next(NULL), first(k), second(v) { }
  HashEntry(const K &k, const V& v, HashEntry<K,V> * const n) : next(n), first(k), second(v) { }
  HashEntry(const K &k, HashEntry<K,V> * const n) : next(n), first(k), second() { }
//  HashEntry(const HashEntry &h) : next(h.next), first(h.first), second(h.second) { }
  //friend class HashTable<K,V>;
  //friend class HashIter<K,V>;
  //friend class HashConstIter<K,V>; // Yaser
#if 0
#if (__GNUC__== 2 && __GNUG__== 2  && __GNUC_MINOR__ <= 7) || defined(_MSC_VER)
  // version 2.7.2 or older of gcc compiler does not understand '<>' so it will give
  // an error message if '<>' is present. However, it is required by newer versions
  // of the compiler and if it is not present, a warning will be given
  friend std::ostream & operator << (std::ostream &, const HashEntry<K,V> &);
#else
  friend std::ostream & operator << <> (std::ostream &, const HashEntry<K,V> &);
#endif
#endif
};

template <typename K, typename V,class A,class B>
inline
std::basic_ostream<A,B>&
         operator<< (std::basic_ostream<A,B> &out, const HashEntry<K,V> & e) {
   return out << '(' << e.first << ',' << e.second << ')';
}


/*
template <typename K, typename V> class HashRemoveIter {
  HashTable<K,V> *ht;
  HashEntry<K,V> **bucket;
  HashEntry<K,V> *entry;
  HashEntry<K,V> *next;
  HashEntry<K,V> * operator & () const { return entry; }
  //     operator = ( HashIter & );             // disable
 public:
//  HashIter operator = ( HashIter & );

  HashIter( ) : ht(0) {}
  void init(HashTable<K,V> &t) {
    ht=&t;
    if ( ht->size() > 0 ) {
      bucket = ht->table;
      while ( !*bucket ) bucket++;
      entry = *bucket++;
      next = entry->next;
    } else
      entry = NULL;

  }

  HashIter( HashTable<K,V> &t)
    {
      init(t);
    }
  void operator++()
    {
      if ( entry == NULL )
        return;
      if ( next ) {
        entry = next;
        next = entry->next;
        return;
      }
      for ( ; ; ) {
        if ( bucket >= ht->table + ht->size() ) {
          entry = NULL;
          return;
        }
        if ( (entry = *bucket++) ) {
          next = entry->next;
          return;
        }
      }
    }
//  int bucketNum() const { return int(bucket - ht->table - 1); }
//  operator bool() const { return ( entry != NULL ); }
  bool operator == (void *nocare) {
        return entry == NULL;
  }
  HashEntry<K,V> * operator != (void *nocare) {
        return entry;
  }
  HashEntry<K,V> & operator *() const { return *entry; }
  HashEntry<K,V> * operator ->() const { return entry; }
  void remove() { // remove_next could be very efficient
    const K &k = (*this)->first;
    this->operator++();
    ht->remove(k);
  }
};
*/

template <typename K, typename V> class HashIter { // Yaser added this - 7-27-2000
  //const HashTable<K,V> *ht;
  HashEntry<K,V> ** bucket;
  HashEntry<K,V> ** end_bucket;
  const HashEntry<K,V> *entry;
  //     operator = ( HashIter & );             // disable
 public:
   //  HashConstIter operator = ( HashConstIter & );            // disable
template <class A,class H,class P>
     void init(HashTable<K,V,H,P,A> &t) {

        if ( t.size() > 0 ) {
          end_bucket = t.table + t.bucket_count();
      bucket = t.table;
      while ( !*bucket ) bucket++;
      entry = *bucket++;
        } else  {
          bucket=end_bucket=NULL;
      entry = NULL;
        }
  }

template <class A,class H,class P>
  HashIter( HashTable<K,V,H,P,A> &t)
    {
      init(t);
    }
   HashIter( )  {}
/*
  HashIter( const HashTable<K,V> &t) : ht(t)
    {
      if ( ht.bucket_count() > 0 ) {
        bucket = ht.table;
        while ( !*bucket ) bucket++;
        entry = *bucket++;
      } else
        entry = NULL;
    }*/
  void operator++()
    {
      if ( entry == NULL )
        return ;
          entry = entry->next;
      if ( entry )
                return;

      for ( ; ; ) {
        if ( bucket >= end_bucket ) {
          entry = NULL;
          return;
        }
        if ( (entry = *bucket++) )
          return;
      }
    }
//  int bucketNum() const { return int(bucket - ht.table - 1); }
//  operator int() const { return ( entry != NULL ); }
        bool operator == (void *nocare) const {
        return entry == NULL;
  }
  const HashEntry<K,V> * operator != (void *nocare) const {
        return entry;
  }

  HashEntry<K,V> & operator *() const { return *(HashEntry<K,V> *)entry; }
  HashEntry<K,V> * operator -> () const { return (HashEntry<K,V> *)entry; }
//  const K & first() const { return entry->first; }
//  const V & second() const { return entry->second; }
};
//
//= hash<K>
#include <functional>
#include <memory>
template <typename K, typename V, typename H=::hash<K>, typename P=std::equal_to<K>, typename A=std::allocator<HashEntry<K,V> > > class HashTable : private A::template rebind<HashEntry<K,V> >::other {
 public:
 typedef K key_type;
 typedef V mapped_type;
  typedef H hasher;
  typedef P key_equal;
   typedef HashIter<K,V> iterator;
   typedef HashIter<K,V> const_iterator;

   typedef std::pair<const K,V> value_type;
   typedef HashEntry<K,V> *find_return_type;
   typedef std::pair<find_return_type,bool> insert_return_type;

  hasher hash_function() const { return get_hash(); }
  key_equal key_eq() const { return get_eq(); }
 protected:
  int siz;  // always a power of 2, stored as 1 less than actual for fast mod operator (implemented as and)
  int cnt;
#ifndef STATIC_HASHER
  H hash;
#endif
#ifndef STATIC_HASH_EQUAL
  P m_eq;
#endif
  const P& get_eq() const {
#ifdef STATIC_HASH_EQUAL
        static P p;return p;
#else
        return m_eq;
#endif
  }
  const H& get_hash() const {
#ifdef STATIC_HASHER
        static H h;return h;
#else
        return hash;
#endif
  }
  int growAt;
  typedef typename A::template rebind<HashEntry<K,V> >::other base_alloc;
  HashEntry<K,V> **table;
  size_t hashToPos(size_t hashVal) const
    {
      return hashVal & siz;
    }
public:
    template <class F>
    void visit_key_val(F &f) {
        typedef HashEntry<K,V> *bucket_chain;
        typedef bucket_chain *buckets_iterator;
#ifdef SUPERDEBUG
        DBP_VERBOSE(0);
        DBP_ON;
        DBPC3("visit_key_val",*this,table);
# endif
        for (buckets_iterator i=const_cast<HashEntry<K,V> **>(table),e=const_cast<HashEntry<K,V> **>(table+siz);i<=e;++i) { // <= because siz = #buckets-1
            for (bucket_chain p=*i;p;p=p->next) {
#ifdef SUPERDEBUG
                DBPC5("visit_key_val",p,i,p->first,p->second);
# endif
                f.visit(p->first,p->second);
            }
        }
    }
 class local_iterator : public std::iterator<std::forward_iterator_tag, std::pair<const K,V> > {
   typedef HashEntry<K,V> *T;
   T m_rep;
   typedef std::pair<const K,V> value_type;
 public:

   local_iterator(T t) : m_rep(t) {}

     local_iterator& operator++()
       {
         m_rep = m_rep->next; return *this;
       }
     local_iterator operator++(int)
     {
       local_iterator tmp(*this); m_rep = m_rep->next; return tmp;
     }
     value_type &operator*() const { return *(value_type *)m_rep; }
     value_type *operator->() const { return (value_type *)m_rep; }
     bool operator==(const local_iterator& x) const
     {
       return m_rep == x.m_rep;
     }
     bool operator!=(const local_iterator& x) const
     {
       return m_rep != x.m_rep;
     }

 };
   typedef local_iterator const_local_iterator;

   const_local_iterator begin(size_t i) const {
         return const_cast<HashEntry<K,V> *>(table[i]);
   }
   const_local_iterator end(size_t i) const {
         return NULL;
   }
   local_iterator begin(size_t i) {
         return table[i];
   }
   local_iterator end(size_t i) {
         return NULL;
   }

    const_iterator begin() const {
        return ((HashTable<K,V,H,P,A> *)this)->begin();
        }
        iterator begin()  {
          return *this;
        }
   find_return_type end() const {
         return NULL;
   }

   size_t bucket(const K& first) const {
         return hashToPos(get_hash()(first));
   }
   size_t bucket_size(size_t i) {
         size_t ret=0;
         for (local_iterator l=begin(i),e=end(i);l!=e;++l)
           ++ret;
         return ret;
   }
private:
  /*
  HashTable(const HashTable<K,V,H,P,A> &ht)
    {
      // making this private will probably prevent
      // tables from being copied, and this message
      // will warn if they are

      //    std::cerr << "Unauthorized hash table copy " << &ht << " to " << this << "\n";
      // This  code is added by Yaser to Allow copy contructors for hash tables - ignore comments above
      //    std::cerr << "copying a hash table \n";
      siz = 4;
      cnt = 0;
      growAt = (int)(DEFAULTHASHLOAD * siz);
      if ( growAt < 2 )
        growAt = 2;
      siz--;   // size is actually siz + 1
      table = alloc_table(siz+1);
      for ( int i = 0 ; i <= siz ; i++ )
        table[i] = NULL;
      for(const_iterator k=ht.begin() ; k != ht.end() ; ++k)
        add(k->first,k->second);
      //    std::cerr <<"done\n";
    }
        */
public:
  static const int DEFAULTHASHSIZE=8;
  static const int MINHASHSIZE=4;
  void swap(HashTable<K,V,H,P,A> &h)
    {
      const size_t s = sizeof(HashTable<K,V>)/sizeof(char) + 1;
      char temp[s];
      memcpy(temp, this, s);
      memcpy(this, &h, s);
      memcpy(&h, temp, s);
    }

        HashTable(int sz, const hasher &hf)
#ifndef STATIC_HASHER
          : hash(hf)
#endif
        {
          init(sz);
        }
        HashTable(int sz, const hasher &hf,const key_equal &eq_)
          #if !(defined(STATIC_HASHER) && defined(STATIC_HASH_EQUAL))
          :
#endif
        #ifndef STATIC_HASHER
        hash(hf)
        #endif
#if !(defined(STATIC_HASHER) && defined(STATIC_HASH_EQUAL))
          ,
#endif
          #ifndef STATIC_HASH_EQUAL
           m_eq(eq_)
          #endif
        {
          init(sz);
        }
        HashTable(int sz, const hasher &hf,const key_equal &eq_,const A &a) :
                #ifndef STATIC_HASHER
        hash(hf)
        #endif
#if !(defined(STATIC_HASHER) && defined(STATIC_HASH_EQUAL))
          ,
#endif
          #ifndef STATIC_HASH_EQUAL
           m_eq(eq_),
          #endif
    base_alloc(a) {
          init(sz);
        }
  HashTable(const HashTable<K,V,H,P,A> &ht) : siz(ht.siz), growAt(ht.growAt),
                #ifndef STATIC_HASHER
        hash(ht.hash)
        #endif
#if !(defined(STATIC_HASHER) && defined(STATIC_HASH_EQUAL))
          ,
#endif
          #ifndef STATIC_HASH_EQUAL
           m_eq(ht.m_eq)
          #endif
  base_alloc(*(base_alloc *)this){
        table = alloc_table(siz+1);
        for (int i=0; i <= siz; ++i)
          table[i] = clone_bucket(ht.table[i]);
  }


        explicit HashTable(int sz = DEFAULTHASHSIZE, float mLoad = DEFAULTHASHLOAD) {
          init(sz,mLoad);
        }
  protected:
        HashEntry<K,V> *clone_bucket(HashEntry<K,V> *p) {
          if (!p)
                return p;
          HashEntry<K,V> *ret=alloc_node();
          PLACEMENT_NEW(ret)HashEntry<K,V>(p->first,p->second,clone_bucket(p->next));
          return ret;
        }
  void init(int sz = DEFAULTHASHSIZE, float mLoad = DEFAULTHASHLOAD)
    {
      if ( sz < MINHASHSIZE )
        siz = MINHASHSIZE;
      else
        siz = pow2Bound(sz);
      cnt = 0;
      growAt = (int)(mLoad * siz);
      if ( growAt < 2 )
        growAt = 2;
      siz--;   // size is actually siz + 1
      alloc_table(siz+1);
      for ( int i = 0 ; i <= siz ; i++ )
        table[i] = NULL;
    }
        public:
          void clear() {
                for (int i=0;i<=siz;i++) {
                  for(HashEntry<K,V> *entry=table[i],*next;entry;entry=next) {
                        next=entry->next;
                        free_node(entry);
                  }
                  table[i]=NULL;
                }
                cnt = 0;
          }
  ~HashTable()
    {
      //    std::cerr << "HashTable destructor called\n"; // Yaser
      if ( table ) {
                clear();
        free_table(table,siz);
        table = NULL;
        siz = 0;
      }
    }


public:
        // use insert instead
  V *add(const K &first, const V &second=V())
    {
      if ( ++cnt >= growAt )
                rehash(2 * siz);
      size_t i = bucket(first);
          HashEntry<K,V> *next=table[i];
      table[i] = alloc_node();
                PLACEMENT_NEW (table[i]) HashEntry<K,V>(first, second, next);
      return &table[i]->second;
    }
public:

        // bool is true if insertion was performed, false if key already existed.  pointer to the key/val pair in the table is returned
  private:
        bool equal(const K& k, const K &k2) const {
          return get_eq()(k,k2);
        }
public:
  // not part of standard!
        insert_return_type insert(const K& first, const V& second=V()) {
          size_t hv=get_hash()(first);
          size_t bucket=hashToPos(hv);
          for ( HashEntry<K,V> *p = table[bucket]; p ; p = p->next )
                if ( equal(p->first,first) )
                  return std::pair<find_return_type,bool>((find_return_type)p,false);

          if ( ++cnt >= growAt ) {
                rehash(2 * siz);
                bucket=hashToPos(hv);
          }
          HashEntry<K,V> *next=table[bucket];
          table[bucket] = alloc_node();
          PLACEMENT_NEW (table[bucket]) HashEntry<K,V>(first, second, next);
          return insert_return_type(
                reinterpret_cast<find_return_type>(table[bucket])
                ,true);

        }

        insert_return_type insert(const value_type &t) {
          return insert(t.first,t.second);
        }


  find_return_type find(const K &first) const
    {
      for ( HashEntry<K,V> *p = table[bucket(first)]; p ; p = p->next )
        if ( equal(p->first,first) )
          return reinterpret_cast<find_return_type>(p);
      return NULL;
    }
  V *find_second(const K& first) const
        {
      for ( HashEntry<K,V> *p = table[bucket(first)]; p ; p = p->next )
        if ( equal(p->first,first) )
          return &(p->second);
      return NULL;

        }
        value_type * find_value(const K &first) const { return (value_type *)find(first); }

/*  V * findOrAdd(const K &first)
    {
      V *ret;
      if ( (ret = find(first)) )
        return ret;
      return add(first);
    }*/
  V & operator[](const K &first)
    {
      //return *findOrAdd(first);
          return insert(first).first->second;
    }

  int erase(const K &first)
    {
      size_t i = bucket(first);
      HashEntry<K,V> *prev = NULL, *p = table[i];
      if ( !p )
        return 0;
      else if ( equal(p->first,first) ) {
        table[i] = p->next;
        free_node(p);
        --cnt;
        return 1;
      }
      for ( ; ; ) {
        prev = p;
        p = p->next;
        if ( !p ) break;
        if ( equal(p->first,first) ) {
          prev->next = p->next;
          free_node(p);
          --cnt;
          return 1;
        }
      }
      return 0;
    }
  size_t bucket_count() const { return siz + 1; }
  size_t max_bucket_count() const { return 0x7FFFFFFF; }
  int size() const { return cnt; }
  int growWhen() const { return growAt; }
  float load_factor() const { return (float)cnt / (float)(siz + 1); }
  float max_load_factor() const { return (float)growAt / (float)(siz + 1); }
  void max_load_factor(float mLoad)
    {
      growAt = (int)(mLoad * (siz+1));
      if ( growAt < 2 )
        growAt = 2;
    }
        protected:
  void rehash_pow2(int request)
    {
      size_t hashVal;
          int oldSiz = siz;
      HashEntry<K,V> *next, *p, **i, **oldTable = table;
      siz = request;
      alloc_table(siz);
      siz--;  // actual size is siz + 1 (power of 2)
      for ( i = table; i <= table + siz ; i++ )
        *i = NULL;
      for ( i = oldTable ; i <= oldTable + oldSiz ; i++ )
        for ( p = *i ; p ; p = next ) {
          next = p->next;
          hashVal = bucket(p->first);
          p->next = table[hashVal];
          table[hashVal] = p;
        }
      growAt = int((float(growAt) * (siz+1)) / (oldSiz+1))+1;
      free_table(oldTable,oldSiz);
    }
        public:
          void rehash(int request) { rehash_pow2(pow2Bound(request)); }
  friend class HashIter<K,V>;
  //friend class HashConstIter<K,V>;

        public:


  private:
#ifndef _MSC_VER
        typedef typename A::template rebind<HashEntry<K,V> * >::other table_alloc;

        void alloc_table(unsigned int _n)
        {
          table=table_alloc().allocate(_n); // should we be keeping the alloc around permanently?  probably.
        }
        void free_table(HashEntry<K,V> ** t,int size) {
          table_alloc().deallocate(t,size+1);
        }
#else

        void alloc_table(unsigned int _n)
        {
          table=NEW HashEntry<K,V> *[_n];
        }
        void free_table(HashEntry<K,V> ** t, int size) {
          delete[] t;
        }
#endif
        HashEntry<K,V> *alloc_node() { return this->allocate(1); }
        void free_node(HashEntry<K,V> *p) { return this->deallocate(p,1); }
//              template <class _K,class _V,class _H,class _A>
//friend _V *find_second(const HashTable<_K,_V,_H,_A>& ht,const _K& first);
};
/*
struct EmptyStruct {
};

template <typename K, typename H=::hash<K>, typename P=std::equal_to<K>, typename A=std::allocator<HashEntry<K,V> > > class HashSet : public HashTable<K,V,H,P,A> {
};

*/

/*
template <class K,class V,class H,class P,class A>
inline typename HashTable<K,V,H,P,A>::find_return_type *find_value(const HashTable<K,V,H,P,A>& ht,const K& first) {
  return ht.find(first);
}
*/


#include "byref.hpp"
template <class K,class V,class H,class P,class A,class F>
void enumerate(const HashTable<K,V,H,P,A>& ht,const K& first,F f)
{
  for (size_t i=0;i<ht.bucket_count();++i)
    for (typename HashTable<K,V,H,P,A>::local_iterator j=ht.begin(i),e=ht.end(i);j!=e;++j)
      deref(f)(*j);
}

template <class K,class V,class H,class P,class A>
inline V *add(HashTable<K,V,H,P,A>& ht,const K&k,const V& v=V())
{
  return ht.add(k,v);
}


template<>
struct hash<unsigned>
{
  size_t operator()(unsigned int key) const {
        return uint_hash(key);
  }
};

template<>
struct hash<char>
{
  size_t operator()(char key) const {
        return key;
  }
};


template<>
struct hash<int>
{
  size_t operator()(int key) const {
        return uint_hash(key);
  }
};

template<>
struct hash<const char *>
{
  size_t operator()(const char * s) const {
        return cstr_hash(s);
  }
};

#endif // graehl HashTable

template<class T1,class T2,class T3,class T4,class T5,class A,class B>
inline
std::basic_ostream<A,B>&
         operator<< (std::basic_ostream<A,B> &out, const HashTable<T1,T2,T3,T4,T5>& t) {
  typename HashTable<T1,T2,T3,T4,T5>::const_iterator i=t.begin();
  out << "begin" << std::endl;
  for (;i!=t.end();++i) {
        out << *i << std::endl;
  }
  out << "end" << std::endl;
  return out;
}

template<class C1,class C2,class A,class B>
inline
std::basic_ostream<A,B>&
         operator<< (std::basic_ostream<A,B> &out, const std::pair< C1, C2 > &p)
{
  return out << '(' << p.first << ',' << p.second << ')';
}

#define BEGIN_HASH_VAL(C) \
HASHNS_B \
template<> struct hash<C> \
{ \
  size_t operator()(const C x) const



#define BEGIN_HASH(C) \
HASHNS_B \
template<> struct hash<C> \
{ \
  size_t operator()(const C& x) const

#define END_HASH        \
};\
HASHNS_E

HASHNS_B
  template<class P> struct hash<P *>
  {
    const size_t operator()( const P *x ) const
    {
//      const unsigned GOLDEN_MEAN_FRACTION=2654435769U;
      return ((size_t)x / sizeof(P))*GOLDEN_MEAN_FRACTION;
    }
  };
HASHNS_E

template <class K,class V,class H,class P,class A>
inline V *find_second(const HashTable<K,V,H,P,A>& ht,const K& first)
{
  return ht.find_second(first);
}

template <class K,class V,class H,class P,class A>
inline
  typename HashTable<K,V,H,P,A>::insert_return_type insert(HashTable<K,V,H,P,A>& ht,const K& first,const V &v=V())
{
  return ht.insert(first,v);
}

#endif
