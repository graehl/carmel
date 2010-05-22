. ~graehl/isd/hints/aliases.sh
. ~graehl/isd/hints/bashlib.sh
wd=${wd:-exp}
noise=${noise:-.1}
iter=${iter:-50}
in=${in:-10k}
inlimit=${inlimit:-30}
vizlimit=${vizlimit:-20}
nin=${nin:-100000}
nviz=${nviz:-6}

mkdir -p $wd
ntrain=`nlines $in.f`
showvars_required wd noise iter in nviz nin ntrain
if [ "$nin" -gt "$ntrain" ] ; then
    mono=mono
else
    mono=first-$nin.mono
fi
inm=$mono.0$noise
set -e
sub=$wd/$inm
./subset-training.py -n $nin -u $inlimit --pcorrupt=$noise --monotone --inbase=$in --outbase=$sub
alignbase=$sub.i=$iter
for s in e-parse f a info; do
    cp $sub.$s $alignbase.$s
done
[ "$noise" == 0 ] && cp $sub.a $sub.a-gold
#if [ "$iter" -gt 0 ] ; then
 out=$alignbase.out
 log=$alignbase.log
 if ! [ "$skip" ] ; then
     set -x
     everyarg=""
     [ "$every" ] && everyarg="--alignments-every=$every"
     ./gextract.py $everyarg --notest --golda $sub.a-gold --alignment-out $alignbase.a --inbase=$sub --iter=$iter "$@" 2>&1 >$out | tee $log
     set +x
 fi
pr=" `lastpr $log`"
comment="noise=$noise iter=$iter$pr"
#fi
function vizsub {
    echo -- vizsub "$@"
    vizlimit=${vizlimit:-20}
    nviz=${nviz:-6}
    lang=${lang:-eng}
    local in=$1
    comment=${comment:-$in}
    align=${2:-$in.a}
    vizout=$align.first$nviz
    ./subset-training.py --align-in=$align --comment="$comment" -l 10 -u $vizlimit -n $nviz --inbase=$in --outbase=$vizout --skip-identity
    lang=$lang vizalign $vizout
    echo $vizout.pdf
}
vizsub $sub $alignbase.a
if [ "$every" ] ; then
    set +e
for i in `seq 0 $iter`; do
    af=$alignbase.a.$i
    echo $af
    set -x
    [ -f $af ] && vizsub $sub $af 2>/dev/null
    set +x
done
fi
showvars pr out log
grep zeroprob $log || echo "no zeroprob"
