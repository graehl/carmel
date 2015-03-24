# if you don't have /usr/bin/time, 'sudo yum install time'
sudodropcaches() {
  echo 3 | sudo tee /proc/sys/vm/drop_caches </dev/null && echo dropped caches using sudo || true
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

have_linuxtime() {
    [[ -x /usr/bin/time ]]
}
full_linuxtime() {
    have_linuxtime && [[ $UNAME != Darwin ]]
}

UNAME=`uname`
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
    #in: nrep, upx
    #out: timeouts timerams timecmds
    local nrep=${nrep:-1}
    local proga=$1
    [[ -x $proga ]] || proga=`which $proga`
    local timedir=${timedir:-`mktemp -d /tmp/time.$(basename $proga).XXXXXX`}
    mkdir -p $timedir
    local stripa=$timedir/`basename $proga`
    if [[ $strip ]] ; then
        cp -f $proga $stripa
        echo "$proga => $stripa"
        chmod +rx $stripa
        chmod u+w $stripa
        if [[ $upx ]] ; then
            strip "$stripa" || true
            upx "$stripa" || true
        fi
        proga=$stripa
    fi
    shift || true
    if [[ $warm ]] ; then
        echo "$proga $* >$proga.out 2>&1"
        $proga "$@" >$proga.out 2>&1
    fi
    outa=$stripa.out
    timeouts+=" $outa"
    echo2 $proga "$@" "> $outa"
    echo2 ::::::::: nrep=$nrep
    local nrepa=$stripa.${nrep}.sh
    cp /dev/null $nrepa
    chmod +x $nrepa
    for i in `seq 1 $nrep`; do
        echo $proga "$@"
    done >> $nrepa
    timecmds+=" $nrepa"
    local iosout=$proga.ios.$nrep
    local drive=${drive:-`driveis .`}
    if [[ ! $nodrop ]] ; then
        echo2 dropcaches - set nodrop=1 to prevent
        if [[ -x /usr/local/bin/dropcaches ]] ; then
            /usr/local/bin/dropcaches
        else
            sudodropcaches
        fi
    fi
    (ios $drive 100 30 | tee $iosout) &
    local ttime=$proga.time.$nrep
    if [[ $tee ]] ; then
        save12timeram $proga.$nrep.out $nrepa | tee $ttime
    else
        save12timeram $proga.$nrep.out $nrepa > $ttime
    fi
    kill ${!} || true
    timerams+=" $ttime $iosout"
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


abtime() {
    (
        set -e
        proga=$1
        progb=$2
        local timedir=${timedir:-`mktemp -d /tmp/time.$(basename $proga)-$(basename $progb).XXXXXX`}
        mkdir -p $timedir
        shift
        shift
        timeouts=
        timerams=
        timecmds=
        timedir=$timedir/A atime $proga "$@"
        timedir=$timedir/B atime $progb "$@"
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
    proga=${1:?usage: reptime [repeat-width] [repeat-height] [input-file] [program] [args] ...}
    shift
    repn $width $input | replinen $height | atime $proga "$@"
}
