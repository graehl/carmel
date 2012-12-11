//power-of-two hash table.  alternatively (defined USE_STD_HASH_MAP) uses the vendor's.  also gives a more convenient Value *find_second(key)
//original rationale: lack of STL support in gcc - and hash_map still isn't officially in STL.
#ifndef GRAEHL_SHARED__2HASH_H
#define GRAEHL_SHARED__2HASH_H

#include <graehl/shared/config.h>
#include <graehl/shared/print_read.hpp>
#include <ostream>
#include <memory>
#include <graehl/shared/byref.hpp>
#include <graehl/shared/hashtable_fwd.hpp>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/byref.hpp>
#include <boost/config.hpp>

#include <graehl/shared/config.h>
#include <ostream>
#include <memory>
#include <graehl/shared/byref.hpp>
#ifdef DEBUG
//# define SUPERDEBUG
# endif
#ifdef SUPERDEBUG
#include <graehl/shared/debugprint.hpp>
#endif

#ifndef USE_STD_HASH_MAP // graehl hash map

#include <iostream>
#include <cstdlib>
#include <new>
#include <graehl/shared/myassert.h>
#include <utility>
#include <functional>
#include <memory>

namespace graehl {

// HashTable

const float DEFAULTHASHLOAD = 0.9f;
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
//private:
   HashEntry<K,V> *next;
  //const
        K first;
  V second;
  typedef K key_type;
  typedef V mapped_type;
    typedef HashEntry<K,V> self_type;
public:
  /*  HashEntry() : next(NULL) { }*/
    template <class V_init>
  HashEntry(const K & k, const V_init& v) : next(NULL), first(k), second(v) { }
    template <class V_init>
  HashEntry(const K &k, const V_init& v, HashEntry<K,V> * const n) : next(n), first(k), second(v) { }
  HashEntry(const K &k, HashEntry<K,V> * const n) : next(n), first(k), second() { }
//  HashEntry(const HashEntry &h) : next(h.next), first(h.first), second(h.second) { }
  //friend class HashTable<K,V>;
  //friend class HashIter<K,V>;
  //friend class HashConstIter<K,V>; // Yaser
    template <class O> void print(O&o) const
    {
        o << '(' << first << ',' << second << ')';
    }
    TO_OSTREAM_PRINT
};

/*
template <typename K, typename V,class A,class B>
inline
std::basic_ostream<A,B>&
         operator<< (std::basic_ostream<A,B> &out, const HashEntry<K,V> & e) {
   return out <<
}
*/

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
template <class K, class V, class H=hash<K>, class P=std::equal_to<K>, class A=std::allocator<HashEntry<K,V> > > class HashTable : private A::template rebind<HashEntry<K,V> >::other {
 public:
 typedef K key_type;
 typedef V mapped_type;
  typedef H hasher;
  typedef P key_equal;
   typedef HashIter<K,V> iterator;
   typedef HashIter<K,V> const_iterator;

   typedef std::pair<const K,V> value_type;
   typedef HashEntry<K,V> *find_result_type;
   typedef std::pair<find_result_type,bool> insert_result_type;

  hasher hash_function() const { return get_hash(); }
  key_equal key_eq() const { return get_eq(); }
 protected:
  unsigned siz;  // always a power of 2, stored as 1 less than actual for fast mod operator (implemented as and)
  unsigned cnt;
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
  unsigned growAt;
    typedef HashEntry<K,V> Node;

  typedef typename A::template rebind<Node >::other base_alloc;
  Node **table;
  std::size_t hashToPos(std::size_t hashVal) const
    {
      return hashVal & siz;
    }
public:
    template <class F>
    void visit_key_val(F &f) {
        typedef Node *bucket_chain;
        typedef bucket_chain *buckets_iterator;
#ifdef SUPERDEBUG
        DBP_VERBOSE(0);
        DBP_ON;
        DBPC3("visit_key_val",*this,table);
# endif
        for (buckets_iterator i=const_cast<Node **>(table),e=const_cast<Node **>(table+siz);i<=e;++i) { // <= because siz = #buckets-1
            for (bucket_chain p=*i;p;p=p->next) {
#ifdef SUPERDEBUG
                DBPC5("visit_key_val",p,i,p->first,p->second);
# endif
                f.visit(p->first,p->second);
            }
        }
    }
 class local_iterator : public std::iterator<std::forward_iterator_tag, std::pair<const K,V> > {
   typedef Node *T;
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

   const_local_iterator begin(std::size_t i) const {
         return const_cast<Node *>(table[i]);
   }
   const_local_iterator end(std::size_t i) const {
         return NULL;
   }
   local_iterator begin(std::size_t i) {
         return table[i];
   }
   local_iterator end(std::size_t i) {
         return NULL;
   }

    const_iterator begin() const {
        return ((HashTable<K,V,H,P,A> *)this)->begin();
    }
    iterator begin()  {
        return iterator(*this);
    }
    find_result_type end() const {
        return NULL;
    }

   std::size_t bucket(const K& first) const {
         return hashToPos(get_hash()(first));
   }
   std::size_t bucket_size(std::size_t i) {
         std::size_t ret=0;
         for (local_iterator l=begin(i),e=end(i);l!=e;++l)
           ++ret;
         return ret;
   }

    BOOST_STATIC_CONSTANT(unsigned,DEFAULTHASHSIZE=8);
    BOOST_STATIC_CONSTANT(unsigned,MINHASHSIZE=4);
  void swap(HashTable<K,V,H,P,A> &h)
    {
      const std::size_t s = sizeof(HashTable<K,V>)/sizeof(char) + 1;
      char temp[s];
      memcpy(temp, this, s);
      memcpy(this, &h, s);
      memcpy(&h, temp, s);
    }

        HashTable(unsigned sz, const hasher &hf)
#ifndef STATIC_HASHER
          : hash(hf)
#endif
        {
          init(sz);
        }
        HashTable(unsigned sz, const hasher &hf,const key_equal &eq_)
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
        HashTable(unsigned sz, const hasher &hf,const key_equal &eq_,const A &a) :
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
  HashTable(const HashTable<K,V,H,P,A> &ht) : siz(ht.siz), growAt(ht.growAt)
#ifndef STATIC_HASHER
                                            , hash(ht.hash)
#endif
#ifndef STATIC_HASH_EQUAL
                                            , m_eq(ht.m_eq),
#endif
            , base_alloc((base_alloc &)ht)
    {
//        Assert(0 == "don't copy hash tables, please");
        alloc_table(bucket_count());
        for (unsigned i=0; i <= siz; ++i)
            table[i] = clone_bucket(ht.table[i]);
    }


        explicit HashTable(unsigned sz = DEFAULTHASHSIZE, float mLoad = DEFAULTHASHLOAD) {
          init(sz,mLoad);
        }
  protected:
        Node *clone_bucket(Node *p) {
          if (!p)
              return p;
          Node *ret=alloc_node();
          PLACEMENT_NEW(ret)Node(p->first,p->second,clone_bucket(p->next));
          return ret;
        }
  void init(unsigned sz = DEFAULTHASHSIZE, float mLoad = DEFAULTHASHLOAD)
    {
      if ( sz < MINHASHSIZE )
        siz = MINHASHSIZE;
      else
        siz = pow2Bound(sz);
      cnt = 0;
      growAt = (unsigned)(mLoad * siz);
      if ( growAt < 2 )
        growAt = 2;
      siz--;   // size is actually siz + 1
      alloc_table(bucket_count());
      for ( unsigned i = 0 ; i <= siz ; i++ )
        table[i] = NULL;
    }
    void grow()
    {
        rehash_pow2(2*bucket_count());
    }

 public:
    void clear() {
        for (unsigned i=0;i<=siz;i++) {
            for(Node *entry=table[i],*next;entry;entry=next) {
                next=entry->next;
                delete_node(entry);
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

    // use insert instead
    V *add(const K &first, const V &second=V())
    {
        if ( ++cnt >= growAt )
            grow();
        std::size_t i = bucket(first);
        Node *next=table[i];
        PLACEMENT_NEW (table[i] = alloc_node()) Node(first, second, next);
        return &table[i]->second;
    }

        // bool is true if insertion was performed, false if key already existed.  pointer to the key/val pair in the table is returned
  private:
        bool equal(const K& k, const K &k2) const {
          return get_eq()(k,k2);
        }
public:
  // not part of standard!
    insert_result_type insert(const K& first, const V& second/*=V()*/) {
          std::size_t hv=get_hash()(first);
          std::size_t bucket=hashToPos(hv);
          for ( Node *p = table[bucket]; p ; p = p->next )
                if ( equal(p->first,first) )
                  return insert_result_type(
                      (find_result_type)p
                      ,false);

          if ( ++cnt >= growAt ) {
              grow();
              bucket=hashToPos(hv);
          }
          Node *next=table[bucket];
          PLACEMENT_NEW (table[bucket] = alloc_node()) Node(first, second, next);
          return insert_result_type(
                reinterpret_cast<find_result_type>(table[bucket])
                ,true);

        }

    // copied from above
        insert_result_type insert(const K& first) {
          std::size_t hv=get_hash()(first);
          std::size_t bucket=hashToPos(hv);
          for ( Node *p = table[bucket]; p ; p = p->next )
                if ( equal(p->first,first) )
                  return insert_result_type(
                      (find_result_type)p
                      ,false);

          if ( ++cnt >= growAt ) {
              grow();
                bucket=hashToPos(hv);
          }
          Node *next=table[bucket];
          PLACEMENT_NEW (table[bucket] = alloc_node()) Node(first,next);
          return insert_result_type(
                reinterpret_cast<find_result_type>(table[bucket])
                ,true);

        }

        insert_result_type insert(const value_type &t) {
          return insert(t.first,t.second);
        }


  find_result_type find(const K &first) const
    {
      for ( Node *p = table[bucket(first)]; p ; p = p->next )
        if ( equal(p->first,first) )
          return reinterpret_cast<find_result_type>(p);
      return NULL;
    }
  V *find_second(const K& first) const
        {
      for ( Node *p = table[bucket(first)]; p ; p = p->next )
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
  V const& operator[](const K &first) const
    {
        return *find_second(first);
    }

    bool erase(const K &first)
    {
      std::size_t i = bucket(first);
      Node *prev = NULL, *p = table[i];
      if ( !p )
        return 0;
      else if ( equal(p->first,first) ) {
        table[i] = p->next;
        delete_node(p);
        --cnt;
        return 1;
      }
      for ( ; ; ) {
        prev = p;
        p = p->next;
        if ( !p ) break;
        if ( equal(p->first,first) ) {
          prev->next = p->next;
          delete_node(p);
          --cnt;
          return 1;
        }
      }
      return 0;
    }
  std::size_t bucket_count() const { return siz + 1; }
  std::size_t max_bucket_count() const { return 0x7FFFFFFF; }
  int size() const { return cnt; }
  int growWhen() const { return growAt; }
  float load_factor() const { return (float)cnt / (float)(siz + 1); }
  float max_load_factor() const { return (float)growAt / (float)(siz + 1); }
  void max_load_factor(float mLoad)
    {
      growAt = (unsigned)(mLoad * (bucket_count()));
      if ( growAt < 2 )
        growAt = 2;
    }
        protected:
  void rehash_pow2(unsigned request)
    {
      std::size_t hashVal;
      unsigned oldSiz = siz;
      Node *next, *p, **i, **oldTable = table;
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
      growAt = unsigned((float(growAt) * (siz+1)) / (oldSiz+1))+1;
      free_table(oldTable,oldSiz);
    }
        public:
          void rehash(unsigned request) { rehash_pow2(pow2Bound(request)); }
  friend class HashIter<K,V>;
  //friend class HashConstIter<K,V>;

        public:


  private:
#ifndef _MSC_VER
        typedef typename A::template rebind<Node * >::other table_alloc;

        void alloc_table(unsigned _n)
        {
          table=table_alloc().allocate(_n); // should we be keeping the alloc around permanently?  probably.
        }
        void free_table(Node ** t,unsigned size) {
          table_alloc().deallocate(t,size+1);
        }
#else

        void alloc_table(unsigned _n)
        {
          table=NEW Node *[_n];
        }
        void free_table(Node ** t, unsigned size) {
          delete[] t;
        }
#endif
        Node *alloc_node() { return this->allocate(1); }
    void delete_node(Node *p)
    {
        p->~Node();
        free_node(p);
    }
        void free_node(Node *p) { return this->deallocate(p,1); }
//              template <class _K,class _V,class _H,class _A>
//friend _V *find_second(const HashTable<_K,_V,_H,_A>& ht,const _K& first);
};

template <class K,class V,class H,class P,class A>
struct hash_traits<HashTable<K,V,H,P,A> >
{
    typedef HashTable<K,V,H,P,A> HT;
    typedef typename HT::find_result_type find_result_type;
    typedef typename HT::insert_result_type insert_result_type;
};

template <class K,class V,class H,class P,class A,class F>
inline void swap(HashTable<K,V,H,P,A>& a,HashTable<K,V,H,P,A>& b)
{
    a.swap(b);
}


/*
struct EmptyStruct {
};

template <typename K, typename H=::hash<K>, typename P=std::equal_to<K>, typename A=std::allocator<HashEntry<K,V> > > class HashSet : public HashTable<K,V,H,P,A> {
};

*/

/*
template <class K,class V,class H,class P,class A>
inline typename HashTable<K,V,H,P,A>::find_result_type *find_value(const HashTable<K,V,H,P,A>& ht,const K& first) {
  return ht.find(first);
}
*/


template <class K,class V,class H,class P,class A,class F>
inline void enumerate(const HashTable<K,V,H,P,A>& ht,const K& first,F f)
{
  for (std::size_t i=0;i<ht.bucket_count();++i)
    for (typename HashTable<K,V,H,P,A>::local_iterator j=ht.begin(i),e=ht.end(i);j!=e;++j)
      deref(f)(*j);
}

template <class K,class V,class H,class P,class A>
inline V *add(HashTable<K,V,H,P,A>& ht,const K&k,const V& v=V())
{
  return ht.add(k,v);
}


template <class K>
inline std::size_t hash_value_dispatch(K const& k)
{
    return hash_value(k); // argument dependent lookup
}


template<>
struct hash<unsigned>
{
  unsigned operator()(unsigned key) const {
        return uint32_hash(key);
  }
};

template<>
struct hash<char>
{
  char operator()(char key) const {
        return key;
  }
};


template<>
struct hash<int>
{
  unsigned operator()(int key) const {
        return uint32_hash(key);
  }
};

template<>
struct hash<const char *>
{
  std::size_t operator()(const char * s) const {
        return cstr_hash(s);
  }
};

}//ns

#endif // graehl HashTable

#endif
