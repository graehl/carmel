#!/bin/bash
cd `dirname $0`
B=${1:-../bin/$ARCH/carmel}
log=logs/tests.`date +%C%y%m%d_%H:%M`
(echo $B;ls -l $B;uname -a;hostname;time . j-test-jap; time . traintest.sh) 2>&1 | tee $log
time $B -IEQ -k 1000 angela.knight.kbest.wfst 2>&1 >> $log
tail -20 $log
