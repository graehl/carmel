#ifndef STRHASH_H
#define STRHASH_H 1
#include "config.h"
#include <iostream>

#include "myassert.h"
#include "dynarray.h"
#include "2hash.h"

#include "stringkey.h"

inline std::ostream & operator << (std::ostream & out, const StringKey &s)
{
  out << (char *)(StringKey)s;
  return out;
}

  static char *static_itoa(unsigned pos) {
    static char buf[] = "0123456789"; // to put end of string character at buf[10]
	static const unsigned bufsize=sizeof(buf);
	Assert(bufsize==11);    
    // decimal string for int
    char *num=buf+bufsize-1; // place at the '\0'
    if ( !pos ) {      
	  *--num='0';
    } else {
      char rem;
	  // 3digit lookup table, divide by 1000 faster?
      while ( pos ) {
		rem = pos;
		pos /= 10;
		rem -= 10*pos;		// maybe this is faster than mod because we are already dividing
		*--num = '0' + (char)rem;
      }	
    }
    return num;
  }


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
  enum {is_noop=0};
	static StringKey borrow(char *str) {
	  StringKey s(str);
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
	static void giveBack(char * str) {
	  StringKey s(str);
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


template <class Sym=StringKey,class StrPool=STRINGPOOLCLASS>
class Alphabet {
  DynamicArray<Sym> names;
  typedef HashTable<Sym, unsigned> SymIndex;
  SymIndex ht;
 public:
  Alphabet() { }
  Alphabet(Sym c) {
    add(c);
  }
  Alphabet(const Alphabet &a) {
#ifdef NODELETE
    memcpy(this, &a, sizeof(Alphabet));
#else
    for ( unsigned i = 0 ; i < a.names.size(); ++i )
      add(a.names[i]);
#endif
  }
  void dump() { 	Config::debug() << ht; }
  bool verify() {
#ifdef DEBUG
	for (unsigned i = 0 ; i < names.size(); ++i ) {
	  static char buf[1000];
	  Assert(*find(names[i])==i);
	  strcpy(buf,names[i]);
	  Assert(find(buf));
	}
#endif
	return true;
  }
  void swap(Alphabet &a)
    {
      const size_t s = sizeof(Alphabet);
      char swapTemp[s];
      memcpy(swapTemp, this, s);
      memcpy(this, &a, s);
      memcpy(&a, swapTemp, s);
    }
  void add(Sym name) { 
	Assert(find(name) == NULL);
	if (!StrPool::is_noop)
	  name = StrPool::borrow(name);
	//ht[name]=names.size();
	::add(ht,name,names.size());
    names.push_back(name);
  }
  unsigned *find(Sym name) const {
    return find_second(ht,name);
  }
  unsigned indexOf(Sym s) {
    //Assert(name);
    //Sym s = const_cast<char *>(name);

	typename SymIndex::insert_return_type it;
	if ( (it = ht.insert(SymIndex::value_type(s,names.size()))).second ) {
	  if (StrPool::is_noop)
		names.push_back(s);
	  else
		names.push_back(*const_cast<Sym*>(&(it.first->first)) = StrPool::borrow(s)); // might seem naughty, (can't change hash table keys) but it's still equal.
	}
    return it.first->second;	
  }
  Sym operator[](unsigned pos) const {
    //Assert(pos < size() && pos >= 0);
    return names[pos];
  }
  Sym operator()(int pos) {
    int iNum = pos;
    if (iNum >= size() || names[iNum] == Sym::empty ) {
      // decimal string for int
     
      Sym k = static_itoa(iNum);
	  if (!StrPool::is_noop)
		k = StrPool::borrow(k);
      if ( iNum < names.size() ) {
		names[iNum] = k;
		ht[k]=iNum;
      } else {
	for ( int i = names.size() ; i < iNum ; ++i )
	  names.push_back(Sym::empty);
	names.push_back(k);
	ht[k]=iNum;
	Assert(names.size() == iNum+1);
      }
    }
    return names[iNum];
  }
  void removeMarked(bool marked[], int* oldToNew) {
    for ( unsigned int i = 0 ; i < names.size() ; ++i )
      if ( names[i] != Sym::empty ) {
	if ( marked[i] ) {
	  ht.erase(names[i]);
#ifndef NODELETE
	  if (!StrPool::is_noop)
		StrPool::giveBack(names[i]);
#endif
	} else {
	  unsigned &rI = ht.find(names[i])->second;
	  rI = oldToNew[rI];
	}
      }
    names.removeMarked(marked);
  }
  void mapTo(const Alphabet &o, int *aMap) const
    // aMap will give which letter in Alphabet o the letters in a 
    // correspond to, or -1 if the letter is not in Alphabet o.    
  {
      unsigned int  *ip;
      for ( unsigned int i = 0 ; i < size() ; ++i )
	aMap[i] = (( ip = o.find(names[i])) ? (*ip) : -1 );
    }
   unsigned size() const { return names.size(); }
  ~Alphabet()
    {
#ifndef NODELETE
	 if (!StrPool::is_noop)
      for ( unsigned int  i = 0; i < names.size() ; ++i )
	if ( names[i] != Sym::empty )	  
		StrPool::giveBack(names[i]);
#endif
    }
	template<class T>  friend std::ostream & operator << (std::ostream &out, Alphabet<T> &alph);
};


template<class T>
inline std::ostream & operator << (std::ostream &out, Alphabet<T> &alph)
{
  for ( unsigned int i = 0 ; i < alph.names.size() ; ++i )
    out << alph.names[i] << '\n';
  return out;
}

#ifdef TEST
#include "../../tt/test.hpp"
BOOST_AUTO_UNIT_TEST( static_itoa )
{
  BOOST_CHECK(!strcmp(static_itoa(0),"0"));
  BOOST_CHECK(!strcmp(static_itoa(3),"3"));
  BOOST_CHECK(!strcmp(static_itoa(10),"10"));
  BOOST_CHECK(!strcmp(static_itoa(109),"109"));
  BOOST_CHECK(!strcmp(static_itoa(190),"190"));
  BOOST_CHECK(!strcmp(static_itoa(199),"199"));
  BOOST_CHECK(!strcmp(static_itoa(1534567890),"1534567890"));
}

BOOST_AUTO_UNIT_TEST( dummy )
{
BOOST_CHECK(true);  
}
#endif

#endif
