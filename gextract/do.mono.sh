. ~graehl/isd/hints/aliases.sh
. ~graehl/isd/hints/bashlib.sh
set -o pipefail
eff=~graehl/bin/eff
function main {
wd=${wd:-exp}
noised=${noised:-4}
iter=${iter:-50}
in=${in:-10k}
inlimit=${inlimit:-30}
vizlimit=${vizlimit:-30}
nin=${nin:-100000}
nviz=${nviz:-6}
temp0=${temp0:-1}
tempf=${tempf:-1}
vizhead=${vizhead:-2}
if [ "$justgraph" ] ; then
    skip=1
fi
if [ "$vizrecall" ] ; then
vizro=--skip-includes-identity
elif [ "$vizall" ] ; then
vizro=
else
vizro=--skip-identity
fi
[ "$vizall" ] || vaopt=-s
vizsubopt=${vizsubopt:-$vizro}
mono=1
if [ "$nomono" ] ; then
    mono=
    noise=${noise:-0}
fi
noise=${noise:-.1}

mkdir -p $wd
ntrain=`nlines $in.f`
showvars_required wd noise iter in nviz nin temp0 tempf vizhead
showvars_optional vizsubopt justgraph skip vizrecall vizall mono nomono skipviz
if [ "$tempf" != 1 -o "$temp0" != 1 ] ; then
    annealarg="--tempf=$tempf"
    a0arg="--temp0=$temp0"
    annealdesc="annealing tempf=$tempf "
    annealf=".tempf=$tempf"
fi
if [ "$mono" ] ; then
    oname=mono
    monoarg=--monotone
else
    oname=`basename $in`
    monoarg=
fi

if [ "$nin" -gt "$ntrain" ] ; then
    echo "corpus $in has only $ntrain lines"
else
    oname=first-$nin.$oname
fi
inm=$oname.0$noise
set -e
sub=$wd/$inm
#desc="monotone links corrupted +-$noisex with prob=$noise"
    if ! [ "$skip" ] ; then
        ./subset-training.py -n $nin -u $inlimit --pcorrupt=$noise --dcorrupt=$noised $monoarg --inbase=$in --outbase=$sub
    fi
desc=`head -1 $sub.info | perl -pe 's/line \d+//'`
alignbase=$sub.iter=$iter$annealf

[ "$noise" == 0 ] && cp $sub.a $sub.a-gold

out=$alignbase.out
log=$alignbase.log
if ! [ "$skip" ] ; then
    set -x
    everyarg=""
    [ "$every" ] && everyarg="--alignments-every=$every"
    ./gextract.py $a0arg $annealarg $everyarg --notest --golda $sub.a-gold --alignment-out $alignbase.a --inbase=$sub --iter=$iter "$@" 2>&1 >$out | tee $log
    set +x
fi
#pr=" `lastpr $log`"
comment="iter=$iter"
irp=$alignbase.irp
$eff -f 'iter,R,log10(cache-prob),P' $log > $irp
function graph {
    local y=$1
    local ylbl="$2"
    local ylbldistance=${3:-'0.7"'}
    local of=$alignbase.y=$y.$ylbl.png
    pl -png -o $of -prefab lines data=$irp pointsym=none x=1 y=$y ylbl="$ylbl" ylbldistance=$ylbldistance xlbl=iter title="$annealdesc$desc" ystubfmt '%4g' ystubdet="size=6" -scale 1.4
    echo $of
}
g2=`graph 2 "alignment-recall"`
g3=`graph 3 "sample-prob"`
g3=`graph 4 "alignment-precision"`

allvizpdf=""
function vizsub {
        echo -- vizsub "$@"
        vizlimit=${vizlimit:-20}
        local limarg
        if [ $vizlimit -lt $inlimit ] ; then
            limarg="-u $vizlimit"
        fi
        nviz=${nviz:-6}
        lang=${lang:-eng}
        local in=$1
        comment=${comment:-$in}
        local alignb=${2:-$in}
        local vizout=$alignb.first$nviz
        local vizpdf=$vizout.pdf
        allvizpdf="$allvizpdf $vizpdf"
    if [ "$skipviz" ] ; then
        echo skipping
    else
        ./subset-training.py --align-in=$alignb.a --info-in=$alignb.info --etree-in=$alignb.e-parse --comment="$comment" -l 10 $limarg -n $nviz --inbase=$in --outbase=$vizout $vizsubopt
        lang=$lang vizalign $vizout $vaopt && echo $vizout.pdf
    fi
}

if ! [ "$justgraph" ] ; then
    vizsub $sub $alignbase
    if [ "$every" ] ; then
        set +e
        for i in  ghkm `seq 0 $iter`; do
            afb=$alignbase.i=$i
            af=$afb.a
            [ -f $af ] && vizsub $sub $afb 2>/dev/null
        done
    fi
    if [ "$vizhead" -gt 0 ] ; then
        currydef pdfheadn pdfrange 1 $vizhead
        mapreduce_files pdfheadn pdfcat $allvizpdf > $alignbase.all.head-$vizhead.pdf
    fi
fi
ls $alignbase*.a
ls $alignbase*.pdf
ls $alignbase*.png
showvars pr out log g2 g3
grep zeroprob $log || echo "no zeroprob"
}
main;exit
