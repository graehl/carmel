#define BOOST_AUTO_TEST_MAIN
#define MAIN

//#include "config.h"
//#include "ttconfig.hpp"

#include "2hash.h"


#include <boost/progress.hpp>
#include <boost/unordered_map.hpp>
#include <iostream>
using namespace std;

#define MASK(a) ((a) & M)

template<class T>
struct int_hash
{
  size_t operator()(T key) const {
//	return (size_t)key;

/*	  key += (key << 12);
      key ^= (key >> 22);
      key += (key << 4);
      key ^= (key >> 9);
      key += (key << 10);
      key ^= (key >> 2);
      key += (key << 7);
      key ^= (key >> 12);
      return key;
*/

/*	  key -= (key << 15);
      key ^=  (key >> 10);
      key +=  (key << 3);
      key ^=  (key >> 6);
      key -= (key << 11);
      key ^=  (key >> 16);
      return key;*/

/*	 const int c1 = 0xd2d84a61;
     const int c2 = 0x7832c9f4;
     key *= c1;
     key ^= (key < 0) ? c2 : (0x7ffffffff ^ c2); // 1 -> 32 Sbox
     return key;
*/
	key *= 2654435769U;
	key ^= (key >> 16);
	return key;
  }
};

int N= 1 << 20;
int M=0x00FFFFFF;
int D=16;
int S=31;


template <class H,class I>
void hash_bench(H &ht) {
  int i,j=0;
  boost::progress_timer t;
  cout << "build table ";
  {
    boost::progress_timer t;
    i=0;for (j=0;j<N;++j) { i=i*S+j;
      ht[MASK(i)]=j;
    }
  }
  cout << "standard find ";
  {
    boost::progress_timer t;
    i=0;for (j=0;j<N;++j) { i=i*S+j;
      I f=ht.find(MASK(i));
      if (f != ht.end())
        ++f->second;
    }
  }
  cout << "build+find ";
}

template <class H>
void hash_bench() {
  H ht(D);
  hash_bench<H,typename H::iterator>(ht);
}

template <class H>
void graehl_hash_bench() {
  H ht(D);
  hash_bench<H,typename H::find_return_type>(ht);
  cout << "find_second ";
  {
    boost::progress_timer t;
    for (int i=0,j=0;j<N;++j) { i=i*S+j;
      if (int *f=ht.find_second(MASK(i)))
        ++*f;
    }
  }
}

//#include <sstream>
int main(int argc, char *argv[])
{
  int i,j;
  if (argc > 1) {
//	string s=argv[1];
//	istringstream(s) >> M;
	//M=atoi(argv[1]);
	sscanf(argv[1],"%x",&M);
  }
  if (argc > 2) {
	D=atoi(argv[2]);
  }
  if (argc > 3) {
	S=atoi(argv[3]);
  }
  if (argc > 4) {
	N=atoi(argv[4]);
  }
  cout << hex << "Mask=" << M << dec << " Defaultsize=" << D << " N=" << N << " stride=" << S << endl;
  cout << "\nboost_defaulthash\n";
  {
    typedef boost::unordered_map<int,int > H;
    hash_bench<H>();
    H ht(D);
  }

  cout << "\nboost int hash\n";
  {
    typedef boost::unordered_map<int,int, int_hash<int> > H;
    hash_bench<H>();
  }

  //typedef stdext::hash_map<int,int> H;
  {
    typedef graehl::HashTable<int,int,int_hash<int> > H;
    cout << "2hash int_hash\n";
    graehl_hash_bench<H>();
  }



  {
    typedef graehl::HashTable<int,int > H;
    cout << "2hash default hash\n";
    graehl_hash_bench<H>();
  }


  return 0;
}

