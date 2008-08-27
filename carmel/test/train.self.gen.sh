#!/bin/bash
set -x
carmel=${carmel:-carmel}
fst=${1:?arg 1: a wfst e.g. train.a.w}
N=${N:-100}
$carmel -g $N $fst > corpus.$fst.$N
$carmel --constant-weight=1 $fst > $fst.u
$carmel -S corpus.$fst.$N $fst.u >/dev/null 
$carmel -t corpus.$fst.$N $fst.u | tee $fst.trained.self.gen
echo original:
$carmel $fst

