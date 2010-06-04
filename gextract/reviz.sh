#!/bin/bash
export skip=1
skip=1 iter=100 nin=1000 noise=.3 noised=5 ./do.mono.sh
[ "$first" ] && exit

skip=1 until=5 noised=2 every=20 temp0=1 tempf=1 noise=.15 iter=160 ./do.mono.sh

skip=1 noised=2 until=3 every=10 temp0=1.5 tempf=.08 noise=.2 iter=80 ./do.mono.sh


# test new log impl, no anneal
skip=1 every=10 until=2 nin=1000 iter=50 noise=.2 noised=2 ./do.mono.sh


#real foreign.
skip=1 until=10 nomono=1 iter=500 every=20 ./do.mono.sh

#no noise, no anneal, more iter.
skip=1 vizall= every=20 temp0=1 tempf=1 noise=0 iter=200 ./do.mono.sh

# no noise
skip=1 every=1 skip=1 temp0=1 tempf=.2 noise=0 iter=10 ./do.mono.sh

#noise
skip=1 noised=2 every=10 temp0=1 tempf=.2 noise=.2 iter=100 ./do.mono.sh
#noise/no anneal/100
skip=1 until=10 noised=2 every=10 temp0=1 tempf=1 noise=.2 iter=100 ./do.mono.sh

#noise, anneal many iter
skip=1 noised=2 every=10 temp0=1 tempf=.2 noise=.2 iter=1000 ./do.mono.sh
#noise, anneal, 100iter
skip=1 noised=2 every=10 temp0=1 tempf=.2 noise=.2 iter=100 ./do.mono.sh

# anneal from temp 2->epsilon - # stopped due to swapping w/ others
skip=1 noised=2 until=5 every=10 temp0=2 tempf=.08 noise=.2 iter=150 ./do.mono.sh



