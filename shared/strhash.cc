#ifndef _STRHASH_CC
#define _STRHASH_CC

#define BOOST_AUTO_TEST_MAIN

#include "strhash.h"


void dump_ht(HashTable<StringKey,int> &ht)
{
  Config::debug() << ht;

}



#ifdef STRINGPOOL
HashTable<StringKey, int> StringPool::counts;
#endif

#include "stringkey.cc"

#ifdef TEST
struct collide_int {
#ifndef STATIC_HASHER
  int mask;

  collide_int() : mask(-1) {}
 #endif
  //collide_int(int m) : mask(m) {}
  size_t operator()(int i) const {
#ifndef STATIC_HASHER
	return i&mask;
#else
	return i;
#endif
  }
};

typedef HashTable<int,int,collide_int> HT;
void dump_ht(HT &ht)
{
  Config::debug() << ht;
}

#include "../../tt/test.hpp"
BOOST_AUTO_UNIT_TEST( alphabet )
{
  Alphabet<StringKey,StringPool> a;
  Alphabet<StringKey,StringPool> b;
  const char *s[]={"u","ul","mu","pi"};
  const int n=sizeof(s)/sizeof(s[0]);
  a.add("a");
  for (int i=0;i<n;++i)
	BOOST_CHECK(a.indexOf(s[i]) == b.indexOf(s[i])+1);
  a.verify();
  b.verify();
  
}
#ifdef BENCH
#include <boost/progress.hpp>
#endif

  static void hashtest(int n, int mask)
  {	
	n *= 2;
#ifdef BENCH
	Config::log() << n << " with mask " << mask << "\n";
#endif
	collide_int hashfn;
		typedef HashTable<int,int,collide_int> HT;
#ifndef STATIC_HASHER
	hashfn.mask=mask;
#endif
	HT ht(n/16,hashfn);
#ifndef STATIC_HASHER
	BOOST_CHECK(ht.hash_function().mask == hashfn.mask);
#endif
	BOOST_CHECK(ht.bucket_count() >= (unsigned)n/16);
	bool *seen=new bool[n];
	{ 
#ifdef BENCH
	  boost::progress_timer t;
#endif
	int i;
	// n must be even
	{
#ifdef BENCH
	Config::log() << "add ";

	  boost::progress_timer t;
#endif
	  for (i=0; i <n; ++i) {

	  add(ht,i,i);
	  BOOST_CHECK(ht.find(i) != ht.end());
	}
	}
	bool first=true;
again:
	if (!first) {
#ifdef BENCH
	  Config::log() << "total ";
#endif
	  return;
	} else
	  first=false;
	BOOST_CHECK(ht.size() == n);
	#ifdef BENCH
	Config::log() << "find ";
#endif
	{ 
#ifdef BENCH
	  boost::progress_timer t;
#endif
	for (i=0; i <n; ++i) {
	  BOOST_CHECK(ht.find(i) != ht.end());
	  BOOST_CHECK(ht.find(i)->first == i);
	  BOOST_CHECK(ht.find(i)->second == i);
	  BOOST_CHECK(ht.insert(std::pair<int,int>(i,0)).second == false);	  
	  BOOST_CHECK(*find_second(ht,i) == i);
	  
	  BOOST_CHECK(ht[i] == i);
	}
	}
	for (i=0; i <n; ++i)
	  seen[i]=false;
	 { 
#ifdef BENCH
	   Config::log() << "it " << (unsigned)ht.bucket_count() << " ";
	  boost::progress_timer t;
#endif
	for (HT::iterator hit=ht.begin();hit!=ht.end();++hit) {
	  int k=hit->first;
	  BOOST_CHECK(hit->second==k);
	  BOOST_CHECK(k<n && k>=0);
	  seen[k]=true;
	}
	}
	for (i=0; i <n; ++i)
	  BOOST_CHECK(seen[i]=true);

	for (i=0; i <n; ++i)
	  seen[i]=false;
	size_t total=0;
	for (i=0;i<(int)ht.bucket_count();++i) {
	  total += ht.bucket_size(i);
	  for (HT::local_iterator hit=ht.begin(i),end=ht.end(i);hit!=ht.end(i);++hit) {
		int k=hit->first;
		BOOST_CHECK(hit->second==k);
		BOOST_CHECK(k<n && k>=0);
		seen[k]=true;
	  }
	}
	BOOST_CHECK(ht.size()==total);
	for (i=0; i <n; ++i)
	  BOOST_CHECK(seen[i]=true);

{ 
#ifdef BENCH
Config::log() << "erase ";	
  boost::progress_timer t; 
#endif
	BOOST_CHECK(ht.size() == n);
	for (i=0; i <n; i+=2) {
	  ht.erase(i);
	  BOOST_CHECK(ht.find(i) == ht.end());
	}
}
	BOOST_CHECK(ht.size() == n/2);
	for (i=0; i <n; i++) {
	  if ( i % 2)
		BOOST_CHECK(ht.find(i) != ht.end());
	  else
		BOOST_CHECK(ht.find(i) == ht.end());
	}
	ht.clear();
	BOOST_CHECK(ht.size() == 0);

	for (i=0; i <n; ++i) {
	  BOOST_CHECK(ht.find(i) == ht.end());
	  BOOST_CHECK(find_second(ht,i) == NULL);
	  if ( i % 2) {
	  HT::insert_return_type insr=ht.insert(std::pair<int,int>(i,i));	  
	  BOOST_CHECK(insr.second == true);
	  BOOST_CHECK(insr.first->first == i);
	  BOOST_CHECK(insr.first->second == i);
	  } else
		ht[i]=i;
	  BOOST_CHECK(ht.find(i) != ht.end());
	}

	goto again;	
  }
	delete[] seen;
  }

  BOOST_AUTO_UNIT_TEST( hashtable )
{
#ifdef BENCH
    Config::log() << "sizeof hashtable = " << sizeof(HT) << "\n";
#endif
  hashtest(0x100,0xFF);
  hashtest(0x100,0);
  hashtest(0x100,0x7F);

  }

#endif

#endif
