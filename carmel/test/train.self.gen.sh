#!/bin/bash
set -x
carmel=${carmel:-carmel}
fst=${1:?arg 1: a wfst e.g. train.a.w}
shift
N=${N:-100}
$carmel -g $N $fst > corpus.$fst.$N
$carmel --constant-weight=1 $fst > $fst.u
$carmel -S corpus.$fst.$N $fst.u >/dev/null 
$carmel -F $fst.trained.self.gen -t $* corpus.$fst.$N $fst.u 
#echo original:
#$carmel $fst

