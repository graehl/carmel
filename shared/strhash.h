#ifndef STRHASH_H
#define STRHASH_H 1
#include "config.h"
#include <iostream>

#include "assert.h"
#include "dynarray.h"
#include "2hash.h"

#include "stringkey.h"

std::ostream & operator << (std::ostream & out, const StringKey &s);


class StringPool {
  static char * clone(const char *str) {
	return strcpy(NEW char[strlen(str)+1], str);
  }
  static void kill(const char *str) {
	delete str;
  }
#ifdef STRINGPOOL
	static HashTable<StringKey, int> counts;
#endif
public:
	static StringKey borrow(StringKey s) {
	  if (s.str == StringKey::empty)
		return s;
#ifdef STRINGPOOL
	  HashTable<StringKey, int>::find_return_type entryP;
		if ( (entryP = counts.find(s)) != counts.end() ) {
			(entryP->second)++;
			return (entryP->first);
		}
		s.clone();
		counts[s]=1;
		return s;
#else
		s.clone();
		return s;
#endif
	}
	static void giveBack(StringKey s) {
#ifdef STRINGPOOL
		HashTable<StringKey, int>::find_return_type entryP;
		if ( s.str != StringKey::empty && (entryP = counts.find(s))!= counts.end() ) {
			Assert(entryP->second > 0);
			Assert(s.str == entryP->first.str);
			if ( !(--(entryP->second)) ) {
				counts.erase(s);
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
	  for ( HashTable<StringKey, int>::iterator i=counts.begin(); i!=counts.end() ; ++i )
			((StringKey &)i->first).kill();
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
  Alphabet(const Alphabet &a) {
#ifdef NODELETE
    memcpy(this, &a, sizeof(Alphabet));
#else
    for ( int i = 0 ; i < a.names.size(); ++i )
      add(a.names[i]);
#endif
  }
  void dump();
  void verify() {
#ifdef DEBUG
	for (int i = 0 ; i < names.size(); ++i ) {
	  static char buf[1000];
	  Assert(*find(names[i])==i);
	  strcpy(buf,names[i]);
	  Assert(find(buf));
	}
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
	Assert(find(name) == NULL);
    StringKey s = StringPool::borrow(StringKey(name));;
	ht[s]=names.size();
    names.push_back(s.str);
  }
  int *find(char *name) const {
    return find_second(ht,(StringKey)name);
  }
  int indexOf(const char *name) {
    Assert(name);
    StringKey s = const_cast<char *>(name);
//#define OLD_INDEXOF
#ifdef OLD_INDEXOF
  int *it;  
	if ( (it = find_second(ht,s)) )
      return *it;
    else {
      StringKey k = StringPool::borrow(s);
      int ret = names.size();
      add(ht,k,ret);
      names.push_back(k.str);
      return ret;
    }
#else
	HashTable<StringKey,int>::insert_return_type it;
	if ( (it = ht.insert(HashTable<StringKey,int>::value_type(s,names.size()))).second )
	  names.push_back(*const_cast<StringKey*>(&(it.first->first)) = StringPool::borrow(s)); // might seem naughty, (can't change hash table keys) but it's still equal.		
    return it.first->second;	
#endif
  }
  const char * operator[](int pos) const {
    //Assert(pos < size() && pos >= 0);
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
    if (iNum >= size() || names[iNum] == StringKey::empty ) {
      // decimal string for int
     
      StringKey k;
      k.str = itoa(iNum);
      k = StringPool::borrow(k);
      if ( iNum < names.size() ) {
		names[iNum] = k.str;
		ht[k]=iNum;
      } else {
	for ( int i = names.size() ; i < iNum ; ++i )
	  names.push_back(StringKey::empty);
	names.push_back(k.str);
	ht[k]=iNum;
	Assert(names.size() == iNum+1);
      }
    }
    return names[iNum];
  }
  void removeMarked(bool marked[], int* oldToNew) {
    for ( int i = 0 ; i < names.size() ; ++i )
      if ( names[i] != StringKey::empty ) {
	if ( marked[i] ) {
	  ht.erase(names[i]);
#ifndef NODELETE
	  StringPool::giveBack(names[i]);
#endif
	} else {
	  int &rI = ht.find(names[i])->second;
	  rI = oldToNew[rI];
	}
      }
    names.removeMarked(marked);
  }
  void mapTo(const Alphabet &o, int *aMap) const
    // aMap will give which letter in Alphabet o the letters in a 
    // correspond to, or -1 if the letter is not in Alphabet o.    
  {
      int i, *ip;
      for ( i = 0 ; i < size() ; ++i )
	aMap[i] = (( ip = o.find(names[i])) ? (*ip) : -1 );
    }
  int size() const { return names.size(); }
  ~Alphabet()
    {
#ifndef NODELETE
      for ( int  i = 0; i < names.size() ; ++i )
	if ( names[i] != StringKey::empty )
	  StringPool::giveBack(names[i]);
#endif
    }
  friend std::ostream & operator << (std::ostream &out, Alphabet &alph);
};


#endif
