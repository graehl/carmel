#ifndef TWO_HASH_H 
#define TWO_HASH_H
#include "config.h"

#include <iostream>
#include <cstdlib>
#include <new>
#include "assert.h"
#include <utility>

const float DEFAULTHASHLOAD = .8f;
template <typename K, typename V> class HashTable ;
template <typename K, typename V> class HashIter ;
//template <typename K, typename V> class HashConstIter ;

int pow2Bound(int request);

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
private:
   HashEntry<K,V> *next;
public:
  /*  HashEntry() : next(NULL) { }*/
  HashEntry(const K & k, const V& v) : next(NULL), first(k), second(v) { }
  HashEntry(const K &k, const V& v, HashEntry<K,V> * const n) : next(n), first(k), second(v) { }
  HashEntry(const K &k, HashEntry<K,V> * const n) : next(n), first(k), second() { }
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
  friend class HashTable<K,V>;
  friend class HashIter<K,V>;
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

#if 0
template <typename K, typename V>
				  std::ostream & operator << (std::ostream & o, const HashEntry<K,V> & e);
#endif

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
    if ( ht->count() > 0 ) {
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

     void init(HashTable<K,V> &t) {
    
	end_bucket = t.table + t.size();
    if ( t.count() > 0 ) {
      bucket = t.table;
      while ( !*bucket ) bucket++;
      entry = *bucket++;      
	} else  {
	  bucket=end_bucket=NULL;
      entry = NULL;
	}
  }

  HashIter( HashTable<K,V> &t)
    {
      init(t);
    }
   HashIter( )  {}
/*
  HashIter( const HashTable<K,V> &t) : ht(t)
    {
      if ( ht.count() > 0 ) {
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

template <typename K, typename V> class HashTable {
 protected:
  int siz;  // always a power of 2, stored as 1 less than actual for fast mod operator (implemented as and)
  int cnt;
  int growAt;
  HashEntry<K,V> **table;
  int hashToPos(int hashVal) const
    {
      return hashVal & siz;
    }
 public:
   typedef HashIter<K,V> iterator;
   typedef HashIter<K,V> const_iterator;

   typedef std::pair<K,V> value_type;
   typedef K key_type;	
   typedef HashEntry<K,V> *pair_pointer;
   typedef V second_type;
   typedef std::pair<pair_pointer,bool> insert_pair;

    const_iterator begin() const {
	 return *(HashTable<K,V> *)this;
	}
	iterator begin()  {
	  return *this;
	}
   HashEntry<K,V> *end() const {
	 return NULL;
   }
private:
  HashTable(const HashTable<K,V> &ht)
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
      table = NEW HashEntry<K,V>*[siz+1];
      for ( int i = 0 ; i <= siz ; i++ ) 
	table[i] = NULL;
      for(const_iterator k=ht.begin() ; k != ht.end() ; ++k)
	add(k->first,k->second);
      //    std::cerr <<"done\n";
    } 
public:
  void swap(HashTable<K,V> &h)
    {
      const size_t s = sizeof(HashTable<K,V>)/sizeof(char) + 1;
      char temp[s];
      memcpy(temp, this, s);
      memcpy(this, &h, s);
      memcpy(&h, temp, s);
    }
  HashTable(int sz = 4, float mLoad = DEFAULTHASHLOAD)
    {
      if ( sz < 4 )
	siz = 4;
      else
	siz = pow2Bound(sz);
      cnt = 0;
      growAt = (int)(mLoad * siz);
      if ( growAt < 2 )
	growAt = 2;
      siz--;   // size is actually siz + 1
      table = NEW HashEntry<K,V>*[siz+1];
      for ( int i = 0 ; i <= siz ; i++ ) 
	table[i] = NULL;
    }
  ~HashTable()
    {
      //    std::cerr << "HashTable destructor called\n"; // Yaser
      if ( table ) {

		for ( iterator i=begin(); i!=end() ; ) {// const_iterator would delete the next pointer before we use it
		  HashEntry<K,V> *entry=&*i;
		  ++i;
		  delete entry;
		}
	delete[] table;
	table = NULL;
	siz = 0;
      }
    }
  V *add(const K &first, const V &second=V())
    {
      if ( ++cnt >= growAt )
		rehash(2 * siz);
      int i = hashToPos(first.hash());
      table[i] =  NEW HashEntry<K,V>(first, second, table[i]);
      return &table[i]->second;
    }	
	/*
  V * add(const K& first)
    {
      if ( ++cnt >= growAt )
	rehash(2 * siz);
      int i = hashToPos(first.hash());
      table[i] =  NEW HashEntry<K,V>(first, table[i]);
      return &table[i]->second;
    }
	*/
	// bool is true if insertion was performed, false if key already existed.  pointer to the key/val pair in the table is returned
	insert_pair insert(const K& first, const V& second=V()) {
	  int hash=first.hash();
	  int bucket=hashToPos(hash);
	  for ( HashEntry<K,V> *p = table[bucket]; p ; p = p->next )
		if ( p->first == first )
		  return std::pair<pair_pointer,bool>((pair_pointer)p,false);
 
	  if ( ++cnt >= growAt ) {
		rehash(2 * siz);
		bucket=hashToPos(hash);
	  }
	  
	  return std::pair<pair_pointer,bool>(
		reinterpret_cast<pair_pointer>(table[bucket] =  NEW HashEntry<K,V>(first, second, table[bucket]))
		,true);
	  
	}
	insert_pair insert(const value_type &t) { 
	  return insert(t.first,t.second);
	}

  pair_pointer find(const K &first) const
    {
      for ( HashEntry<K,V> *p = table[hashToPos(first.hash())]; p ; p = p->next )
	if ( p->first == first )
	  return reinterpret_cast<pair_pointer>(p);
      return NULL;
    }
	second_type * find_second(const K &first) const
	{
      for ( HashEntry<K,V> *p = table[hashToPos(first.hash())]; p ; p = p->next )
   	if ( p->first == first )
	  return &(p->second);
      return NULL;
	  /* alt:
	  pair_pointer p=find(first);
	  if (p)
		return &(p->second);
	  return p;
	  */

	}
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
    }/*
  HashEntry<K,V> * findEntry(const K &first) const
    {
      for ( HashEntry<K,V> *p = table[hashToPos(first.hash())]; p ; p = p->next )
	if ( p->first == first )
	  return p;
      return NULL;
    } 
	*/
  int erase(const K &first)
    {
      int i = hashToPos(first.hash());
      HashEntry<K,V> *prev = NULL, *p = table[i];
      if ( !p ) 
	return 0;
      else if ( p->first == first ) {
	table[i] = p->next;
	delete p;
	--cnt;
	return 1;
      }
      for ( ; ; ) {
	prev = p;    
	p = p->next;
	if ( !p ) break;
	if ( p->first == first ) {
	  prev->next = p->next;
	  delete p;
	  --cnt;
	  return 1;
	}
      }
      return 0;
    }
  int size() const { return siz + 1; }
  int count() const { return cnt; } 
  int growWhen() const { return growAt; }
  float load_factor() const { return (float)cnt / (float)(siz + 1); }
  float max_load_factor() const { return (float)growAt / (float)(siz + 1); }
  void max_load_factor(float mLoad) 
    {
      growAt = (int)(mLoad * (siz+1));
      if ( growAt < 2 )
	growAt = 2;
    }
  void rehash(int request)
    {
      int hashVal, oldSiz = siz;
      HashEntry<K,V> *next, *p, **i, **oldTable = table;
      siz = pow2Bound(request);
      table = NEW HashEntry<K,V>*[siz];
      siz--;  // actual size is siz + 1 (power of 2)
      for ( i = table; i <= table + siz ; i++ )
	*i = NULL;
      for ( i = oldTable ; i <= oldTable + oldSiz ; i++ )
	for ( p = *i ; p ; p = next ) {
	  next = p->next;
	  hashVal = hashToPos(p->first.hash());
	  p->next = table[hashVal];
	  table[hashVal] = p;
	}
      growAt = int((float(growAt) * (siz+1)) / (oldSiz+1));
      delete[] oldTable;
    }
  friend class HashIter<K,V>;
  //friend class HashConstIter<K,V>;
};


#endif
