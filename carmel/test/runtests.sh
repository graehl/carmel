#!/bin/bash
cd `dirname $0`
B=${1:-../bin/$ARCH/carmel}
which $B
set -x
log=logs/tests.`basename $B`.`date +%C%y%m%d_%H:%M`
(echo $B;ls -l $B;uname -a;hostname;time . j-test-jap; time . traintest.sh) > $log 2>&1 
time $B -IEQ -k 1000 angela.knight.kbest.wfst  >> $log 2>&1
head -100 $log
echo -------
tail -5 $log
