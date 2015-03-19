// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

    implementation for random.hpp, all in terms of random01(). for threads, include this inside a class with a
   random0n member. otherwise, you can use globals or std::rand
*/

inline double random0n(double n)  // random from [0..n)
{
  return n * random01();
}


inline double random_pos_fraction()  // returns uniform random number on (0..1]
{
#ifdef USE_STD_RAND
  return ((double)std::rand() + 1.) * (1. / ((double)RAND_MAX + 1.));
#else
  return 1. - random01();
#endif
}

template <class V1, class V2>
inline V1 random_half_open(const V1& v1, const V2& v2) {
  return v1 + random01() * (v2 - v1);
}

struct std_rand {
  unsigned operator()() const { return std::rand(); }
};

/// \return on [0, limit) from unigned rand()
template <class Rand>
unsigned random_less_than(unsigned limit, Rand rand)
{
  unsigned min =
  // set min = 2^32 % limit
#if (UINT_MAX > 0xffffffffUL)
      0x100000000UL % limit;
#else
  min = limit > 0x80000000 ? 1 + ~limit : ((0xffffffff - (limit * 2)) + 1) % limit;
#endif
  for (;;) {
    unsigned r = rand();
    if (r >= min)
      return r % limit;
  }
}

inline unsigned random_less_than(unsigned limit) {
  if (limit <= 1) return 0;
#if USE_STD_RAND
  return random_less_than(limit, std_rand());
#else
  return (unsigned)(random01() * limit);
#endif
}

inline bool random_bool() {
  return random_less_than(2);
}

template <class Int>
inline Int random_up_to(Int limit) {
#if USE_STD_RAND
  if (limit == RAND_MAX) return (Int)std::rand();
#endif
  return random_less_than(limit + 1);
}


#define GRAEHL_RANDOM__NLETTERS 26
// works for only if a-z A-Z and 0-9 are contiguous
inline char random_alpha() {
  unsigned r = random_less_than((unsigned)(GRAEHL_RANDOM__NLETTERS * 2));
  return (r < GRAEHL_RANDOM__NLETTERS) ? 'a' + r : ('A' - GRAEHL_RANDOM__NLETTERS) + r;
}

inline char random_alphanum() {
  unsigned r = random_less_than((unsigned)(GRAEHL_RANDOM__NLETTERS * 2 + 10));
  return r < GRAEHL_RANDOM__NLETTERS * 2
             ? ((r < GRAEHL_RANDOM__NLETTERS) ? 'a' + r : ('A' - GRAEHL_RANDOM__NLETTERS) + r)
             : ('0' - GRAEHL_RANDOM__NLETTERS * 2) + r;
}
#undef GRAEHL_RANDOM__NLETTERS

inline std::string random_alpha_string(unsigned len) {
  boost::scoped_array<char> s(new char[len + 1]);
  char* e = s.get() + len;
  *e = '\0';
  while (s.get() < e--) *e = random_alpha();
  return s.get();
}

// P(*It) = double probability (unnormalized).
template <class It, class P>
It choose_p(It begin, It end, P const& p) {
  if (begin == end) return end;
  double sum = 0.;
  for (It i = begin; i != end; ++i) sum += p(*i);
  double choice = sum * random01();
  for (It i = begin;;) {
    choice -= p(*i);
    It r = i;
    ++i;
    if (choice < 0 || i == end) return r;
  }
  return begin;  // unreachable
}

// P(*It) = double probability (unnormalized).
template <class Sum, class It, class P>
It choose_p_sum(It begin, It end, P const& p) {
  if (begin == end) return end;
  Sum sum = 0.;
  for (It i = begin; i != end; ++i) sum += p(*i);
  double choice = random01();
  for (It i = begin;;) {
    choice -= p(*i) / sum;
    It r = i;
    ++i;
    if (choice < 0 || i == end) return r;
  }
  return begin;  // unreachable
}

// as above but already normalized
template <class It, class P>
It choose_p01(It begin, It end, P const& p) {
  double sum = 0.;
  double choice = random01();
  for (It i = begin;;) {
    sum += p(*i);
    It r = i;
    ++i;
    if (sum > choice || i == end) return r;
  }
  return begin;  // unreachable
}


template <class It>
void randomly_permute(It begin, It end) {
  using std::swap;
  size_t N = end - begin;
  for (size_t i = 0; i < N; ++i) {
    swap(*(begin + i), *(begin + random_up_to(i)));
  }
}

template <class V>
void randomly_permute(V& vec) {
  using std::swap;
  size_t N = vec.size();
  for (size_t i = 0; i < N; ++i) {
    swap(vec[i], vec[random_up_to(i)]);
  }
}
