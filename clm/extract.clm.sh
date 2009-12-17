#!/bin/bash
. ~graehl/isd/hints/bashlib.sh
. ~graehl/t/utilities/make.lm.sh

extract=${extract:-`which extract`}
extract=`realpath $extract`
[ "$numclass" ] && enumclass=1
[ "$numclass" ] && fnumclass=1
showvars enumclass fnumclass
function one {
    perl -pe 's/$/ 1/' "$@"
}
function bocounts {
    perl -ne 'chomp;@a=split;for (0..$#a) { $a[$_] =~ s/\d/\@/g if $_==$#a && $ENV{fnumclass} || $_<$#a && $ENV{enumclass}};$a[$#a]=~s/^\F<(\/)?s\>$/F<${1}foreign-sentence>/o;for (0..$#a) { print join(" ",@a[$_..$#a]),"\n" }' "$@"
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
function stripEF {
    perl -pi -e 's/(\s)[EF](\S+)/$1$2/go' "$@"
}
function clm_from_counts {
    local count=${1:?'Ea Eb Fc x' e.g. x=1 time, clm ngram counts.  E... are all nonevents (context), F... is predicted.  env N=3 means trigram}
    shift
    local ngram=${N:-3}
    local out=${1:-$count.$ngram.srilm}
    shift
    local Ev=$count.Ev
    #`mktemp`
    Evocab $count > $Ev
    show $count $Ev
    local unkargs="-unk"
    local ngoargs="-order $ngram"
    local noprune="-minprune $((ngram+1))"
    local smoothargs="-wbdiscount"
#kn discount fails when contexts are not events.
    set -x
    ngram-count $ngoargs $unkargs $smoothargs $noprune -sort -read $count -nonevents $Ev -lm $out $*
    [ "$stripEF" ] && stripEF $out
    lwlm_from_srilm $out
    set +x
#    rm $Ev
}
###

function main {
#TODO: parallelize (giraffe?) ghkm chunks, pipe preproc straight to ngram-count w/o intermediate file?
grf=${grf:-giraffe}
ix=${ix:-training}
ox=${ox:-x}
chunksz=${chunksz:-100000}
nl=${head:-`nlines $ix.e-parse`}
if [ $chunksz -gt $nl ] ; then
     chunksz=$nl
fi
nc=$(((nl+chunksz-1)/chunksz))
N=${N:-3}
bign=${bign:-0}
banner "$((nc)) chunks of $chunksz ea. for $nl lines.  $N-gram i=$ix o=$ox bign=$bign"
set -e
lfiles=""
rfiles=""
rm -f $ox.c*.{left,right}
for i in `seq 1 $nc`; do
    el=$((chunksz*i))
    sl=$((el-chunksz+1))
#    showvars_required nc chunksz el sl
    oxi=$ox.c$i
    #empirically (100sent) verififed to not change uniqued locations over minimal: -G - (wsd), $bign>0, -T
    echo $extract "$@" -s $sl -e $el -w $oxi.left -W $oxi.right -N $N -r $ix -z -x /dev/null  -g 1 -l 1000:$bign -m 5000 -O -i -X
done | $grf -
header DONE WITH GHKM
for d in left right; do
    (
    dp=$ox.$d
    dfiles=$ox.c*.$d
    showvars_required dfiles
    sort $dfiles | uniq | filt > $dp
    tbz=$ox.$d.ghkm.tar.bz2
    rm -f $tbz
    tar -cjf $tbz $dfiles && rm $dfiles
    ulm=$ox.$N.srilm.$d
    stripEF=1 clm_from_counts $dp $ulm
    show $dp $ulm
    bzip2 -f $dp
    ) &
done
wait
}

[ "$nomain" ] || main
