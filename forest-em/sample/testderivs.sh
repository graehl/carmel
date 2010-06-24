#!/bin/bash
d=`dirname $0`
F=$d/../bin/$ARCH/forest-em
dd=$d/derivs
w=$1
shift
normsuffix=${1:-norm}
shift
norm=$dd/$w.$normsuffix
deriv=$dd/$w.deriv
rules=$dd/$w.rules
out=train.$w.$normsuffix.out
log=train.$w.$normsuffix.log
watchrule=`grep -n '^S(x0:NP-C x1:VP)' $rules | head -1 | cut -d: -f1`
echo watching rule $watchrule:
head -$watchrule $rules | tail -1
cm="$F -f $deriv -n $norm -o $out --rules-file $rules  --watch-rule $watchrule --watch-depth 40 --watch-period 5 -M 1560 -i 200 -r 4 $*"
echo $cm
time $cm 2>&1 | tee $log
#$F -M 500 -i 200 -f $deriv -n $norm -o $out $*
