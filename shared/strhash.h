#ifndef STRHASH_H
#define STRHASH_H 1
#include "config.h"
#include <iostream>

#include "assert.h"
#include "dynarray.h"
#include "2hash.h"

class StringKey {
 public:
  char *str;
  static char *empty;
  StringKey() : str(empty) {}
  StringKey(char *c) : str(c) {}
  const char *clone()
    {
      char *old = str;
      str = strcpy(NEW char[strlen(str)+1], str);
      return old;
    }
  void kill()
    {
      if (str != empty)
	delete str;
      str = empty;
    }
  operator char * () { return str; }
  char * operator =(char * c) { char *t = str; str = c; return t; } // returns old value: why?
  int operator == ( const StringKey &a ) const
    {
      return !strcmp(str, a.str);
    }
  bool isGlobalEmpty() const { return str == empty; }
  int hash() const
    {
      char *x = str;
      int h = 0;
      int g;
      while (*x != 0)
	{
	  h = (h << 4) + *x++;
	  if ((g = h & 0xf0000000) != 0)
	    h = (h ^ (g >> 24)) ^ g;
	}
      return (h >> 4);
    }
};

std::ostream & operator << (std::ostream & out, const StringKey &s);


class StringPool {
#ifdef STRINGPOOL
  static HashTable<StringKey, int> counts;
#endif
 public:
  static StringKey borrow(StringKey s) {
#ifdef STRINGPOOL
    Entry<StringKey, int> *entryP;
    if ( (entryP = counts.findEntry(s)) ) {
      (entryP->val)++;
      return (entryP->key);
    }
    s.clone();
    counts.add(s, 1);
    return s;
#else
    s.clone();
    return s;
#endif
  }
  static void giveBack(StringKey s) {
#ifdef STRINGPOOL
    Entry<StringKey, int> *entryP;
    if ( s.str != StringKey::empty && (entryP = counts.findEntry(s)) ) {
      Assert(entryP->val > 0);
      Assert(s.str == entryP->key.str);
      if ( !(--(entryP->val)) ) {
	counts.remove(s);
	s.kill();
      }
    }
#else
    s.kill();
#endif
  }
  ~StringPool()
    {
#ifdef STRINGPOOL
      for ( HashIter<StringKey, int> i(counts); i ; ++i )
	((StringKey &)i().key).kill();
#endif
    }
};


class Alphabet {
  DynamicArray<char *> names;
  HashTable<StringKey, int> ht;
 public:
  Alphabet() { }
  Alphabet(char * c) {
    add(c);
  }
  int  size() const{ // returns size of the alphabet ... added by Yaser - 7-13-2000
    return names.count();
  }
  Alphabet(const Alphabet &a) {
#ifdef NODELETE
    memcpy(this, &a, sizeof(Alphabet));
#else
    for ( int i = 0 ; i < a.names.count(); ++i )
      add(a.names[i]);
#endif
  }
  void swap(Alphabet &a)
    {
      const size_t s = sizeof(Alphabet);
      char swapTemp[s];
      memcpy(swapTemp, this, s);
      memcpy(this, &a, s);
      memcpy(&a, swapTemp, s);
    }
  void add(char *name) { 
    StringKey s = StringPool::borrow(StringKey(name));;
    ht.add(s, names.count());
    names.pushBack(s.str);
  }
  int *find(char *name) const {
    return ht.find(StringKey(name));
  }
  int indexOf(char *name) {
    Assert(name);
    int *it;
    StringKey s = name;
    if ( (it = ht.find(s)) )
      return *it;
    else {
      StringKey k = StringPool::borrow(s);
      int ret = names.count();
      ht.add(k, ret);
      names.pushBack(k.str);
      return ret;
    }
  }
  const char * operator[](int pos) const {
    //Assert(pos < count() && pos >= 0);
    return names[pos];
  }
  static char *itoa(int pos) {
    static char buf[10] = "012345678"; // to put end of string character at buf[9]
    int iNum = pos;
    // decimal string for int
    char *num;
    if ( !pos ) {
      buf[8] = '0';
      num = buf + 8;
    } else {
      int rem, i = 9;
      while ( pos ) {
	rem = pos;
	pos /= 10;
	rem -= 10*pos;		// maybe this is faster than mod because we are already dividing
	buf[--i] = '0' + (char)rem;
      }
	
      num = buf + i;
    }
    return num;
  }
  const char * operator()(int pos) {
    int iNum = pos;
    if (iNum >= count() || names[iNum] == StringKey::empty ) {
      // decimal string for int
     
      StringKey k;
      k.str = itoa(iNum);
      k = StringPool::borrow(k);
      if ( iNum < names.count() ) {
	names[iNum] = k.str;
	ht.add(k,iNum); // Yaser added this 8-3-2000: to fix what I think is a bug. Since the String key is never added to the hashtable ht.
      } else {
	for ( int i = names.count() ; i < iNum ; ++i )
	  names.pushBack(StringKey::empty);
	names.pushBack(k.str);
	ht.add(k,iNum ); // Yaser added this 8-3-2000: to fix what I think is a bug. Since the String key is never added to the hashtable ht.
	Assert(names.count() == iNum+1);
      }
    }
    return names[iNum];
  }
  void removeMarked(bool marked[], int* oldToNew) {
    for ( int i = 0 ; i < names.count() ; ++i )
      if ( names[i] != StringKey::empty ) {
	if ( marked[i] ) {
	  ht.remove(names[i]);
#ifndef NODELETE
	  StringPool::giveBack(names[i]);
#endif
	} else {
	  int *pI = ht.find(names[i]);
	  *pI = oldToNew[*pI];
	}
      }
    names.removeMarked(marked);
  }
  void mapTo(const Alphabet &o, int *aMap) const
    // aMap will give which letter in Alphabet o the letters in a 
    // correspond to, or -1 if the letter is not in Alphabet o.
    // aMap should have space for size+1 ints
    // the first (unused) slot is reserved for *e*, the empty character
    {
      int i, *ip;
      for ( i = 0 ; i < count() ; ++i )
	aMap[i] = (( ip = o.find(names[i])) ? (*ip) : -1 );
    }
  int count() const { return names.count(); }
  ~Alphabet()
    {
#ifndef NODELETE
      for ( int  i = 0; i < names.count() ; ++i )
	if ( names[i] != StringKey::empty )
	  StringPool::giveBack(names[i]);
#endif
    }
  friend std::ostream & operator << (std::ostream &out, Alphabet &alph);
};

#endif
