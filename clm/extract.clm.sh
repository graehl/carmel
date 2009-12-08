#!/bin/bash
. ~graehl/isd/hints/bashlib.sh
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

###

#TODO: parallelize (giraffe?) ghkm chunks, pipe preproc straight to ngram-count w/o intermediate file?
grf=${grf:-giraffe}
ix=${ix:-training}
ox=${ox:-x}
chunksz=${chunksz:-100000}
nl=`nlines $ix.e`
nc=$(((nl+chunksz-1)/chunksz))
N=${N:-3}
bign=${bign:-0}
echo "$((nc)) chunks of $chunksz ea. for $nl lines.  $N-gram i=$ix o=$ox bign=$bign"
set -e
(
for i in `seq 1 $nc`; do
    el=$((chunksz*i))
    sl=$((el-chunksz+1))
    oxi=$ox.c$i
    #-G -T
    echo extract "$@" -s $sl -e $el -w $oxi.left -W $oxi.right -N $N -r $ix -x /dev/null -g 1 -l 1000:$bign -m 5000 -O -i -X
done
) | $grf -
for d in left right; do
    (
    dp=$ox.$d
    sort $ox.c*.$d | uniq | filt > $dp
    ulm=$dp.$N.srilm
    clm_from_counts $ox.$d.u $ulm
    show $dp $ulm
    )&
done
wait
