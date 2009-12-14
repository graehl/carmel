set -e
echo ${suf:=fem} ${ITER:=500} ${restarts:=0}
i4=$((ITER/4))
[ "$EM" ] || CRP=1
if [ "$EM" ] ; then
$fem -f forest -H -n norm -I param -e 0 -o $suf.em -i $i4 -r $restarts
weights=$suf.em ./errors.sh
fi
if [ "$CRP" ] ; then
if [ "$DA" ] ; then
 sda=".crp.da=.$DA"
 argda="--high-temp=2 --low-temp=$DA"
fi
$fem -f forest -H -n norm -I param -e 0 -o $suf$sda $argda --crp=$ITER --burnin=$i4 --alpha=alpha
weights=$suf$sda ./errors.sh
fi
