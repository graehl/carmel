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
$B -m $a $b > $a.comp.$b
$B -am $a.g $b.g > $a.comp.-a.$b
$B -@k $N $a.comp.$b > $a.composed.best
$B -k $N $a.comp.$b > $a.composed.paths
$B -S $a.composed.best $a.comp.$b 
$B -S $a.composed.best $a.comp.-a.$b 
