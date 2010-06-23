#!/bin/bash
set -x
carmel=${carmel:-carmel}
N=${N:-100}
M=${M:-5}
function safefilename {
   echo "$@" | perl -pe 's/\W+/./g'
}
comp=comp.`safefilename $*`
corp=.corpus.$comp.$N
$carmel "$@" > $comp
$carmel -g $N $comp > $corp
uchain=
for f in $*; do
 $carmel -n --constant-weight=1 $f > $f.u
 uchain+=" $f.u"
done
$carmel -S $corp $uchain >/dev/null
$carmel -S $corp $comp >/dev/null
$carmel -M $M --train-cascade $ARGS $corp $uchain
for f in $*; do
 echo original:
 $carmel $f
 echo trained:
 $carmel $f.u.trained
done
