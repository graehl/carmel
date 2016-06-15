# if you don't have /usr/bin/time, 'sudo yum install time'
sudodropcaches() {
  echo 3 | sudo tee /proc/sys/vm/drop_caches </dev/null && echo dropped caches using sudo || true
}

have_linux_readlink()  {
    [[ `uname` != Darwin ]] && readlink -nfs / 2>&1 >/dev/null
}
realpath() {
    if have_linux_readlink ; then
        for f in "$@"; do
            readlink -nfs $(cd "$(dirname $f)"; pwd)/"$(basename $f)"
            echo
        done
    else
        for f in "$@"; do
            echo $(cd "$(dirname $f)"; pwd)/"$(basename $f)"
        done
    fi
}
whichreal() {
    local w=`which "$@"`
    if [ -x "$w" ] ; then
        realpath $w
    else
        echo $w
    fi
}

driveis() {
    local d=`df ${1:-.} | tail -1 | cut -d' ' -f1`
    echo ${d#/dev/}
}
ios() {
    local drive=${1:-`driveis .`}
    local n=${2:-200}
    local sleep=${3:-10}
    local megsarg=
    if [[ $megs ]] ; then
        megsarg=-m
    fi
    iostat -m -p ALL | grep tps
    (
    for (( i=1; i <= $n; i++ )); do
        iostat $megsarg -y -p ALL $sleep 1 | grep $drive | tail -n 1
    done
    )
}

echo2() {
    echo "$@" 1>&2
}
preview_banner() {
    echo "==> $* <=="
}
preview1() {
    if [[ $2 ]] ; then
        preview_banner $2
    else
        preview_banner $1
    fi
    tail -${tail:-4} "$1"
    echo
}
preview() {
    for f in "$@"; do
        preview1 $f
    done
}

UNAME=`uname`
have_linuxtime() {
    local linuxtime=/usr/bin/time
    [[ -x $linuxtime ]]
}
full_linuxtime() {
    have_linuxtime && [[ $UNAME != Darwin ]]
}
linuxtime() {
    if have_linuxtime ; then
        if full_linuxtime ; then
            #          /usr/bin/time -f '%Es - %Mkb peak' -- "$@"
            local timeoutarg
            if [[ $timeout ]] ; then
                timeoutarg="-o $timeout"
            fi
            /usr/bin/time $timeoutarg -f '%Es - %Mkb peak %Iinputs %Ooutputs' -- "$@"
            if [[ $timeout ]] ; then
                cat $timeout 1>&2
            fi
        else
            /usr/bin/time -lp "$@"
        fi
    else
        TIMEFORMAT='%3lR'
        time "$@"
    fi
}


timerss() {
    echo "time $*" 1>&2
    local timeout=`mktemp /tmp/timeram.out.XXXXXX`
    linuxtime "$@"
    cat $timeout
    rm $timeout
}

save12timeram() {
    local save1="$1"
    shift || true
    echo "time $*" 1>&2
    if full_linuxtime ; then
        local timeout=`mktemp /tmp/timeram.out.XXXXXX`
        /usr/bin/time  -f '%Es - %Mkb peak %Iinputs %Ooutputs' -o $timeout -- "$@" >$save1 2>&1
        cat $timeout
    else
        TIMEFORMAT='%3lR'
        (time "$@" >$save1 2>&1) 2>&1
    fi
}


atime() {
    #usage: atime program args ...
    #vars in: nrep, upx, warm, progname vars, ios
    #vars out: timeouts timerams timecmds
    local nrep=${nrep:-1}
    local proga=$1
    shift || true
    local name=${progname:-`basename $proga`}
    [[ -x $proga ]] || proga=`whichreal $proga`
    local timedir=${timedir:-`mktemp -d /tmp/time.$name.XXXXXX`}
    mkdir -p $timedir
    local outpre=$timedir/$name
    if [[ $strip ]] ; then
        #        local stripped=$outpre/$name.stripped
        local stripped=$proga.stripped
        set -e
        cp -f $proga $stripped
        echo "$proga => $stripped"
        chmod +rx $stripped
        chmod u+w $stripped
        if [[ $upx ]] ; then
            strip "$stripped" || true
            upx "$stripped" || true
        fi
        proga=$stripped
    fi
    if [[ $warm ]] ; then
        echo "$proga $* 2>&1"
        $proga "$@" 2>&1
    fi
    outa=$outpre.$nrep.out
    timeouts+=" $outa"
    echo2 $proga "$@" "> $outa"
    echo2 ::::::::: nrep=$nrep
    local nrepsh=$outpre.${nrep}.sh
    cp /dev/null $nrepsh
    chmod +x $nrepsh
    for i in `seq 1 $nrep`; do
        echo $proga "$@"
    done >> $nrepsh
    timecmds+=" $nrepsh"
    local iosout=$outpre.ios.$nrep
    #[[ $drop ]] || nodrop=1
    if [[ ! $nodrop ]] ; then
        echo2 dropcaches - set nodrop=1 to prevent
        if [[ -x /usr/local/bin/dropcaches ]] ; then
            /usr/local/bin/dropcaches
        else
            sudodropcaches
        fi
    fi
    if [[ $ios ]] ; then
    local drive=${drive:-`driveis .`}
    (ios $drive 100 30 | tee $iosout) &
    iosps=${!}
    fi
    local ttime=$outa.time.$nrep
    if [[ $tee ]] ; then
        save12timeram $outa $nrepsh | tee $ttime
    else
        save12timeram $outa $nrepsh > $ttime
    fi
    if [[ $ios ]] ; then
        kill $iosps || true
        timerams+=" $iosout"
    fi
    timerams+=" $ttime"
    grep 's - ' $ttime
    echo2 $proga "$@" "done (exit=$?)"
    echo2 =========
}

save1timeram() {
    local save1="$1"
    shift || true
    echo "time $*" 1>&2
    if full_linuxtime ; then
        linuxtime >$save1 2>&1
    else
        TIMEFORMAT='%3lR'
        time "$@"  >$save1 2>&1
    fi
}

getprogname12() {
    prog1=$1
    prog2=$2
    abs1=`whichreal $prog1`
    abs2=`whichreal $prog2`
    name1=$1
    name2=$2
    short1=$1
    short2=$2
    while [[ $short1 != . ]] && [[ $short2 != . ]] ; do
        name1=`basename $short1`
        name2=`basename $short2`
        if [[ $name1 != $name2 ]] ; then
            return
        fi
        short1=`dirname $short1`
        short2=`dirname $short2`
    done
    [[ $name1 ]] || name1=A`basename $prog1`
    [[ $name2 ]] || name2=B`basename $prog2`
}
abtime() {
    (
        getprogname12 "$@"
        set -e
        local timedir=${timedir:-`mktemp -d /tmp/time.$name1-$name2.XXXXXX`}
        mkdir -p $timedir
        shift
        shift
        timeouts=
        timerams=
        timecmds=
        timedir=$timedir/A progname=$name1 atime $abs1 "$@"
        timedir=$timedir/B progname=$name2 atime $abs2 "$@"
        echo
        preview $timeouts $timecmds $timerams
    )
}

replinen() {
    perl -e '
$n=shift;
$m=$n;
($m,$n)=($1,$2) if $n=~/(\d+)-(\d+)/;
print STDERR "$m-$n vertical repeats of each line...\n";
while(<>) {
 for $i (1..$n) {
  print;
 }
}' "$@"
}
repn() {
    perl -e '
$n=shift;
$m=$n;
($m,$n)=($1,$2) if $n=~/(\d+)-(\d+)/;
print STDERR "$m-$n horizontal repeats of each line...\n";
while(<>) {
 chomp;
 $one=$_;
 for $i (1..$n) {
  print "$_\n" if $i>=$m;
  $_="$_ $one";
 }
}
' "$@"
}
reptime() {
    width=$1
    shift
    height=${1}
    shift
    input=$1
    shift
    prog1=${1:?usage: reptime [repeat-width] [repeat-height] [input-file] [program] [args] ...}
    shift
    repn $width $input | replinen $height | atime $prog1 "$@"
}

save12timeram() {
    local save1="$1"
    shift || true
    echo "time $*" 1>&2
    if full_linuxtime ; then
        local timeout=`mktemp /tmp/timeram.out.XXXXXX`
        /usr/bin/time  -f '%Es - %Mkb peak %Iinputs %Ooutputs' -o $timeout -- "$@" >$save1 2>&1
        cat $timeout
    else
        TIMEFORMAT='%3lR'
        (time "$@" >$save1 2>&1) 2>&1
    fi
}
save1time() {
    local save1="$1"
    shift || true
    local savetime="$1"
    shift || true
    if full_linuxtime ; then
        /usr/bin/time  -f '%Es - %Mkb peak %Iinputs %Ooutputs' -o $savetime -- "$@" >$save1
    else
        TIMEFORMAT='%3lR'
        time "$@" >$save1 2>$savetime
    fi

}
untilfail() {
    echo "$*"
    local i=1
    [[ $smallestdir ]] && mkdir -p $smallestdir && echo "smaller outputs in $smallestdir"
    mkdir $untildir || true
    rm -f $untildir/FAIL*
    local lasti=0
    (set -e
    while save1time $untildir/$i.out $untildir/$i.time "$@" 2>$untildir/$i.err  ; do
        echo $i `cat $untildir/$i.time` > $untildir/FINISH
        perl -e '$i=$ARGV[0]; $g=10;$gn=100;print ($i % $g == 0 ? $i : ".");print "\n" if $i % $gn == 0' $i
        j=$((i+1))
        if [[ $lasti != 0 ]] && ! [[ $keepall ]] ; then
            rm -f $untildir/$lasti.*
        fi
        lasti=$i
        i=$j
        echo $i > $untildir/START
    done
    ln -sf $i.out $untildir/FAIL.out
    ln -sf $i.err $untildir/FAIL.err
    ln -sf $i.time $untildir/FAIL.time
    )
    preview $untildir/FINISH $untildir/FAIL*
    ls -l $untildir/FINISH $untildir/FAIL*
    echo "$*"
}
