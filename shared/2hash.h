#ifndef TWO_HASH_H 
#define TWO_HASH_H
#include "config.h"

#include <iostream>
#include <cstdlib>
#include <new>
#include "assert.h"

const FLOAT_TYPE DEFAULTHASHLOAD = .8f;
template <typename K, typename V> class HashTable ;
template <typename K, typename V> class HashIter ;
template <typename K, typename V> class HashConstIter ;

int pow2Bound(int request);

inline int pow2Bound(int request) {
  Assert(request < (2 << 29));
  int mask = 2;
  for ( ; mask < request ; mask <<= 1 ) ;
  return mask;
}

template <typename K, typename V> class Entry {
  Entry<K,V> *next;
  Entry operator =(Entry &);	//disallow
  //operator =(Entry &);	//disallow
 public:
  const K key;
  V val;
  /*  Entry() : next(NULL) { }*/
  Entry(const K & k, const V& v) : next(NULL), key(k), val(v) { }
  Entry(const K &k, const V& v, Entry<K,V> * const n) : next(n), key(k), val(v) { }
  Entry(const K &k, Entry<K,V> * const n) : next(n), key(k), val() { }
#ifdef CUSTOMNEW
  static Entry<K,V> *freeList;
  static const int newBlocksize;
  void *operator new(size_t s)
    {
      size_t dummy = s;
      dummy = dummy;
      Entry<K,V> *ret, *max;
      if (freeList) {
	ret = freeList;
	freeList = freeList->next;
	return ret;
      }
      freeList = (Entry<K,V> *)::operator new(newBlocksize * sizeof(Entry<K,V>));
      freeList->next = NULL;
      max = freeList + newBlocksize -1;
      for ( ret = freeList++; freeList < max ; ret = freeList++ )
	freeList->next = ret;
      return freeList--;
    }
  void operator delete(void *p) 
    {
      Entry<K,V> *e = (Entry<K,V> *)p;
      e->next = freeList;
      freeList = e;
    }
#endif
  friend class HashTable<K,V>;
  friend class HashIter<K,V>;
  friend class HashConstIter<K,V>; // Yaser
#if __GNUC__== 2 && __GNUG__== 2  && __GNUC_MINOR__ <= 7
  // version 2.7.2 or older of gcc compiler does not understand '<>' so it will give
  // an error message if '<>' is present. However, it is required by newer versions
  // of the compiler and if it is not present, a warning will be given 
  friend std::ostream & operator << (std::ostream &, const Entry<K,V> &);
#else
  friend std::ostream & operator << <> (std::ostream &, const Entry<K,V> &);
#endif
};

template <typename K, typename V>
				  std::ostream & operator << (std::ostream & o, const Entry<K,V> & e);

template <typename K, typename V> class HashIter {
  HashTable<K,V> *ht;
  Entry<K,V> **bucket;
  Entry<K,V> *entry;
  Entry<K,V> *next;
  HashIter operator = ( HashIter & );
  Entry<K,V> * operator & () const { return entry; }
  //     operator = ( HashIter & );		// disable
 public:
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
  int operator++()
    {
      if ( entry == NULL )
	return 0;
      if ( next ) {
	entry = next;
	next = entry->next;
	return 1;
      }
      for ( ; ; ) {
	if ( bucket >= ht->table + ht->size() ) {
	  entry = NULL;
	  return 0;
	}
	if ( (entry = *bucket++) ) {
	  next = entry->next;
	  return 1;
	}
      }
    }
  int bucketNum() const { return int(bucket - ht->table - 1); }
  operator bool() const { return ( entry != NULL ); }
  Entry<K,V> & operator ()() const { return *entry; }

  const K & key() const { return entry->key; }
  V & val() const { return entry->val; }
  void remove() {	/* could be more efficient */
    const K &k = key();
    this->operator++();
    ht->remove(k);
  }
};

template <typename K, typename V> class HashConstIter { // Yaser added this - 7-27-2000
  const HashTable<K,V> &ht;
  Entry<K,V> ** bucket;
  const Entry<K,V> *entry;
  const Entry<K,V> *next;
  HashConstIter operator = ( HashConstIter & );		// disable
  //     operator = ( HashIter & );		// disable
 public:
  HashConstIter( const HashTable<K,V> &t) : ht(t)
    {
      if ( ht.count() > 0 ) {
	bucket = ht.table;
	while ( !*bucket ) bucket++;
	entry = *bucket++;
	next = entry->next;
      } else 
	entry = NULL;
    }
  int operator++()
    {
      if ( entry == NULL )
	return 0;
      if ( next ) {
	entry = next;
	next = entry->next;
	return 1;
      }
      for ( ; ; ) {
	if ( bucket >= ht.table + ht.size() ) {
	  entry = NULL;
	  return 0;
	}
	if ( (entry = *bucket++) ) {
	  next = entry->next;
	  return 1;
	}
      }
    }
  int bucketNum() const { return int(bucket - ht.table - 1); }
  operator int() const { return ( entry != NULL ); }
  const Entry<K,V> & operator ()() const { return *entry; }
  const Entry<K,V> * operator & () const { return entry; }
  const K & key() const { return entry->key; }
  const V & val() const { return entry->val; }
};

template <typename K, typename V> class HashTable {
 protected:
  int siz;  // always a power of 2, stored as 1 less than actual for fast mod operator (implemented as and)
  int cnt;
  int growAt;
  Entry<K,V> **table;
  int hashToPos(int hashVal) const
    {
      return hashVal & siz;
    }
 public:
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
      table = NEW Entry<K,V>*[siz+1];
      for ( int i = 0 ; i <= siz ; i++ ) 
	table[i] = NULL;
      for(HashConstIter<K,V> k(ht) ; k ; ++k)
	add(k.key(),k.val());
      //    std::cerr <<"done\n";
    } 
  void swap(HashTable<K,V> &h)
    {
      const size_t s = sizeof(HashTable<K,V>)/sizeof(char) + 1;
      char temp[s];
      memcpy(temp, this, s);
      memcpy(this, &h, s);
      memcpy(&h, temp, s);
    }
  HashTable(int sz = 4, FLOAT_TYPE mLoad = DEFAULTHASHLOAD)
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
      table = NEW Entry<K,V>*[siz+1];
      for ( int i = 0 ; i <= siz ; i++ ) 
	table[i] = NULL;
    }
  ~HashTable()
    {
      //    std::cerr << "HashTable destructor called\n"; // Yaser
      if ( table ) {
	for ( HashIter<K,V> i(*this); i ; ++i )
	  delete &(i());
	delete[] table;
	table = NULL;
	siz = 0;
      }
    }
  int safeAdd(const K &key, const V &val)
    {
      if ( find(key) )
	return 0;
      add(key, val);
      return 1;
    }
  V *add(const K &key, const V &val)
    {
      if ( ++cnt >= growAt )
	resize(2 * siz);
      int i = hashToPos(key.hash());
      table[i] =  NEW Entry<K,V>(key, val, table[i]);
      return &table[i]->val;
    }
  V * add(const K& key)
    {
      if ( ++cnt >= growAt )
	resize(2 * siz);
      int i = hashToPos(key.hash());
      table[i] =  NEW Entry<K,V>(key, table[i]);
      return &table[i]->val;
    }
  V * find(const K &key) const
    {
      for ( Entry<K,V> *p = table[hashToPos(key.hash())]; p ; p = p->next )
	if ( p->key == key )
	  return &p->val;
      return NULL;
    }
  V * findOrAdd(const K &key)
    {
      V *ret;
      if ( (ret = find(key)) )
	return ret;
      return add(key);
    }
  V & operator[](const K &key)
    {
      return *findOrAdd(key);
    }
  Entry<K,V> * findEntry(const K &key) const
    {
      for ( Entry<K,V> *p = table[hashToPos(key.hash())]; p ; p = p->next )
	if ( p->key == key )
	  return p;
      return NULL;
    } 
  int remove(const K &key)
    {
      int i = hashToPos(key.hash());
      Entry<K,V> *prev = NULL, *p = table[i];
      if ( !p ) 
	return 0;
      else if ( p->key == key ) {
	table[i] = p->next;
	delete p;
	--cnt;
	return 1;
      }
      for ( ; ; ) {
	prev = p;    
	p = p->next;
	if ( !p ) break;
	if ( p->key == key ) {
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
  FLOAT_TYPE threshold() const { return (FLOAT_TYPE)growAt / (FLOAT_TYPE)(siz + 1); }
  void changeThreshold(FLOAT_TYPE mLoad) 
    {
      growAt = (int)(mLoad * (siz+1));
      if ( growAt < 2 )
	growAt = 2;
    }
  void resize(int request)
    {
      int hashVal, oldSiz = siz;
      Entry<K,V> *next, *p, **i, **oldTable = table;
      siz = pow2Bound(request);
      siz--;  // actual size is siz + 1 (power of 2)
      table = NEW Entry<K,V>*[siz+1];
      for ( i = table; i <= table + siz ; i++ )
	*i = NULL;
      for ( i = oldTable ; i <= oldTable + oldSiz ; i++ )
	for ( p = *i ; p ; p = next ) {
	  next = p->next;
	  hashVal = hashToPos(p->key.hash());
	  p->next = table[hashVal];
	  table[hashVal] = p;
	}
      growAt = int((FLOAT_TYPE(growAt) * (siz+1)) / (oldSiz+1));
      delete[] oldTable;
    }
  friend class HashIter<K,V>;
  friend class HashConstIter<K,V>;
};


#endif
