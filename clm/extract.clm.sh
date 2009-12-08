#!/bin/bash
#TODO: parallelize (giraffe) ghkm chunks, pipe preproc straight to ngram-count w/o intermediate file?
ix=${ix:-training}
ox=${ox:-x}
set -x
extract "$@" -w $ox.left -W $ox.right -s 0 -e ${head:-999999999} -N ${N:=3} -r $ix -x ${ruleout:=/dev/null} -G - -g 1 -l 1000:${bign:=0} -m 5000 -O -T -i -X
set +x
set -e
function one {
    perl -pe 's/$/ 1/' "$@"
}
function bocounts {
    perl -ne '@a=split;for (0..$#a) { print join(" ",@a[$_..$#a]),"\n" }' "$@"
}
function filt {
    egrep '^[0-9]' -- "$@" | cut  -f4- | bocounts | one
}
function show {
    echo '==>' $name '<=='
    wc -l "$@"
    head "$@"
}
function Evocab {
    perl -ne '$e{$2}=1 while /(^| )(E\S+)/g;END{print "$_\n" for (keys %e)}' "$@"
}
function clm_from_counts {
    local count=${1:?'Ea Eb Fc x' e.g. x=1 time, clm ngram counts.  E... are all nonevents (context), F... is predicted}
    shift
    local out=$1
    shift
    name=Evocab ngram=${N:-3}
    local Ev=$count.Ev
    #`mktemp`
    Evocab $count > $Ev
    show $count $Ev
    echo using ngram order N=$ngram
    local unkargs="-unk"
    local ngoargs="-order $ngram"
    local noprune="-minprune $((ngram+1))"
    local smoothargs="-wbdiscount"
#kn discount fails when contexts are not events.
    ngram-count $ngoargs $unkargs $smoothargs $noprune -sort -read $count -nonevents $Ev -lm $out $*
#    rm $Ev
}

for f in $ox.left $ox.right; do
    sort $f | uniq | filt > $f.u
#    filt < $f > $f.d
    ulm=$f.$N.srilm
    clm_from_counts $f.u $ulm
    show $f.u $ulm
done

wc -l $ruleout $ox*.u
