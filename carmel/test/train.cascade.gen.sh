#!/bin/bash
set -x
carmel=${carmel:-carmel}
fst1=${1:?arg 1: a wfst}
fst2=${2:?arg 2: a wfst to compose w/ fst 1}
shift
shift
N=${N:-100}
M=${M:-5}
comp=$fst1.comp.$fst2
corp=.corpus.$comp.$N
$carmel $fst1 $fst2 > $comp
$carmel -g $N $comp > $corp
$carmel -n --constant-weight=1 $fst1 > $fst1.u
$carmel -n --constant-weight=1 $fst2 > $fst2.u
$carmel -S $corp $fst1.u $fst2.u >/dev/null
$carmel -S $corp $comp >/dev/null
$carmel -M $M --train-cascade $* $corp $fst1.u $fst2.u 
echo trained:
$carmel $fst1.u.trained
$carmel $fst2.u.trained
echo original:
$carmel $fst1
$carmel $fst2

