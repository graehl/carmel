# if you don't have /usr/bin/time, 'sudo yum install time'
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

timerss() {
    echo "time $*" 1>&2
    if [[ -x /usr/bin/time ]] ; then
        /usr/bin/time -f '%Es - %Mkb peak' -- "$@"
    else
        TIMEFORMAT='%3lR'
        time "$@"
    fi
}

save12timeram() {
    local save1="$1"
    shift
    echo "time $*" 1>&2
    if [[ -x /usr/bin/time ]] ; then
        local timeout=`mktemp /tmp/timeram.out.XXXXXX`
        /usr/bin/time -o $timeout -f '%Es - %Mkb peak' -- "$@" >$save1 2>&1
        cat $timeout
    else
        TIMEFORMAT='%3lR'
        time "$@" >$save1 2>&1
    fi
}


atime() {
#appends to globals timeouts timerams timecmds
    local nrep=${nrep:-1}
    local proga=$1
    local timedir=${timedir:-`mktemp /tmp/time.$(basename $proga).XXXXXX`}
    mkdir -p $timedir
    local stripa=$timedir/`basename $proga`
    cp -a $proga $stripa
    proga=$stripa
    shift
    strip "$proga" || true
    upx "$proga" || true
    $proga "$@" >$proga.out 2>&1
    timeouts+=" $proga.out"
    echo2 $proga "$@" "> $outa"
    echo2 ::::::::: nrep=$nrep
    local nrepa=$proga.${nrep}.sh
    cp /dev/null $nrepa
    chmod +x $nrepa
    for i in `seq 1 $nrep`; do
        echo $proga "$@"
    done >> $nrepa
    timecmds+=" $nrepa"
    save12timeram $proga.$nrep.out $nrepa | tee $proga.time.$nrep
    timerams+=" $proga.time.$nrep"
    echo2 $proga "$@" "done (exit=$?)"
    echo2 =========
}

save1timeram() {
    local save1="$1"
    shift
    echo "time $*" 1>&2
    if [[ -x /usr/bin/time ]] ; then
        /usr/bin/time -f '%Es - %Mkb peak' -- "$@" >$save1
    else
        TIMEFORMAT='%3lR'
        time "$@" >$save1
    fi
}


abtime() {
    proga=$1
    progb=$2
    local timedir=${timedir:-`mktemp /tmp/time.$(basename $proga)-$(basename $progb).XXXXXX`}
    shift
    shift
    timeouts=
    timerams=
    timecmds=
    timedir=$timedir/A atime $proga "$@"
    timedir=$timedir/B atime $progb "$@"
    echo
    preview $timeouts $timecmds $timerams
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
