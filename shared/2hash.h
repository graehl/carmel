#ifndef TWO_HASH_H 
#define TWO_HASH_H

#include "config.h"
#include <ostream>

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


#ifdef UNORDERED_MAP

#include <boost/unordered_map.hpp>
// no template typedef so ...
#define HashTable boost::unordered_map
#define HASHNS_B namespace boost {
#define HASHNS_E }

template <class K,class V,class H,class P,class A>
inline typename HashTable<K,V,H,P,A>::iterator find_value(const HashTable<K,V,H,P,A>& ht,const K& first) {
  return ht.find(first);
}

template <class K,class V,class H,class P,class A>
inline V *find_second(const HashTable<K,V,H,P,A>& ht,const K& first)
{
  return ht.find_second(first);
}

template <class K,class V,class H,class P,class A>
inline V *add(HashTable<K,V,H,P,A>& ht,const K&k,const V& v=V())
{
  return &(ht[k]=v);
}

#else // internal hash map

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

const float DEFAULTHASHLOAD = 1.0f;
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
  HashEntry operator =(HashEntry &);	//disallow
  //operator =(HashEntry &);	//disallow
 public:
  //const 
	K first;
  V second;
typedef K first_type;
typedef V second_type;
//private:
   HashEntry<K,V> *next;
public:
  /*  HashEntry() : next(NULL) { }*/
  HashEntry(const K & k, const V& v) : next(NULL), first(k), second(v) { }
  HashEntry(const K &k, const V& v, HashEntry<K,V> * const n) : next(n), first(k), second(v) { }
  HashEntry(const K &k, HashEntry<K,V> * const n) : next(n), first(k), second() { }
//  HashEntry(const HashEntry &h) : next(h.next), first(h.first), second(h.second) { }
#ifdef CUSTOMNEW
  static HashEntry<K,V> *freeList=NULL;
  static const int newBlocksize=128;
  void *operator new(size_t s)
    {
      size_t dummy = s;
      dummy = dummy;
      HashEntry<K,V> *ret, *max;
      if (freeList) {
	ret = freeList;
	freeList = freeList->next;
	return ret;
      }
      freeList = (HashEntry<K,V> *)::operator new(newBlocksize * sizeof(HashEntry<K,V>));
      freeList->next = NULL;
      max = freeList + newBlocksize -1;
      for ( ret = freeList++; freeList < max ; ret = freeList++ )
	freeList->next = ret;
      return freeList--;
    }
  void operator delete(void *p) 
    {
      HashEntry<K,V> *e = (HashEntry<K,V> *)p;
      e->next = freeList;
      freeList = e;
    }
#endif	
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
  //     operator = ( HashIter & );		// disable
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
  //     operator = ( HashIter & );		// disable
 public:
   //  HashConstIter operator = ( HashConstIter & );		// disable
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
	bool operator == (void *nocare) {
	return entry == NULL;
  }
  const HashEntry<K,V> * operator != (void *nocare) {
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
template <typename K, typename V, typename H=::hash<K>, typename P=std::equal_to<K>, typename A=std::allocator<HashEntry<K,V> > > class HashTable : private A::rebind<HashEntry<K,V> >::other {
 public:
  typedef H hasher;
  typedef P key_equal;
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
  typedef typename A::rebind<HashEntry<K,V> >::other base_alloc;
  
  HashEntry<K,V> **table;
  size_t hashToPos(size_t hashVal) const
    {
      return hashVal & siz;
    }
public:
   typedef HashIter<K,V> iterator;
   typedef HashIter<K,V> const_iterator;

   typedef std::pair<const K,V> value_type;
   typedef K key_type;	
   typedef HashEntry<K,V> *find_return_type;
   typedef V second_type;
   typedef std::pair<find_return_type,bool> insert_return_type;
   class local_iterator : public std::iterator<std::forward_iterator_tag, std::pair<const K,V> > {
	 typedef HashEntry<K,V> *T;
	 T m_rep;
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
	typename local_iterator::value_type &operator*() const { return *(typename local_iterator::value_type *)m_rep; }
      typename local_iterator::value_type *operator->() const { return (typename local_iterator::value_type *)m_rep; }
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
	 return *(HashTable<K,V,H,P,A> *)this;
	}
	iterator begin()  {
	  return *this;
	}
   HashEntry<K,V> *end() const {
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
public:
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

//	value_type * find_value(const K &first) const { return (value_type *)find(first); }

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
  V *find_second(const K& first) const
	{
      for ( HashEntry<K,V> *p = table[bucket(first)]; p ; p = p->next )
   	if ( equal(p->first,first) )
	  return &(p->second);
      return NULL;

	}
	public:


  private:
#ifndef _MSC_VER	
	typedef typename A::rebind<HashEntry<K,V> * >::other table_alloc;

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
	HashEntry<K,V> *alloc_node() { return allocate(1); }
	void free_node(HashEntry<K,V> *p) { return deallocate(p,1); }
//		template <class _K,class _V,class _H,class _A>
//friend _V *find_second(const HashTable<_K,_V,_H,_A>& ht,const _K& first);
};

template <class K,class V,class H,class P,class A>
inline typename HashTable<K,V,H,P,A>::find_return_type *find_value(const HashTable<K,V,H,P,A>& ht,const K& first) {
  return ht.find(first);
}

template <class K,class V,class H,class P,class A>
inline V *find_second(const HashTable<K,V,H,P,A>& ht,const K& first)
{
  return ht.find_second(first);
}

template <class K,class V,class H,class P,class A>
inline V *add(HashTable<K,V,H,P,A>& ht,const K&k,const V& v=V())
{
  return ht.add(k,v);
}


template<>
struct hash<int>
{
  size_t operator()(int i) const {
	return i * 2654435767U;
  }
};

template<>
struct hash<const char *>
{
  size_t operator()(const char * s) const {
	return cstr_hash(s);
  }
};

#ifdef CUSTOMNEW
#define HASHCUSTOMNEW
#endif

#endif

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

#define END_HASH	\
};\
HASHNS_E


#endif
