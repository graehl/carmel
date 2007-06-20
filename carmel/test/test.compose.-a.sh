#!/bin/bash
. ~/isd/hints/aliases.sh
B=${B:-carmel}
N=${N:-10}
a=$1
b=${2:-args: xdcr1 xdcr2 (for composition)}
set -x
set -e
$B -N 100000 $a > $a.g
$B -N 200000 $b > $b.g
$B -m $a $b > $a.composed
$B -am $a.g $b.g > $a.composed.g
$B -@k $N $a.composed > $a.composed.best
$B -k $N $a.composed > $a.composed.paths
$B -S $a.composed.best $a.composed | tee $a.composed.ppx
$B -S $a.composed.best $a.composed.g | tee $a.composed.g.ppx
