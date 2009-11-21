set -e
echo ${suf:=fem} ${ITER:=100} ${restarts:=0}
i4=$((ITER/4))
[ "$EM" ] || CRP=1
if [ "$EM" ] ; then
$fem -f forest -n norm -I param -e 0 -o $suf.em -i $i4 -r $restarts
weights=$suf.em ./errors.sh
fi
if [ "$CRP" ] ; then
$fem -f forest -n norm -I param -e 0 -o $suf.crp --crp=$ITER --burnin=$i4 --alpha=alpha
weights=$suf.crp ./errors.sh
fi
