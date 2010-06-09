#!/bin/bash
export skip=1
vizall=1 skip=1  noised=0 until=3 every=10 noise=0 iter=100 ./do.mono.sh
skip=1  noised=4 until=3 every=20 noise=.3 iter=100 ./do.mono.sh
[ "$first" ] && exit
skip=1 until=5 nomono=1 temp0=1.2 tempf=.2 iter=100 every=20 ./do.mono.sh
skip=1 until=10 nomono=1 iter=200 every=20 ./do.mono.sh
skip=1 noised=5 until=3 every=10 noise=.3 iter=120 ./do.mono.sh
skip=1 until=5 noised=2 every=20 temp0=1 tempf=1 noise=.2 iter=160 ./do.mono.sh

skip=1 iter=100 nin=1000 noise=.3 noised=5 ./do.mono.sh

skip=1 noised=2 until=3 every=10 temp0=1.5 tempf=.08 noise=.2 iter=80 ./do.mono.sh

