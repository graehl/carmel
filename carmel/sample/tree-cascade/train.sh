#!/bin/bash
verbose=
hidden=hidden.fsa
given="observed1 observed2"
channel=observed0
observed="$given $channel"
carmel=${carmel:-carmel}

function tofile {
    if [ "$verbose" ] ; then
        echo to $1: 2>&1
        tee "$1"
    else
        cat > "$1"
    fi
}
function desc {
    echo
    echo "$@"
    echo =============
}


if [ "$verbose" = 1 ] ; then
    mflag=-m
    echo verbose mode enabled
else
    mflag=
fi
desc $source produces $observed - training $source and $channel

set -e
set -o pipefail
dir=${dir:-`pwd`}
cd $dir || exit

biases=
biasesnorm=
desc 'building p(observed data|hidden) wfsas'
for ob in $given; do
    bdata=$ob.data
    bchan=$ob.fst
    normchan=$bchan.conditional
    $carmel $mflag -n $bchan | tofile $normchan
    bsrc=$ob.fsa
    desc "p(hidden|$bdata)*constant to $bsrc"
    $carmel $mflag -ri $bchan $bdata | tofile $bsrc.unlocked
    $carmel $mflag --project-left --project-identity-fsa -N 0 $bsrc.unlocked | tofile $bsrc
    biases+=" $bsrc"
    biasesnorm+=N
done
tail $src $biases

#todo: think about biasing training by using dirichlet pseudocount prior instead (ad hoc)
data=$channel.data.train
(echo;cat $channel.data) > $data

desc possible hidden given $given "(excluding $channel data)":
$carmel $hidden $biases -Ok 10
chan=$channel.fst
desc training hidden $hidden and channel $chan - should produce argmax "P(hidden|$observed data)"
$carmel $mflag --normby=J${biasesnorm}C --train-cascade $data $hidden $biases $chan
biased=$hidden.trained
#unbiased for comparison
unbhidden=$hidden.unbiased
cp $hidden $unbhidden
desc training just hidden and channel, ignoring other givne data
$carmel --train-cascade $data $unbhidden $chan
unbiased=$unbhidden.trained

$carmel --project-right --project-identity-fsa $biased > $biased.id
$carmel --project-right --project-identity-fsa $unbiased > $unbiased.id
#show likelihood of bias-observations under biased and unbiased (should be higher for biased)
for ob in $observed; do
    bdata=$ob.data
    bchan=$ob.fst
if [ "$verbose" = 1 ] ; then
    desc kbest derivations for $ob under biased
    $carmel -kir 5 $biased.id $bchan $bdata
    desc kbest derivations for $ob under unbiased
    $carmel -kir 5 $unbiased.id $bchan $bdata
fi
    desc likelihood for $ob under biased $ob
    $carmel -br $biased $bchan $bdata
    desc likelihood for $ob under unbiased $ob
    $carmel -br $unbiased $bchan $bdata
done

tail $biased $chan.trained
