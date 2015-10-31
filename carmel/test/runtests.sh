#!/bin/bash
cd `dirname $0`
B=${1:-../bin/$HOST/carmel}
which $B
mkdir -p logs
log=logs/tests.`basename $B`.`date +%C%y%m%d_%H:%M`
(echo $B;ls -l $B;uname -a;hostname; time . traintest.sh;time $B -IEQ -k 1000 angela.knight.kbest.wfst;time . j-test-jap ) 2>&1  | tee $log
ln -sf $log latest.log
echo
echo `pwd`/latest.log
