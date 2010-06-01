#!/bin/bash
export skip=1
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



