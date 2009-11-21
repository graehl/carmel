set -e
echo ${suf:=fem} ${ITER:=100} ${restarts:=0}
i4=$((ITER/4))
$fem -f forest -n norm -I param -e 0 -o $suf -i $i4 -r $restarts
weights=$suf ./errors.sh
$fem -f forest -n norm -I param -e 0 -o $suf.crp --crp=$ITER --burnin=$i4 --alpha=alpha
weights=$suf.crp ./errors.sh
weights=$suf ./errors.sh
