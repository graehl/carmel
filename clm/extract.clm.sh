rm x.*
set -x
extract "$@" -w x.left -W x.right -s 0 -e ${head:=10} -N ${N:=3} -r training -x x.rules -G - -g 1 -l 1000:4 -m 5000 -O -T -i -X
set +x
set -e
function one {
    perl -pe 's/$/ 1/' "$@"
}
function filt {
    egrep '^[0-9]' -- "$@" | cut  -f4- | one
}
function show {
    echo '==>' $name '<=='
    wc -l "$@"
    head "$@"
}
function Evocab {
    perl -ne '$e{$2}=1 if /(^| )(E\S+)/;END{print "$_\n" for (keys %e)}' "$@"
}
function clm_from_counts {
    local count=${1:?'Ea Eb Fc x' e.g. x=1 time, clm ngram counts.  E... are all nonevents (context), F... is predicted}
    shift
    local out=$1
    shift
    name=Evocab ngram=${N:-3}
    local Ev=`mktemp`
    Evocab $count > $Ev
    show $count $Ev
    echo using ngram order N=$ngram
    local unkargs="-unk"
    local ngoargs="-order $ngram"
    local smoothargs="-wbdiscount"
#kn discount fails when contexts are not events.
    ngram-count $ngoargs $unkargs $smoothargs -sort -read $count -nonevents $Ev -lm $out $*
    rm $Ev
}

for f in x.left x.right; do
    sort $f | uniq | filt > $f.u
    filt < $f > $f.d
    ulm=$f.clm.$N.srilm
    clm_from_counts $f.u $ulm
    show $f.u $f.d $ulm
done
