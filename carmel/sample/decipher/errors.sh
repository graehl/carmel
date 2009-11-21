#!/bin/bash
set -e
function eval-res {
 cat $1 | tr ' ' '\012' | awk 'NF > 0' | tr -d '"' >z1
 cat $2 | tr ' ' '\012' | awk 'NF > 0' | tr -d '"' >z2
 echo `paste -d ' ' z1 z2 | awk '$1 != $2' | wc -l`
 #echo `diff -y z1 z2 | egrep '(\||<|>)' | wc -l` '('`diff z1 z2 | grep '^<' | wc -l`')'
 #grep '^<' | wc -l
}
echo ${suf:=.trained} ${csuf:=2} ${carmel:=carmel} ${chanbase=subst.wfst}
echo ${src:=plain.bi.wfsa} ${chan:=$chanbase$suf} ${cipher:=cipher$csuf} ${correct:=correct$csuf} ${log:=errors.log}

if [ "$weights" ] ; then
    suf=`basename $weights`
    set -x
    $carmel -H --load-fem-param=$weights $src $chanbase --no-compose --write-loaded=$suf
    chan=$chanbase.$suf
    set +x
fi

$carmel -HJ -= 3.0 $chan > $chan.cubed
$carmel --project-right --project-identity-fsa $src > $src.id
function errors_chan
{
$carmel -qbsriQIWEk 1 $src.id $1 < $cipher > $chan.decode
echo "errors $2 = " `eval-res $correct $chan.decode `
}
(
echo 'length of text = ' `tr -d '"_' < $correct | wc -w`
errors_chan $chan "    ";
errors_chan $chan.cubed "cubed"
) 2>&1 | tee $log
