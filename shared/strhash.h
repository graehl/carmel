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
  out << s.c_str();
  return out;
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
	static StringKey borrow(StringKey s) {
	  if (s.isDefault())
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
	  if (s.isDefault())
		return;
		HashTable<StringKey, int>::find_return_type entryP;
		if ( !s.isDefault() && (entryP = counts.find(s))!= counts.end() ) {
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

template <class Sym>
struct NoStringPool {
  enum {is_noop=1};
  static Sym borrow(const Sym &s) {
    return s;
  }
  static void giveBack(const Sym &s) {
  }
};


// Sym must be memcpy-moveable, char * initializable (for operator () only), define == and hash<Sym>, and default initialize to == Sym::empty
template <class Sym=StringKey,class StrPool=NoStringPool<Sym> >
class Alphabet {
public:
  typedef DynamicArray<Sym> SymArray;
private:
  DynamicArray<Sym> names;
  typedef HashTable<Sym, unsigned> SymIndex;
  SymIndex ht;
 public:
   const DynamicArray<Sym> &symbols() const { return names; }
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
  template <class S,class StrP>
  bool operator ==(const Alphabet<S,StrP> &r) const {
	return r.symbols() == symbols();
  }
  void compact() {
	names.compact();
  }
  void dump() const { 	Config::debug() << ht; }
  bool verify() const {
#ifdef DEBUG
	for (unsigned i = 0 ; i < names.size(); ++i ) {
	  static char buf[1000];
	  if (!names[i].isDefault()) {
	  Assert(*find(names[i])==i);
	  strcpy(buf,names[i].c_str());
	  Assert(find(buf));
	  }
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
  void reserve(unsigned n) {
	names.reserve(n);
  }
  unsigned *find(Sym name) const {
    return find_second(ht,name);
  }
  bool is_index(unsigned pos) const {
	return pos < names.size();
  }
  unsigned index_of(Sym s) {
	return indexOf(s);
  }
  unsigned indexOf(Sym s) {
    //Assert(name);
    //Sym s = const_cast<char *>(name);

	typename SymIndex::insert_return_type it;
	if ( (it = ht.insert(typename SymIndex::value_type(s,names.size()))).second ) {
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
  // (explicit named_states flag in WFST) but is used in int WFST::getStateIndex(const char *buf)
  
  Sym operator()(unsigned pos) {
    unsigned iNum = pos;
#ifdef OLD_ALPH_NUM
	if (iNum >= size() || names[iNum] == Sym::empty ) {
#else
	if (names.at_grow(iNum) == Sym::empty) {
#endif
      // decimal string for int
     
     Sym k = static_itoa(iNum);
	 if (!StrPool::is_noop)
	   k = StrPool::borrow(k);
#ifdef OLD_ALPH_NUM
      if ( iNum < names.size() ) {
#endif
	names[iNum] = k;
	ht[k]=iNum;
	Assert(names.size() > iNum);
#ifdef OLD_ALPH_NUM
      } else {
	for ( int i = names.size() ; i < iNum ; ++i )
	  names.push_back(); //names.push_back(Sym::empty);
	names.push_back(k);
	ht[k]=iNum;
	Assert(names.size() == iNum+1);
      }
#endif
	  return k;
    }
    return names[iNum];
  }

  // syncs hashtable in preparation for deleting entries from names array
  void removeMarked(bool marked[], int* oldToNew) {
    for ( unsigned int i = 0 ; i < names.size() ; ++i )
      if ( !names[i].isDefault() ) { // FIXME:why?
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
  void giveBackAll() {
#ifndef NODELETE
	if (!StrPool::is_noop)
	  for ( typename SymArray::iterator i=names.begin(),end=names.end();i!=end;++i)
//	if ( *i != Sym::empty )	  
		StrPool::giveBack(*i);
#endif
  }
  void clear() {
	if (size() > 0) {
	giveBackAll();
	names.clear();
	ht.clear();
	}
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
	  giveBackAll();
    }
	template<class T,class P>  friend std::ostream & operator << (std::ostream &out, Alphabet<T,P> &alph);
};


template<class T,class P>
inline std::ostream & operator << (std::ostream &out, Alphabet<T,P> &alph)
{
  for ( unsigned int i = 0 ; i < alph.names.size() ; ++i )
    out << alph.names[i] << '\n';
  return out;
}

#ifdef TEST
#include "../../tt/test.hpp"
BOOST_AUTO_UNIT_TEST( TEST_static_itoa )
{
  BOOST_CHECK(!strcmp(static_itoa(0),"0"));
  BOOST_CHECK(!strcmp(static_itoa(3),"3"));
  BOOST_CHECK(!strcmp(static_itoa(10),"10"));
  BOOST_CHECK(!strcmp(static_itoa(109),"109"));
  BOOST_CHECK(!strcmp(static_itoa(190),"190"));
  BOOST_CHECK(!strcmp(static_itoa(199),"199"));
  BOOST_CHECK(!strcmp(static_itoa(1534567890),"1534567890"));
}

#endif


#ifdef MAIN
#include "strhash.cc"
#endif

#endif
