#!/bin/bash
function tofile {
#    cat > $tofile
    tee $tofile
}

set -e
set -x
set -o pipefail
carmel=${carmel:-~/trunk/graehl/carmel/bin/zergling/carmel.debug}
dir=${dir:-`pwd`}
cd $dir || exit
src=source.fsa
chan=channel.fst
data=channel.data
nbias=2
#$carmel -i bias1.data > bias1.data.fsa
biases=
biasesnorm=
for i in `seq 1 $nbias`; do
    b=bias$i
    bdata=$b.data
    bchan=$b.fst
#    bchanl=$b.fst.locked
#    $carmel -N 0 $bchan | tofile $bchanl
    bchanl=$bchan
    bsrc=$b.source.fsa
    $carmel --project-right --project-identity-fsa $src | tofile $bsrc
    if ! $carmel --normby=J --train-cascade $bdata $bsrc $bchanl ; then
        set +x
        echo failed to train for $b
        echo composition:
        $carmel -m $bsrc $bchanl
        exit
        echo
        echo training:
        cat $bdata
        echo likelihood:
        $carmel -S $bdata $bsrc $bchanl
        echo random gen:
        $carmel -g 5 $bsrc $bchanl
        echo kbest:
        $carmel -kIOE 5 $bsrc $bchanl
        tail $bdata $bsrc $bchanl
        exit
    fi
    lbs=$bsrc.locked
    $carmel -N 0 $bsrc > $lbs
    biases+=" $lbs"
    biasesnorm+=N
done
tail -n 2 $src $biases

#todo: multiplying the fixed bias-trained src identity wfsas' probs with what's trained gives us a result ($biased) that's deficient ... renormalizing after might be ok.
#todo: decide whether multiplying p(src|data)*p(src|bdata1)*... is better than aggregating the counts then normalizing.  i suspect it is.
$carmel --normby=J$biasesnorm --train-cascade $data $src $biases $chan
biased=biased.source.fsa.trained
tail $src $src.trained $biases
$carmel -n $src.trained $biases | tofile $biased
exit
#unbiased for comparison
$carmel --train-cascade $data $src $chan
unbiased=$src.trained

#show likelihood of bias-observations under biased and unbiased (should be higher for biased)
for i in `seq 1 $nbias`; do
    b=bias$i
    bdata=$b.data

    echo biased $i
    $carmel -S $bdata $biased
    echo unbiased $i
    $carmel -S $bdata $biased
done
