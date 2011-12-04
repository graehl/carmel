#define BOOST_AUTO_TEST_MAIN
#define MAIN
#define HAVE_GOOGLE_DENSE_HASH_MAP
#define HAVE_SBMT
//#include "config.h"
//#include "ttconfig.hpp"

#include <graehl/shared/hash.hpp>
#include <graehl/shared/2hash.h>
#ifdef HAVE_SBMT
# include <sbmt/hash/oa_hashtable.hpp>
#endif
#ifdef HAVE_GOOGLE_DENSE_HASH_MAP
#include <google/dense_hash_map>
#include <google/sparse_hash_map>
#endif
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
void hash_bench_it(H &ht) {
  hash_bench<H,typename H::iterator>(ht);
}

template <class H>
void hash_bench() {
  H ht(D);
  hash_bench_it(ht);
}

template <class H>
void hash_bench(std::string banner) {
  cout <<endl<<banner<<endl;
  hash_bench<H>();
}

template <class H>
void graehl_hash_bench() {
  H ht(D);
  hash_bench<H,typename H::find_result_type>(ht);
  cout << "find_second ";
  {
    boost::progress_timer t;
    for (int i=0,j=0;j<N;++j) { i=i*S+j;
      if (int *f=ht.find_second(MASK(i)))
        ++*f;
    }
  }
}

template <class H>
void google_hash_bench() {
  H ht(D);
  ht.set_empty_key(-1);
  ht.set_deleted_key(-2);
  hash_bench_it(ht);
}

template <class H>
void google_hash_bench(std::string banner) {
  cout <<endl<<banner<<endl;
  google_hash_bench<H>();
}

template <class H>
void graehl_hash_bench(std::string banner) {
  cout <<endl<<banner<<endl;
  graehl_hash_bench<H>();
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

#ifdef HAVE_GOOGLE_DENSE_HASH_MAP
  google_hash_bench<google::dense_hash_map<int,int> >("google::dense_hash_map");
  google_hash_bench<google::dense_hash_map<int,int,int_hash<int> > >("google::dense_hash_map<..int_hash...>");
#endif

  hash_bench<google::sparse_hash_map<int,int> >("google::sparse_hash_map");

  hash_bench<sbmt::oa_hash_map<int,int> >("sbmt default hash");

  hash_bench<sbmt::oa_hash_map<int,int,int_hash<int> > >("sbmt int_hash");

  hash_bench<boost::unordered_map<int,int> >("boost default hash");

  hash_bench<boost::unordered_map<int,int,int_hash<int> > >("boost int_hash");

  hash_bench<stdext::hash_map<int,int> >("gnu_cxx default hash");

  hash_bench<stdext::hash_map<int,int,int_hash<int> > >("gnu_cxx int_hash");
  //typedef stdext::hash_map<int,int> H;
  {
    graehl_hash_bench<graehl::HashTable<int,int,int_hash<int> > >("2hash int_hash");
  }
  {
    graehl_hash_bench<graehl::HashTable<int,int> >("2hash default hash");
  }
  return 0;
}

