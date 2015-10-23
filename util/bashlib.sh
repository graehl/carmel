#sets: BLOBS(blob base dir), d(real script directory), realprog (real script name)
#export LC_ALL=C
wordsnl() {
    perl -e 'print "$_\n" for (@ARGV)' "$@"
}
dedupwords() {
    wordsnl "$@" | sort | uniq
}

execcp() {
    if cygwin ; then
        perl -e '
for (0..$#ARGV) {
if ($ARGV[$_] eq "-cp") {
 $i=$_;
  $_=$ARGV[$_+1];
@d=split ":";
@d=map { $_=`/usr/bin/cygpath --mixed $_`;chomp;$_ } @d;
$ARGV[$i+1]=join ";",@d;
}
}
print STDERR join(" ",@ARGV),"\n";
exec @ARGV;
' "$@"
    else
        "$@"
    fi
}
cygpathm() {
    if [ -x /usr/bin/cygpath ] ; then
        /usr/bin/cygpath --mixed "$@"
    else
        echo "$@"
    fi
}
clj() {
    local b=clojure.main
    local a=jline.ConsoleRunner
    if [ "$*" ] ; then
        a=
        b=
    fi
    local c=${CLOJURE_EXT:-~/.clojure}
    execcp ${CLOJURE_JAVA:-java} -cp $c/jline-1.0.jar:$c/clojure.jar:$c/clojure-contrib.jar $a $b "$@"
}
clr() {
    breakchars="(){}[],^%$#@\"\";:''|\\"
    local c=${CLOJURE_EXT:-~/.clojure}
    if [ $# -eq 0 ]; then
        execcp rlwrap --remember -c -b "$breakchars" \
            -f "$HOME"/.clj_completions \
            ${CLOJURE_JAVA:-java} -cp $c/jline-1.0.jar:$c/clojure.jar:$c/clojure-contrib.jar clojure.main
    else
        execcp ${CLOJURE_JAVA:-java} -cp $c/jline-1.0.jar:$c/clojure.jar:$c/clojure-contrib.jar clojure.main $1 -- "$@"
    fi
}
clines() {
    catz "$@" | tr ',' '\n'
}
weights() {
    local a=
    local c=cat
    local ca=-f1-
    if [[ $abs ]] ; then
        a='abs($2)."\t".'
        ca=-f2-
    fi
    clines "$@" | perl -pe 's/(.*):(.*)/''"$2\t$1"/e' | sort -rg | cut $ca | unescape_sb | left=1 table2txt
}
lnweights() {
    forall lnweights1 "$@"
}
lnweights1() {
    local w=$1/weights.txt
    local r=$1/record.txt
    ln -sf $(perl -ne 'print $1 if m{-w (/\S+weights\S*)}' $r) $w
    weights $w | sortbynum | tee $w.sorted | headtail
    ls -l $w*
    grep sblm $w.sorted || true
    grep sblm-ngram $r || true
}
nonblanks() {
    catz "$@" | grep -v ^$
}
runmain() {
    main "$@" 2>&1 | tee $0.log ; exit $?
}
#for LD search path: (RPATH)
name1() {
    perl -e '$_=shift;if (/(.*)=(.*)/) { print "$1\n" } else { print "$_\n" }' "$@"
}
val1() {
    perl -e '$_=shift;$v=shift || 1;if (/(.*)=(.*)/) { print "$2\n" } else { print "$v\n" }' "$@" "${defaultval:-1}"
}
names() {
    forall name1 "$@"
}
#defaultval=1
vals() {
    forall val1 "$@"
}
min() {
    bound1 '<' "$@"
}
max() {
    bound1 '>' "$@"
}
bound1() {
    local op=$1
    shift
    local m
    for x in "$@"; do
        if [ -z $m ] || [ $x $op $m ] ; then
            m=$x
        fi
    done
    echo "$m"
}
reduce() {
    local z=$1
    local f=$2
    shift
    shift
    for x in "$@"; do
        z=$($f $x $z)
    done
    echo "$z"
}
vecop() {
    local op=$1
    shift
    perl -e 'die "need 2 args: @ARGV ".scalar(@ARGV) unless @ARGV==2;$a=$ARGV[0];$b=$ARGV[1];@a=split " ",$a;@b=split " ",$b;$n=$#a>$#b?$#a:$#b;for (0..$n) { print " " if $_; print ($a[$_]'"$op"'$b[$_]);}print "\n";'  "$@"
}
vecdiff() {
    vecop - "$@"
}
vecsum() {
    vecop + "$@"
}
duration() {
    for f in "$@"; do
        mod_changed_diff "$(dirname $f)" "$f"
    done
}
oldest_modified() {
    min $(modified "$@")
}
newest_modified() {
    max $(modified "$@")
}
span_modified() {
    expr $(newest_modified "$@") - $(oldest_modified "$@")
}
hspanning() {
    sec2h $(span_modified "$@")
}
divfloat() {
    perl -e "print ($2 ? $1 / $2 : 0)"',"\n"'
}
sec2h() {
    divfloat "$1" 3600.
}
mod_changed_diff() {
    echo2 "$*"
    vecdiff "$(modified $*)" "$(changed $*)"
}
modified() {
    # time of last write (not including inode changes)
    /usr/bin/stat -L -c %Y "$@"
}
changed() {
    # time of last inode change (mv chgrp creation) OR modification
    /usr/bin/stat -L -c %Z "$@"
}
accessed() {
    # time of last read
    /usr/bin/stat -L -c %X "$@"
}

darwin() {
    if [[ $ONDARWIN ]] ; then
        true
    else
        local OS=`uname`
        [ "${OS#Darwin}" != "$OS" ] && ONDARWIN=1
    fi
}
getrpath() {
    if darwin ; then
        otool -L "$@"
    else
        chrpath -l "$1" | perl -ne 'if (/RPATH=(.*)/) {
@r=split(/:/,$1);
print join(":",grep { -d $_ } @r);
}'
    fi
}
addrpath() {
    local f=$1
    shift
    if darwin ; then
        local p
        for d in "$@"; do
            if [[ -d $d ]] ; then
                echo $d
                p+=" -add_rpath '$d'"
            fi
        done
        install_name_tool $p $f
    else
        local p=$(getrpath $f)
        (
            set -e
            if [ "$*" ] ; then
                for d in "$@"; do
                    if [[ -d $d ]] ; then
                        echo $d
                        p="$d:$p"
                    fi
                done
            elif [[ $nocd != 1 ]] ; then
                p="$(pwd):$p"
            fi
            chrpath -r "$p" "$f"
        )
    fi
}
objrpath() {
    objdump -x "$1" | grep -i rpath
}

relpathr() {
    relpath `realpath "$1"` `realpath "$2"`
}
relhome() {
    local r=`realpath "$@"`
    if [ "$workflowreal" ] ; then
        local s=${r#$workflowreal}
        if [  "$s" != "$r" ] ; then
            echo workflow/$s
            return
        fi
    fi
    relpath $homereal $r
}

sniplong() {
    perl -e '$long=$ENV{cols} || 80; while(<>) { chomp;$_=substr($_,0,$long-3)."..." if length($_)>$long;print "$_\n" }' "$@"
}
trimlong() {
    sniplong "$@"
}
droplong() {
    perl -e '$long=$ENV{cols} || 80; ++$long; while(<>) { print unless length > $long; }' "$@"
}
filterlong() {
    droplong "$@"
}
ntimes() {
    local i
    local n=$1
    shift
    for i in $(seq 1 $n); do
        catz "$@"
    done
}
lastn() {
    local n=$1
    shift
    local darg=-d
    [ "$*" ] || darg=
    ls -rt $darg "$@" | tail -$n
}
last1() {
    lastn 1 "$@"
}
envtofile() {
    perl -e 'push @INC,"'$BLOBS'/libgraehl/latest";require "libgraehl.pl";print filename_from(join "-",map { $ENV{$_}==1?"$_":"$_=$ENV{$_}" } grep { $ENV{$_} } @ARGV);' "$@"
}

absp() {
    readlink -nfs "$@"
}
absdirname() {
    local b=${1:-.}
    dirname $(readlink -nfs "$b")
}
summarize_num() {
    perl ~graehl/t/graehl/util/summarize_num.pl "$@"
}
casub() {
    ( set -e;
        local d=$1
        [[ -f $1 ]] && d=`dirname $d`
        if [ "$2" ] ; then ln -sf $d $d.$2
            echo "$@" > $d/NOTES
        fi
        pushd $d

        shift

        if [[ -f $1 ]] ; then
            d=`basename $1`
        else
            d=`echo *.dag`
        fi
        [[ -f $d ]]
        echo ${d%.dag} $d
        set -x
        rm -f $d.{condor.sub,dagman.log,lib.out,lib.err,rescue}
        if [ "$hex" ] ; then
            perl -i -pe 's/quadcore/hexcore/g' *.sub
            grep hexcore *.sub
        fi
        vds-submit-dag $d
        popd
    )
}
casubs() {
    for f in "$@"; do
        ( set -e
            for d in *$f*0000; do
                [ -d $d ]
                casub $d
            done
        )
    done
}
cjobs() {
    perl -ne '$j{$1}=1 if /\((\d+)\.000\.000\)/; END { print "$_ " for (keys %j) }' "$@"
    echo
}
kjobs() {
    for j in `cjobs $1/*.dagman.log`; do
        echo $j
        condor_rm $j
    done
}

BLOBS=$(echo ~/blobs)
export BLOBS
WHOAMI=`whoami`
HOST=${HOST:-$(hostname)}
export TEMP=${TEMP:-/tmp}
export HADOOP_HOME=${HADOOP_HOME:-/home/nlg-01/chiangd/pkg/hadoop}
wordsn() {
    for i in $(seq 1 ${2:-1}); do
        echo -n "$1 "
    done
    echo
}
repn() {
    for i in $(seq 1 ${2:-1}); do
        echo -n "$1"
    done
    echo
}
hadlsr() {
    hadfs -lsr "$@"
}
hadls() {
    hadfs -ls "$@"
}
hadtest() {
    if [[ $local ]] ; then
        $1 "$2"
    else
        #        hadfs -test $1 "$2"
        #BUGGY. BUGGY BUGGY. terrible
        if [[ $1 = -e ]] ; then
            silently hadfs -stat "$2"
        elif [[ $1 = -d ]] ; then
            silently hadfs -ls "$2/*"
        else
            error "hadfs -test is buggy. I support -e and -d with stat and ls only"
        fi
    fi
}
hadisdir() {
    hadtest -d "$@"
}
hadexists() {
    hadtest -e "$@"
}
#hadempty() {
#    hadtest -z "$@"
#}
hadrm() {
    forall hadrm1 "$@"
}
hadrm1() {
    if [[ $1 != - ]]  ; then
        if [ "$local" ] ; then
            rm -f "$1"
        else
            hadfs -rmr "$1"
        fi
    fi
}
hadrmhad() {
    if ! [[ $local ]] ; then
        hadrm "$@"
    fi
}
hadpreview() {
    if [[ $1 != - ]]  ; then
        if [ "$local" ] ; then
            preview1 "$1" "local: $1"
        else
            hadcat "$1" 2>/dev/null | preview1 - "hadfs: $1" || true
        fi
    fi
}
hadput() {
    require_file "$1"
    if [[ ! $local ]] ; then
        hadfs -put "$1" $(basename "$1")
    fi
}
hadget() {
    local rpath=$(basename "$1")
    if [[ ! $local ]] ; then
        if hadexists "$1" ; then
            rm -f "$rpath"
            cmd=getmerge
            if hadisdir "$1" ; then
                cmd=getmerge
            fi
            hadfs -$cmd "$1" "$rpath"
        fi
    fi
    require_file "$rpath"
}
hadcat() {
    if [[ $local ]] ; then
        catz "$1"
    else
        hadfs -cat "$1/part-*" 2>/dev/null
    fi
}
hadfs() {
    $HADOOP_HOME/bin/hadoop fs "$@"
}

step() {
    local i=$1
    shift
    if [[ ${start_at:-0} -le $i ]] ; then
        echo2 "running step $i: $*"
        "$@"
    else
        echo2 "skipping step $i (start_at=$start_at): $*"
        false || [[ $nostepexit ]]
    fi
}
rwhich() {
    realpath $(which_default "$@")
}
previewf() {
    if [[ $1 != - ]]  ; then
        preview1 "$@"
    fi
}

libg=$BLOBS/libgraehl/latest

tabsort() {
    local tab=$(echo -e '\t')
    sort -t "$tab" "$@"
}

mapsort() {
    if [[ $savemap ]] ; then
        catz_to "$savemap"
        preview2 "$savemap"
        tabsort -k 1 "$savemap"
    else
        tabsort -k 1
    fi
}

make_nodefile() {
    if [[ $PBS_NODEFILE && -r $PBS_NODEFILE ]]; then
        export PBS_NODEFILE=$(mktemp /$TEMP/pbs_nodefile.XXXXXX)
        local i
        for i in $(seq 1 ${NODEFILE_N:-1}); do
            echo $HOST
        done > $PBS_NODEFILE
    fi
    #    echo $PBS_NODEFILE
}

scratchdir() {
    if [[ "$SCRATCH" && -d "$SCRATCH" ]] ; then
        echo $SCRATCH
    elif [ -d /scratch ] ; then
        echo /scratch
    else
        echo $(mktemp -d "$TEMP/scratch.XXXXXX")
    fi
}

save1() {
    #pipe stderr to out, saving original out to file arg1, for cmd arg2...argN
    local out="$1"
    shift
    "$@" 2>&1 1>"$out"
}

save12() {
    local out="$1"
    [[ -f $out ]] && mv "$out" "$out~"
    shift
    echo2 saving output $out
    "$@" 2>&1 | tee $out | ${page:=cat}
    echo2 saved output:
    echo2 `relpath ~ $out`
}


#keep error code but no output
silently() {
    "$@" 2>/dev/null >/dev/null
}

#no output, no error code
shush() {
    silently "$@" || true
}

#show output/log ONLY if error code (like silently but you get the filenames if something goes wrong)
quietly() {
    if [ "$verbose" -o "$showcmd" ] ; then
        echo +++ "$@"
    fi
    if [ "$verbose" ] ; then
        "$@"
    else
        local TEMP=${TEMP:-/tmp}
        local log=`mktemp $TEMP/quietly.log.XXXXXXX`
        local out=`mktemp $TEMP/quietly.out.XXXXXXX`
        if ! "$@" 2>$log >$out ; then
            local ret=$?
            tail $out $log
            echo $log $out
            return $ret
        else
            rm $out $log
        fi
    fi
}

verbose2() {
    if [ "$verbose" ] ; then
        echo2 "$@"
    fi
}
mapreduce_files() {
    local TEMP=${TEMP:-/tmp}
    local mapper=$1
    local reducer=${2:?usage: mapreduce mapper reducer [fileargs]}
    shift
    shift
    local temps=""
    echo2 "+++ mapreduce_files $mapper $reducer" "$@"
    while [ "$1" ] ; do
        local arg=$1
        shift
        [ -f $arg ] || continue
        local b=`basename --  $arg`
        local temp=`mktemp $TEMP/$b.XXXXXXX`
        temps="$temps $temp"
        verbose2 " +++ $mapper $arg > $temp"
        if ! $mapper $arg > $temp ; then
            local ret=$?
            error "Error during mapreduce_files: failed on '$mapper $arg'"
            [ "$savetmp" ] || rm $temps
            return $ret
        fi
    done
    verbose2 "+++ $reducer $temps"
    $reducer $temps
    local ret=$?
    [ "$savetmp" ] || rm $temps
    return $ret
}

cpdd() {
    if [ "$1" != "$2" ] ; then
        /bin/dd if="$1" of="$2" bs=1M
    fi
}
function filesz
{
    stat -L -c%s "$@"
}
function same_size
{
    [ `filesz "$1"` -eq `filesz "$2"` ]
}
cpdd_skip() {
    if [ -f "$2" ] && same_size "$@" ; then
        echo2 "skipping already copied $1 => $2"
    else
        echo2 "copying $1 => $2"
        cpdd "$1" "$2"
    fi
}
totmp() {
    for f in "$@"; do
        cpdd_skip $f $TMPDIR/`basename $f`
    done
}

clengths() {
    local p
    for f in "$@"; do
        lengths $f > $f.lengths
        p="$p $f.lengths"
        echo $f
    done
    paste $p
    echo paste $p

}

killdag() {
    pushd $1
    if [ -d 1btn0000 ] ; then
        id=`perl -ne 'if (/^\d+ \((\d+)\./) { print "$1\n";exit }' 1btn0000/1button.dag.dagman.log`
        if [ "$id" ] ; then
            echo $id
            condor_q
            condor_rm $id
        fi
    fi
    popd
}

debug() {
    PS4='+${BASH_SOURCE}:${LINENO}:${FUNCNAME[0]}: ' bash -x $*
}
_log() {
    if [ "$DEBUG" = "true" ] ; then
        echo 1>&2 _ "$@"
    fi
}
lexicon_vocab() {
    catz "$@" | perl -ne 'split;$c{$_[1]}++;$d{$_[2]}++;END{print scalar keys %c," ",scalar keys %d,"\n"}'
}

vocab() {
    catz "$@" | perl -ne '@l=split;$c{$_}++ for @l;END{print STDERR scalar keys %c,"\n";print $_," " for keys %c;print "\n"}'
}

quotevocab() {
    perl -ne '$t{$1}++ while /"([^" ]+)"/g;END{print "$_\n" for keys %t}' $*
}
invocab() {
    carmel --project-left $* | quotevocab
}
outvocab() {
    carmel --project-right $* | quotevocab
}


maybe_cp() {
    mkdir -p `dirname $2`
    echo cp $1 $2
    if [ ! -f $2 -o $1 -nt $2 ] ; then
        if [ "$backup" ] ; then
            bak=$2.maybecp~
            [ -f $2 ] && mv $2 $bak && echo2 backup at $bak
        fi
        cp $1 $2
        echo2 DONE
    else
        echo2 "NOT copying: $1 => $2 (latter is newer)"
        false
    fi
}

lengths() {
    #perl -ne '$l=(scalar split);print "$l\n"' "$@"
    catz "$@" | awk '{print NF}'
}

sum_lengths() {
    lengths "$@" | summarize-num
}
tab() {
    echo -ne "\t$*"
}

print() {
    echo -ne "$@"
}

print2() {
    echo -ne "$@" 1>&2
}

println() {
    echo -e "$@"
}

header() {
    echo "### $*"
    echo "############"
}


banner() {
    local prog=""
    if [[ $0 ]] && [[ $0 != -bash ]] ; then
        prog="[$(basename -- $0)]"
    fi
    header "$@" $prog
}

pmem_mb() {
    # assume w/o checking that line ends in "kB"
    head -n 1 /proc/meminfo | gawk '{ print int($2/1024) }'
}

pmem_avail() {
    local p=$((`pmem_mb` - 512))
    [ $p -le 256 ] && p=256
    echo $p
}

checkscript() {
    bash -u -n -x "$@"
}

dryrun() {
    bash -vn "$@"
}

function seq2
{
    seq -f %02g "$@"
}

function seq3
{
    seq -f %03g "$@"
}

function seq4
{
    seq -f %04g "$@"
}

cygwin() {
    if [[ $ONCYGWIN ]] ; then
        true
    else
        local OS=`uname`
        [ "${OS#CYGWIN}" != "$OS" ] && ONCYGWIN=1
    fi
}


ulimitsafe() {
    local want=${1:-131072}
    local type=${2:-s}
    local OS=`uname`
    if [[ ${OS#CYGWIN} != $OS ]] && [[ $type = s ]] ; then
        # error "cygwin doesn't allow stack ulimit change"
        return
    fi
    if [[ ${OS#MINGW} != $OS ]] ; then
        return
    fi
    # fix stack limits
    local soft=`ulimit -S$type`
    if [ ! "$soft" = 'unlimited' ]; then
        local hard=`ulimit -H$type`
        if [ "$hard" = 'unlimited' ]; then
            ulimit -S$type $want
        elif [ $hard -gt $want ]; then
            ulimit -S$type $want
        else
            ulimit -S$type $hard
        fi
    fi
}

balanced() {
    perl -n -e '$nopen=$nclose=0;++$nopen while (/\(/g);  ++$nclose while (/\)/g); warn("$nopen open and $nclose close parens") unless $nopen == $nclose;$no+=$nopen;$nc+=$nclose; END { print "$no opened, $nc closed parens\n"}' "$@"
}

#USAGE: assert "condition" $LINENO
assert() {
    E_PARAM_ERR=98
    E_ASSERT_FAILED=99
    [ "$2" ] || return $E_PARAM_ERR #must call with $LINENO
    local lineno=$2
    if ! eval [ $1 ] ; then
        echo2 "Assertion failed:  \"$1\" with exit=$?"
        echo2 "File \"$0\", line $lineno"
        exit $E_ASSERT_FAILED
    fi
}

# while getopts ":m:h" options; do
#   case $options in
#     m ) method=$OPTARG;;
#     * ) usage;;
#   esac
# done

# after getopt, OPTIND is set to the index of the first non-option argument - put remaining positional parameters into "$@" by this:
# shift $(($OPTIND - 1))

parent_dir() {
    local p=${1:-`pwd`}
    local dir=`dirname -- $p`
    basename --  $dir
}

finish_getopt() {
    return 0
}

usage() {
    echo2 USAGE:
    echo2 -e $usage
    local exitcode=$1
    shift
    if [ "$exitcode" ] ; then
        echo2 -e "$@"
        exit $exitcode
    fi
    exit 13
}

function showargs
{
    perl -e 'print "$_\n" while $_=shift' "$@"
}

is_interactive() {
    case "$-" in
        *i*)   return 0 ;;
        *)   return 1 ;;
    esac
}

on_err() {
    local exitcode=$?
    echo2
    errorq \(exit code $exitcode\)
        [ "${noexit+set}" = set ] || is_interactive || { echo2 ABORTING. ; exit 1; }
        return 1
}

trapexit() {
    trap "$*" EXIT INT TERM
}

traperr() {
    if [ "$BASH_VERSINFO" -gt 2 ] ; then
        set -o pipefail
        trap 'on_err' ERR
    fi
}

untraperr() {
    if [ "$BASH_VERSINFO" -gt 2 ] ; then
        trap ERR
    fi
}


echo2() {
    echo "$@" 1>&2
}

debug() {
    [ "$DEBUG" ] && echo2 DEBUG: "$@"
    return 0
}

debugv() {
    [ "$DEBUG" ] && echo2 DEBUG: "$*"=`eval echo "$@"`
    return 0
}


warn() {
    echo2 WARNING: "$@"
}

errorq() {
    echo2 ERROR: "$@"
}

die() {
    errorq "File \"$0\", line $lineno: " "$@"
    exit 19
}

error() {
    errorq "$@"
    return 2
}

usageline() {
    echo2 USAGE: "$@"
    return 2
}

nanotime() {
    date +%s.%N
}

tmpnam() {
    local TEMP=${TEMP:-/tmp}
    local nano=`nanotime`
    local suffix="$$$nano"
    echo $TEMP/$1$suffix
}

checksum() {
    local file=$1
    md5sum $file | cut -d' ' -f1
}

# $noexit (if set, return error code, else exit)
maybe_exit() {
    [ "$*" ] && errorq "$@"
    [ "${noexit+set}" = set ] || exit 1
    return 1
}

cpz_safe() {
    alltodir cpz_safe_one "$@"
}

cpz_safe_one() {
    if [ ${simulate_fail:-0} = 1 ] ; then
        simulate_fail=
        return 3
    fi
    if [ ! "$1" -a "$2" ] ; then
        usageline 'copyz in out[.gz]'
        return 2
    fi
    local infile=$1
    local outfile=$2
    if [ -d $outfile ] ; then
        outfile=$outfile/`basename --  $infile`
    fi
    local same=1
    local outgz
    local ingz
    local outbz2
    local inbz2
    [ "${outfile%.gz}" = "$outfile" ] || outgz=1
    [ "${infile%.gz}" = "$infile" ] || ingz=1
    [ "${outfile%.bz2}" = "$outfile" ] || outbz2=1
    [ "${infile%.bz2}" = "$infile" ] || inbz2=1
    [ "$outgz" = "$ingz" ] || same=0
    [ "$outbz2" = "$inbz2" ] || same=0
    [ "$outgz" -o "$outbz2" ] || same=1
    local checksumfile=`dirname -- $outfile`/.`basename --  $outfile`.md5
    if [ $same = 1 ] ; then
        cp $infile $outfile && [  `checksum $infile | tee $checksumfile` = `checksum $outfile` ]
    else
        if [ "$outbz2" ] ; then
            [ "$ingz" ] && warn not decompressing $infile ... compressing to $outfile
            bzip2 -c $infile > $outfile && [ `bunzip2 -c $outfile | checksum` = `checksum $infile | tee $checksumfile` ]
        else
            [ "$inbz2" ] && warn not decompressing $infile ... compressing to $outfile
            gzip -c $infile > $outfile && [ `gunzip -f -c $outfile | checksum` = `checksum $infile|  tee $checksumfile` ]
        fi
    fi
}

cpz() {
    alltodir cpz_one "$@"
}

cpz_one() {
    local oldnoexit=$noexit
    noexit=1
    cpz_for_sure_one "$@"
    local ret=$?
    noexit=$oldnoexit
    return $ret
}

cpz_for_sure() {
    alltodir cpz_for_sure_one "$@"
}

# $ncpzretry (default 5)
cpz_for_sure_one() {
    local max=${ncpzretry:-5}
    local TRYCPZ
    for TRYCPZ in `seq 1 $max`; do
        cpz_safe_one "$@" && return 0
    done
}

#
# affected by vars:
# $execstatus (if set, create outfile.FAILED on failure)
# $execstatusok (if set, create outfile.OK on success)
# $TEMP (default /tmp)
# $logerr (2>&1, save stderr to same file)
# $keepbad (delete output on bad exit code unless set)
# $nretry (default 5)
# $emptyok (assumes empty output is an error unless set)
# $simple (simple=1: no shell interpolation, pipelines, etc. are needed - simplifies shell quotation issues if set)
# $teeout : file contents also to stdout if set
# $removeold: if 1, remove old file first
# $mkdir: if 1, mkdir -f `dirname -- outfile`
exec_safe() {
    if [ ! "$1" -a "$2" ] ; then
        usageline 'noexit=1 execstatus=1 TEMP=/tmp logerr=1 keepbad=1 exec_safe output_file{.gz} command -and -arguments'
        return 2
    fi
    local outfile=$1
    shift
    local tmpfile=`tmpnam exec_safe`
    local max=${nretry:-5}
    local outdir=`dirname -- $outfile`
    if [ ! -d "$outdir" ] ; then
        if [ ${mkdir:-0} = 1 ] ; then
            echo2 "creating directory $outdir"
            mkdir -p $outdir || return 4
        else
            error "can't create outfile $outfile since the directory $outdir doesn't exist"
            return 5
        fi
    fi
    [ -L "$outfile" ] && echo2 "removing output $outfile because it is a symlink" && rm -f $outfile
    if [ -d "$outfile" ] ; then
        error "outfile $outfile is a directory"
        return 3
    fi
    [ ${removeold:-0} = 1 ] && rm -f $outfile
    rm -f $outfile.OK $outfile.FAILED
    local i
    touch $outfile.PENDING
    if have_file $outfile.PENDING ; then
        for i in `seq 1 $max`; do
            if [ ${logerr:-0} = 1 ] ; then
                if [ ${simple:-0} = 1 ] ; then
                    if [ ${teeout:-0} = 1 ] ; then
                        "$@" 2>&1 | tee $tmpfile || continue
                    else
                        "$@" > $tmpfile 2>&1 || continue
                    fi
                else
                    if [ ${teeout:-0} = 1 ] ; then
                        eval "$@" 2>&1 | tee $tmpfile || continue
                    else
                        eval "$@" > $tmpfile 2>&1 || continue
                    fi
                fi
            else
                if [ ${simple:-0} = 1 ] ; then
                    if [ ${teeout:-0} = 1 ] ; then
                        "$@"  | tee $tmpfile || continue
                    else
                        "$@" > $tmpfile || continue
                    fi
                else
                    if [ ${teeout:-0} = 1 ] ; then
                        eval "$@"  | tee $tmpfile || continue
                    else
                        eval "$@" > $tmpfile || continue
                    fi
                fi
            fi
            [ -f $tmpfile ] || continue
            [ -s $tmpfile ] || [ ${emptyok:-0} = 1 ] || continue
            cpz_for_sure $tmpfile $outfile || continue
            #echo2 OK
            rm -f $tmpfile
            [ ${execstatusok:-0} = 1 ] && touch $outfile.OK
            rm -f $outfile.PENDING
            return 0
        done
    fi
    [ ${execstatus:-0} = 1 ] && touch $outfile.FAILED
    rm $outfile.PENDING
    rm -f $outfile
    [ ${keepbad:-0} = 1 ] && cpz_safe $tmpfile $outfile
    rm -f $tmpfile
    error gave up after $max tries: exec_safe "$@"
}

is_abspath() {
    [ "${1:0:1}" = / ]
}

#usage: realprog gets real script location, d gets real script dir
getrealprog() {
    realprog=$0
    d=`dirname -- $realprog`
    if [ -L $realprog ] ; then
        if [ -x "`which_quiet readlink`" ] ; then
            while [ -L $realprog ] ; do
                realprog=`readlink "$realprog"`
            done
            if [ ${realprog:0:1} = / ] ; then #absolute path
                d=`dirname -- $realprog`
            else
                d=$d/`dirname -- $realprog`
            fi
        fi
    fi
}

# if you supply dirname --/basename -- , that's tried first, then just basename --  from path
# default: from PATH; then: from same directory as script; finally: the second argument if supplied
which_default() {
    #set -x
    local prog=$1
    if [ -z "$prog" ] ; then
        error "which_default progname defaultfilename - default: from PATH; then: from same directory as script; finally: the second argument if supplied"
        return 1
    fi
    local default=$2
    local ret=`which_quiet $prog 2>/dev/null`
    if [ -x "$ret" ] ; then
        echo $ret
    else
        prog=`basename --  $prog`
        ret=`which_quiet $prog 2>/dev/null`
        if [ -x "$ret" ] ; then
            echo $ret
        else
            getrealprog
            local ret=$d/$prog
            if [ -x "$ret" ] ; then
                echo $ret
            else
                if [ -x "$default" ] ; then
                    echo $default
                else
                    error No executable $prog found in PATH, $d, or at default $default
                fi
            fi
        fi
    fi
}

#usage: set abspath, call, read abspath
makeabspath() {
    if have_linux_readlink ; then
        abspath=`readlink -nfs "$abspath"`
        #  while [ -L $abspath ] ; do
        #     abspath=`readlink $abspath`
        #  done
    else
        if [ ${abspath:0:1} != / ] ; then #absolute path
            abspath="`pwd`/$abspath"
        fi
    fi
}
abspath() {
    if have_linux_readlink ; then
        readlink -nfs "$1"
    elif [ ${1:0:1} != / ] ; then #absolute path
        echo $(cd "$(dirname $1)"; pwd)/"$(basename $1)"
    else
        echo "$@"
    fi
}

catz1() {
    if [ -z "$1" -o "$1" = - ] ; then
        gunzip -f -c
    else
        while [[ ${1:-} ]] ; do
            if [ ! -f "$1" ] ; then
                echo "input file $1 not found" 1>&2
                exit -1
            fi
            if [ "${1%.gz}" != "$1" -o "${1%.tgz}" != "$1" ] ; then
                gunzip -f -c "$1"
            else
                if [ "${1%.bz2}" != "$1" ] ; then
                    bunzip2 -c "$1"
                else
                    cat "$1"
                fi
            fi
            shift
        done
    fi
}

catz() {
    forall catz1 "$@"
}

catz_to() {
    if [ -z "$1" -o "$1" = - ] ; then
        cat
    else
        if [ "${1%.gz}" != "$1" -o "${1%.tgz}" != "$1" ] ; then
            gzip -c > "$1"
        else
            if [ "${1%.bz2}" != "$1" ] ; then
                bzip2 -c > "$1"
            else
                cat > "$1"
            fi
        fi
    fi
}

function dos2unix()
{
    perl -i~ -pe 'y/\r//d' "$@"
}

function isdos()
{
    perl -e 'while(<>) { if (y/\r/\r/) { print $ARGV,"\n"; last; } }' "$@"
}

headz() {
    map_files headz_one_default "$@"
}

headz_one_default() {
    headz_one ${head:-8} "$@"
}

headz_one() {
    local n=$1
    shift
    if [ -z "$1" ] ; then
        catz $n | head
    else
        catz "$@" | head -n $n
    fi
}

first() {
    echo $1
}

tailz() {
    local n=$1
    shift
    if [ -z "$1" ] ; then
        catz $n | tail
    else
        catz "$@" | tail -n $n
    fi
}

wcz() {
    nlinesz "$@"
}

nlines() {
    catz "$@" | wc -l
}

haslines() {
    local n=$1
    shift
    [ `nlines "$@"` -ge $n ]
}

# get nth line from stdin
function getline_stdin
{
    local line=$1
    local text
    # head -n $line | tail -n 1
    local n=0
    while [ $n -lt $line ] ; do
        if ! read -r text ; then
            return 1
        fi
        n=$((1+$n))
        if [ $n = $line ] ; then
            echo -E "$text"
        fi
    done
}

function getline_stdin
{
    perl -e 'BEGIN{$l=shift};while(<>){ if (--$l==0) { print; exit; } } exit 1' "$@"
}

# get line number n
get() {
    local line=$1
    shift
    catz "$@" | getline_stdin $line
    # if [ ! "$*" ] ; then
    # getline_stdin $line
    # else
    #  if haslines $line $* ; then
    #   catz $* | getline_stdin $line
    #  else
    #   return 1
    #  fi
    # fi
}

# get n lines, starting from optional 2nd arg
getn() {
    local n=$1
    shift
    if [ "$1" ] ; then
        local h=$(($1+$n - 1))
        shift
    fi
    headz -$h "$@" | tail -$n
}

#get n bytes, starting from optional 2nd arg
getnbytes() {
    local n=$1
    shift
    local offset=$1
    shift
    catz "$@" | dd bs=1 skip=$offset count=$n
}

getline() {
    get "$@"
}

# sort stdin lines with key following first argument.  e.g. sortbynum id= would sort lines by id=N
sortby() {
    perl -e '@l=<>;@w=map {$_->[0]} sort { $a->[1] cmp $b->[1] } map {/\Q'"$1"'\E(\S+)/;[$_,$1]} @l; print @w'
}
sortbynum() {
    perl -e '@l=<>;@w=map {$_->[0]} sort { $a->[1] <=> $b->[1] } map {/\Q'"$1"'\E(\S+)/;[$_,$1]} @l; print @w'
}
sortdefbynum() {
    perl -e '@l=<>;@w=map {$_->[0]} sort { $a->[1] <=> $b->[1] } grep { defined($_->[1]) } map {/\Q'"$1"'\E(\S+)/;[$_,$1]} @l; print @w'
}
sortdefby() {
    perl -e '@l=<>;@w=map {$_->[0]} sort { $a->[1] cmp $b->[1] } grep { defined($_->[1]) } map {/\Q'"$1"'\E(\S+)/;[$_,$1]} @l; print @w'
}
sortbynumabsinterp() { # alpha*f1+f2
    perl -e '@l=<>;@w=map {$_->[0]} sort { $a->[1] <=> $b->[1] } grep { defined($_->[1]) } map {/\Q'"$2"'\E(\S+)/;$x='"$1"'*abs($1);/\Q'"$3"'\E(\S+)/;[$_,$x+$1]} @l; print @w'
}
sortbynumprod() { # f1*f2
    perl -e 'sub makenum {
local($_)=@_;
defined ($_) ? $_ : -1e100
}
@l=<>;
@w=map {$_->[0]} sort { $a->[1] <=> $b->[1] } grep { defined($_->[1]) }
map {/\Q'"$1"'\E(\S+)/;$x=&makenum($1);/\Q'"$2"'\E(\S+)/;[$_,$x*&makenum($1)]} @l; print @w'
}


getparams() {
    catz "$@" | perl -e 'while(<>) { last if /^COMMAND LINE/; } <>; while(<>) { last if /^>>>* PARAMETERS/; print $_; }'
}
getcmdline() {
    catz "$@" | perl -e 'while(<>) { last if s/.*COMMAND LINE:\s*//; } $_=<> unless /\S+/; print;'
}
getparam() {
    local name=$1
    shift
    getparams "$@" | perl -n -e 'print $1,"\n" if /^\w+ \Q'"$name"'\E=(.*)/'
}
timestamp() {
    date +%Y.%m.%d_%H.%M
}

getlast() {
    ls -t "$@" | head -n 1
}

getlastgrep() {
    local pattern=shift
    ls -t "$@" | grep "$pattern" | head -n 1
}

getlastegrep() {
    local pattern=shift
    ls -t "$@" | egrep "$pattern" | head -n 1
}

getlastfgrep() {
    local pattern=shift
    ls -t "$@" | fgrep "$pattern" | head -n 1
}

getlaststar() {
    local f
    for f in "$@"; do
        ls -t "$f"* | head -n 1
    done
}


safefilename() {
    echo "$@" | perl -pe 's/\W+/./g'
}

sortdiff() {
    [ -f $1 ] || error file 1: $1 not found
    [ -f $2 ] || error file 2: $2 not found
    f1=$1
    f2=$2
    shift
    shift
    local tmp1=`safefilename $f1`
    local tmp1=`tmpnam $tmp1`
    local tmp2=`safefilename $f2`
    local tmp2=`tmpnam $tmp2`
    catz $f1 | sort "$@" > $tmp1
    catz $f2 | sort "$@" > $tmp2
    diff -u -b $tmp1 $tmp2
    rm $tmp1
    rm $tmp2
}

bwhich() {
    which_quiet "$@"
    local syswhich=`/usr/bin/which "$@"`
    [ -f $syswhich ] && ls -l $f 1>&2
}

which_quiet() {
    builtin type -a "$@" | perl -n -e 'if (m#(?: is (/.*)|aliased to \`(.*)\'\'')$#) { print "$1$2\n";exit 0; };if (m#(.*)( is a function)#) { print "$1$2\n";print while(<>)}'
    #cut -d' ' -f3-
}
qwhich() {
    which_quiet "$@"
}
where() {
    builtin type -a "$@"
}

# echocsv 1 2 3 => 1, 2, 3
echocsv() {
    local a=$1
    shift
    echo -n $a
    while [ "$1" ] ; do
        a=$1
        shift
        echo -n ", $a"
    done
    echo
}


is_text_file() {
    local t=`tmpnam`
    file "$@" > $t
    grep -q text $t
    local ret=$?
    rm $t
    return $ret
}

##### ENVIRONMENT VARIABLES as SCRIPT OPTIONS:
showvars_required() {
    echo2 $0 RUNNING WITH REQUIRED VARIABLES:
    local k
    for k in "$@"; do
        eval local v=\$$k
        echo2 $k=$v
        if [ -z "$v" ] ; then
            errorq "required (environment or shell) variable $k not defined!"
            return 1
        fi
    done
    echo2
}

showvars() {
    echo2 $0 RUNNING WITH VARIABLES:
    local k
    for k in "$@"; do
        eval local v=\$$k
        echo2 $k=$v
    done
    echo2
}

showvarsq() {
    local k
    for k in "$@"; do
        eval local v=\$$k
        echo -ne "$k=$v " 1>&2
    done
    echo2
}

showvars_optional() {
    echo2 RUNNING WITH OPTIONAL VARIABLES:
    local k
    for k in "$@"; do
        if isset $k ; then
            eval local v=\$$k
            echo2 $k=$v
        else
            echo2 UNSET: $k
        fi
    done
    echo2
}
showvars_files() {
    echo2 $0 RUNNING WITH FILES FROM VARIABLES:
    local k
    for k in "$@"; do
        eval local v=\$$k
        local r=`realpath $v`
        echo2 "$k=$v [$r]"
        require_files $v
    done
    echo2
}

showvars_filedirs() {
    echo2 $0 RUNNING WITH FILES FROM VARIABLES:
    local k
    for k in "$@"; do
        eval local v=\$$k
        local r=`realpath $v`
        echo2 "$k=$v [$r]"
        [ -e "$v" ] || return 1
    done
    echo2
}

showvars_dirs() {
    echo2 $0 RUNNING WITH DIRECTORIES FROM VARIABLES:
    local k
    for k in "$@"; do
        eval local v=\$$k
        local r=`realpath $v`
        echo2 "$k=$v [$r]"
        [ -d "$v" ] || return 1
    done
    echo2
}

greplines() {
    grep -n "$@" | cut -f1 -d:
}

# see ~graehl/bin/extract-field*
getfield() {
    perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl';&argvz;"'$f=shift;while(<>) {$v=&getfield($f,$_);print "$v\n" if defined $v;}' -- "$@"
}

getfield_blanks() {
    perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl';&argvz;"'$f=shift;while(<>) {$v=&getfield($f,$_);print "$v\n";}' -- "$@"
}

getfields_blanks() {
    perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl';&argvz;"'$IFS="\t";while(defined($l=<STDIN>)) {@v=map {&getfield($_,$l)} @ARGV;print "@v\n"}' -- "$@"
}

getfirstfield_blanks() {
    perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl';&argvz;"'while(defined($l=<STDIN>)) {for (@ARGV) { $f=&getfield($_,$l); last if defined $f; } print "$f\n"}' -- "$@"
}

getfields_blanks_header() {
    perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl';&argvz;"'$IFS="\t";print "@ARGV\n";while(defined($l=<STDIN>)) {@v=map {&getfield($_,$l)} @ARGV;print "@v\n"}' -- "$@"
}

perfs() {
    catz "$@" | grep 'wall seconds'
}

grep_nullchar() {
    #    xxd "$@" | (grep -m 1 -e 0x00)
    catz "$@" | perl -w -e 'use strict; while (defined ($_=getc)) { die "NUL READ\n" if $_ eq "\x0"; }'
}

grep_funnychars() {
    # grep -m 1 -e '[^ -~]' "$@"
    LC_CTYPE= grep -m 1 '[[:cntrl:]]' "$@"
}

# why do we capture output?  because in bash pre-v3, pipeline exit codes are lost.
validate_nonull() {
    local n=`catz "$@" | grep_nullchar 2>&1`
    local files="$*"
    [ "$files" ] || files="STDIN"
    if [ -z "$n" ] ; then
        return 0
    fi
    warn "NUL (0, aka ^@) CHARACTER DETECTED in $files"
    return 1
}

validate_english() {
    local c=`catz $* | perl -pe chomp | grep_funnychars -c`
    local files="$*"
    [ "$files" ] || files="STDIN"
    if [ "$c" = "1" ] ; then
        warn "FUNNY (non-ASCII-printable) CHARACTERS DETECTED in $files"
        return 1
    fi
    return 0
}

expand_template() {
    perl -e '$t=shift;$i=shift;$mustexist=shift;$nocheck=!$mustexist;$t=~s/\{\}/$i/g;print "$t\n" if ($nocheck or -f $t)' "$@"
}

#usage: $maxi sets default upper bound
maxi=99
mustexist=1
fileseq() {
    #mustexist
    local template=$1
    shift
    local a=${1:-0}
    shift
    local b=${1:-$maxi}
    shift
    local i
    local s=`seq $a $b`
    [ "$nofill" ] || s=`seq -f '%02g' $a $b`
    for i in $s; do
        expand_template $template $i $mustexist
    done
}

ifiles() {
    mustexist=1 fileseq "$@"
}

ofiles() {
    mustexist=0 fileseq "$@"
}

# starts with an initial file, and increments the number part of the filename as long as more files are found, e.g. line1, line2, line3 ... lineN (gaps will end the sequence)
files_from() {
    # perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl';"'while(defined ($f=shift)) { while(-f $f) { print "$f\n";$f=inc_numpart($f); } }' "$@"
    perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl'"';@files=files_from(@ARGV);print "$_\n" for (@files);' -- "$@"
}

filename_from() {
    # perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl';"'while(defined ($f=shift)) { while(-f $f) { print "$f\n";$f=inc_numpart($f); } }' "$@"
    perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl'"';$"=" ";print filename_from("@ARGV"),"\n"' -- "$@"
}

chmod_notlinks() {
    local mode=$1
    shift
    local dir=${1:-.}
    find $dir \( -type f -o -type d \) -exec chmod -f $mode {} \;
}

interleave() {
    paste -d'\n' "$@" /dev/null
}

sidebyside() {
    local files=''
    local desc='#'
    local f
    for f in "$@"; do
        local t=`mktemp /tmp/sidebyside.XXXXXXXXX`
        files="$files $t"
        desc="$desc $f"
        nl -ba $f > $t
    done
    echo $desc
    paste -d'\n' $files /dev/null
    # $cmd
    rm -f $files
}

# optional 2nd arg: 1st file must exist *and* be newer than 2nd.  optional (env) var require_nonempty
have_file() {
    [ "$1" -a -f "$1" -a \( -z "$2" -o "$1" -nt "$2" \)  -a \( -z "$require_nonempty" -o -s "$1" \) ]
}

have_dir() {
    [ "$1" -a -d "$1" ]
}

have_data() {
    [ "$1" -a -s "$1" ]
}

require_files() {
    local f
    [ "$*" ] || error "require_files called with empty args list"
    for f in "$@"; do
        if ! have_file "$f" ; then
            error "missing required file: $f"
            return 1
        fi
    done
    return 0
}

have_remote_file() {
    #env remote: ssh host. arg1: absolute path on remote.  return true if file exists
    local r=${remote:-hpc.usc.edu}
    local u=${remoteuser:-$WHOAMI}
    [ "$1" ] || return 1
    ssh $r -l $u <<EOF
[ -f "$1" ]
EOF
}

have_remote_dir() {
    #env remote: ssh host. arg1: absolute path on remote.  return true if file exists
    local r=${remote:-hpc.usc.edu}
    local u=${remoteuser:-$WHOAMI}
    [ "$1" ] || return 1
    ssh $r -l $u <<EOF
[ -d "$1" ]
EOF
}

have_files() {
    local f
    [ "$*" ] || error "have_files called with empty args list"
    for f in "$@"; do
        have_file "$f" || return 1
    done
    return 0
}

function show_outputs
{
    [ "$*" ] || error "show_outputs called with empty args list" || return 1
    show_tailn=${show_tailn:-4}
    [ 0$show_tailn -gt 0 ] || return 0
    [ $# = 1 ] && echo '==>' $1 '<=='
    tail -n $show_tailn "$@"
    [ $# = 1 ] && echo
    wc -l "$@"
}


skip_status() {
    #  [ "$quiet_skip" ] || echo2 "$@"
    #  return 0
    if ! ["$quite_skip" ] ; then
        echo2 "skip_files: start_at=$start_at stop_at=$stop_at r=$*"
    fi
}

not_skipping() {
    working_files="$*"
}

function show_stale_files
{
    if [ "$stale_files" ] ; then
        echo
        header "STALE (skip_files skipped) output files:"
        show_outputs $stale_files
    fi
}

function show_new_files
{
    if [ "$new_files" ] ; then
        echo
        header "NEW (skip_files regenerated) output files:"
        show_outputs $new_files
    fi
}

function working_elapsed
{
    echo $((`date +%s`-$working_start)) s elapsed
}

function skip_done
{
    [ "$working_files" ] && echo2 `working_elapsed` producing $working_files && require_files $working_files && new_files="$new_files $working_files"
    true
}

function new_file
{
    require_files "$@"
    new_files="$new_files $*"
}

function skip_done_bg
{
    [ "$working_files" ] && echo2 backgrounded producing $working_files && new_files="$new_files $working_files"
    true
}

# usage: newer_than=file skip_files 1 out1 ... || something > out1; skip_done; ... show_new_files
# first arg: $start_at must be lessequal to $1 or else unset (otherwise we skip). see also stop_at .  remaining args: files that should exist or else regenerate them all.  returns true (0) if you can skip, false (1) if you can't.
# if first arg is >= stop_at, then don't run (return true) no matter if outputs exist or not
skip_files() {
    #    echo2 skip_files "$@"
    working_files=
    working_start=`date +%s`
    local r=$1
    shift
    if ! [ "$*" ]  ; then
        warn "skip_files has empty list of output files as args.  rerunning command always."
        false
    fi
    #    echo2 "require $r < $start_at - files: $*"
    if [ -n "$stop_at" -a $r -ge 0$stop_at ] ; then
        stale_files="$stale_files $*"
        skip_status "$r skipping - stop_at=$stop_at is <= $r" && return 0
    fi
    if [ 0$start_at -gt $r ] ; then
        # might skip if files are already there:
        local f
        for f in "$@"; do
            if ! have_file $f $newer_than ; then
                skip_status "$r rerunning, for missing output $f"
                not_skipping "$@"
                return 1
            fi
        done
        stale_files="$stale_files $*"
        skip_status "$r skipping - had all files $*" && return 0
    fi
    not_skipping "$@"
    skip_status "$r rerunning to produce $*" && return 1
}


# usage: newer_than=file skip_files 1 out1 ... || something > out1; skip_done; ... show_new_files
# first arg: $start_at must be greater than $1 .  remaining args: files that should exist or else regenerate them all.  returns true (0) if you can skip, false (1) if you can't.
# if first arg is >= stop_at, then don't run (return true) no matter if outputs exist or not
have_outputs() {
    #    echo2 skip_files "$@"
    working_files=
    working_start=`date +%s`
    local r=$1
    shift
    if ! [ "$*" ]  ; then
        warn "skip_files has empty list of output files as args.  rerunning command always."
        return 0
    fi
    #    echo2 "require $r < $start_at - files: $*"
    if [ -n "$stop_at" -a $r -ge 0$stop_at ] ; then
        stale_files="$stale_files $*"
        skip_status "$r skipping - stop_at=$stop_at is <= $r" && return 1
    fi
    if [ 0$start_at -gt $r ] ; then
        # might skip if files are already there:
        local f
        for f in "$@"; do
            if ! have_file $f $newer_than ; then
                skip_status "$r rerunning, for missing output $f"
                not_skipping "$@"
                return 1
            fi
        done
        stale_files="$stale_files $*"
        skip_status "$r skipping - had all files $*" && return 1
    fi
    if [ 0$force_start_at -gt $r ] ; then
        warn "forcing start at $force_start_at even if files are stale"
        return 0
    fi
    not_skipping "$@"
    skip_status "$r rerunning to produce $*" && return 0
}

require_dirs() {
    local f
    [ "$*" ] || error "require_dirs called with empty args list"
    for f in "$@"; do
        have_dir $f || error "missing required dir: $f" || return 1
    done
}

require_dir() {
    require_dirs "$@"
}

require_file() {
    require_files "$@"
}

require_data() {
    [ "$1" ] || error "require_data on empty filename"
    require_file $1
    [ -s "$1" ] || error "required file: $f is EMPTY!"
    # FIXME: check for empty .gz and .bz2?
}

trim_ws() {
    perl -pe 's/^\s+//;s/\s+$//;' "$@"
}

get_fields() {
    perl -ae '$,=" ";while(<STDIN>) {@F=split; print (map {$F[$_-1]} @ARGV);print "\n"}' "$@"
}

alias showargv="perl -e 'print \"======\\n\$_\\n\" while(defined(\$_=shift));'"


#fake qsubrun-depend.sh - only run 1st arg
eval1st() {
    eval $1
}

wildcardfound() {
    [ "$1" ] && [ "$1" = $1 ]
}

sum() {
    perl -ne '$s+=$_;END{print "$s\n"}' "$@"
}

#for smoothing freq of freq lines: (freq #items-with-freq)
sumff() {
    perl -ane '$s+=$F[0]*$F[1];$r+=$F[1];END{print "$s $r\n"}' "$@"
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

basereal() {
    basename $(realpath "$@")
}

dirreal() {
    dirname $(realpath "$@")
}

skipfirst() {
    catz | (read;cat)
}

alltodir() {
    local allNARGS=$#;
    local allLAST=$(($allNARGS-1))
    local all_args_arr
    declare -a all_args_arr
    all_args_arr=($*);
    local allcmd=${all_args_arr[0]}
    local II=1
    local DEST=${all_args_arr[$allLAST]}
    if [ $allLAST -lt 3 -o -d $DEST ] ; then
        while [ $II -lt $allLAST ]; do
            echo2 "+++++++++ $allcmd ${all_args_arr[$II]} $DEST/"
            if ! $allcmd ${all_args_arr[$II]} $DEST/ ; then
                error "Error on item $II in 'alltodir $allcmd $*' - failed on '$allcmd ${all_args_arr[$II]} $DEST/'"
                return $?
            fi
            II=$(($II+1))
        done
    else
        error "last argument is not a directory - usage: alltodir cmd a b c dir/ => cmd a dir/;cmd b dir/;cmd c dir/"
    fi
}

forall() {
    # simpler than map, we don't exit on error
    local cmd=$1
    shift
    local f
    for f in "$@"; do
        $cmd "$f"
    done
}
forall0() {
    #if args empty, call cmd with no args once
    local cmd=$1
    shift
    if [ "$*" ] ; then
        forall $cmd "$@"
    else
        $cmd
    fi
}
map() {
    local cmd=$1
    shift
    while [ "$1" ] ; do
        [ "$quiet" ] || echo2 "+ $cmd $1"
        if ! $cmd $1 ; then
            error "Error during map: failed on '$cmd $1'"
            return $?
        fi
        shift
        echo2
    done
}

pipemap() {
    local cmd=$1
    shift
    while [ "$1" ] ; do
        echo2 "+ catz $1 | $cmd"
        eval "catz $1 | $cmd"
        shift
        echo2
    done
}

map_files() {
    local cmd=$1
    shift
    while [ "$1" ] ; do
        local arg=$1
        shift
        [ -f $arg ] || continue
        echo2 "++++++++++ $cmd $arg"
        if ! $cmd $arg ; then
            error "Error during map: failed on '$cmd $arg'"
            return $?
        fi
        echo2
    done
}


stripunk() {
    perl -i -pe 's/\(\S+\s+\@UNKNOWN\@\)//g;s/\@UNKNOWN\@//g;' "$@"
}

basecd() {
    basename --  `pwd`
}

topnode() {
    local node=$1
    shift
    local n=${1:-30}
    ssh $node "top b n 1 | grep -v root | head -n $n"
}

lnreal() {
    # echo2 $#
    local forceln
    local multi=2
    if [ "$1" = -f ] ; then
        forceln=-f
        multi=3
    fi
    # echo2 $#
    if [ $# = $multi ] ; then
        lnreal_one "$@"
    else
        alltodir lnreal_one "$@"
    fi
}

lnreal_one() {
    local force
    if [ "$1" = -f ] ; then
        force=true
        shift
    fi
    local dest=$2
    [ -d $dest -a ! -L $dest ] && dest=$dest/`basename --  $1`
    [ "$force" ] && [ -f $dest -o -L $dest ] && rm -f $dest
    ln -s `realpath $1` $dest
}

gaps() {
    perl -ne '$|=1;print if ( !( $_ || /^0[.]?0?$/) || $last && abs($last-$_)!=1);$last=$_;' "$@"
}

non_finalize() {
    perl -pe 's/(finalize\s*\[\d+,\d+\]\.\s*done.\s*)*//' "$@"
}

non_nbest() {
    catz "$@" |  non_finalize | grep -v "^NBEST "
}

preview_banner() {
    echo "==> $* <=="
}

headtail() {
    forall0 headtail1 "$@"
}
htpreview1() {
    if [[ $2 ]] ; then
        preview_banner $2
    else
        preview_banner $1
    fi
    headtail1 "$1"
    echo
}
htpreview() {
    forall htpreview1 "$@"
}
preview() {
    forall preview1 "$@"
}
head1() {
    local n=${tailn:-6}
    if [[ $2 ]] ; then
        n=$1
        shift
    fi
    if [[ $1 = - ]] ; then
        head -n $n
    else
        head $tailarg -n $n "$1"
    fi
}
tail1() {
    local n=${tailn:-6}
    if [[ $2 ]] ; then
        n=$1
        shift
    fi
    if [[ $1 = - ]] ; then
        tail -n $n
    else
        tail $tailarg -n $n "$1"
    fi
}
headtailp() {
    #todo: use fixed sized queue
    perl -e '$n=shift||5;@l=<>;$nl=scalar @l;if ($nl<=2*$n+1) { print @l } else { print @l[0..$n-1],"...\n",@l[scalar -$n..-1] }' "$@"
}
headtail1() {
    local n=${tailn:-6}
    if true || ! [[ $1 ]] || [[ $1 = - ]] ; then
        headtailp $n "$@"
    else
        local n=$(nlines "$1")
        local m=$((2 * $n + 1))
        #showvars_required n m
        if [ $n -le $m ] ; then
            cat "$1"
        else
            head1 $n "$1"
            echo ...
            tail1 $n "$1"
        fi
    fi
}
preview1() {
    (
    tailn=${tailn:-12}
    if [[ $headn ]] ; then
        tailn=$headn
    fi
    local v="-v"
    if [[ ${2:-} ]] ; then
        preview_banner $2
        v=
    elif [[ $(uname) = Darwin && ${v:-} ]] ; then
        v=
        if [ "$1" ] ; then
            preview_banner "$1"
        else
            preview_banner "<STDIN>"
        fi
    fi
    tailarg=$v head1 $tailn "$@"
    )
}
preview2() {
    tailn=$tailn preview "$@" 1>&2
}

diff_filter() {
    showvars_required filter || return 1
    local b1=`basename --  $1`
    local b2=`basename --  $2`
    local t1=`mktemp $TEMP/$b1.XXXXXXX`
    local t2=`mktemp $TEMP/$b2.XXXXXXX`
    catz $1 | $filter > $t1
    catz $2 | $filter > $t2
    echo2 "DIFF: - is only in $1, + is only in $2"
    diff -u $t1 $t2
    [ "$ediff" ] && ediff $t1 $t2
    rm $t1 $t2
}

diffhead() {
    local b1=`basename --  $1`
    local b2=`basename --  $2`
    local t1=`mktemp $TEMP/$b1.XXXXXXX`
    local t2=`mktemp $TEMP/$b2.XXXXXXX`
    local n=${3:-100}
    if [ "$3" ] ; then
        catz $1 | head -n $3 > $t1
        catz $2 | head -n $3 > $t2
    else
        catz $1 > $t1
        catz $2 > $t2
    fi
    echo2 "DIFF: - is only in $1, + is only in $2"
    diff -u -b $t1 $t2
    rm $t1 $t2
}

diffline() {
    local n=$1
    shift
    [ "$n" -gt 0 ] || return 1
    local b1=`basename --  $1`
    local b2=`basename --  $2`
    local t1=`mktemp $TEMP/$b1.XXXXXXX`
    local t2=`mktemp $TEMP/$b2.XXXXXXX`
    getline $n $1 > $t1
    getline $n $2 > $t2
    #echo2 "DIFF: - is only in $1, + is only in $2"
    if ! diff -q $t1 $t2 >/dev/null ; then
        echo2 -e "\t$n"
        if [ "$ediff" ] ; then
            echo "DIFF: - was from $1, + was added to $2"
            ediff $t1 $t2
        else
            cat $t1 $t2
        fi
    fi
    rm $t1 $t2
}

difflines() {
    require_files $1 $2
    local n1=`nlines $1`
    local n2=`nlines $2`
    echo2 different lines from `basename --  $1` to `basename --  $2`:
    for n in `seq 1 $n1`; do
        diffline $n $1 $2
    done
}

diffz() {
    diffhead "$@"
}

showlocals() {
    local t1=`mktemp $TEMP/set.XXXXXXX`
    local t2=`mktemp $TEMP/printenv.XXXXXXX`
    set | sort > $t1
    printenv | sort > $t2
    diff $t1 $t2 | grep "<" | awk '{ print $2 }' | grep -v '^[{}]'
}

isset() {
    eval local v=\${$1+set}
    [ "$v" = set ]
}

isnull() {
    eval local v=\${$1:-}
    [ -z "$v" ]
}

notnull() {
    eval local v=\${$1:-}
    [ "$v" ]
}

nwords() {
    catz "$@" | perl -ne 'next if /^$/;$n++;$n++ while / /g;END{print "$n\n"}'
}

nlinesz() {
    pipemap 'nlines' "$@"
}

countblanks() {
    catz "$@" | grep -c '^$'
}

skipn_pipe() {
    for i in `seq 1 $1`; do
        read
    done
    cat
}

skipn() {
    local a=$1
    shift
    catz "$@" | skipn_pipe $a
}

wordsperline() {
    catz "$@" | perl -ane '$n=(scalar @F);$s+=$n;++$N;print "$n\n";END{print STDERR "total words $s, lines $N, avg ",$s/$N,"\n"}'
}


wordsplit() {
    perl -ane '$a=shift @F;print "$a\n";print " $_\n" for (@F)' "$@"
}

ediff() {
    local b1=`basename --  $1`
    local b2=`basename --  $2`
    local t1=`mktemp $TEMP/$b1.XXXXXXX`
    local t2=`mktemp $TEMP/$b2.XXXXXXX`
    catz $1 |  wordsplit > $t1
    catz $2 | wordsplit > $t2
    diff -y $t1 $t2
    rm $t1 $t2
}

escaped_args() {
    while [ $# -ge 1 ]; do
        perl -e "push @INC,'$BLOBS/libgraehl/latest';require 'libgraehl.pl';println(escaped_shell_args_str("@ARGV"));" -- $1
        shift
    done
}

savelog() {
    local logto=$1.`timestamp`.gz
    have_file $1 && catz $1 | gzip -c > $logto
    echo2 saved log to $logto
}

rotatelog() {
    local logto=$1.`timestamp`.gz
    have_file $1 && gzip $1
    have_file $1.gz && mv $1.gz $1.`timestamp`.gz && echo moved old log to $1.`timestamp`.gz
}


#PBS queue aliases
qsinodes=${qsinodes:-1}
qsippn=${qsippn:-1}
qsiq=${qsiq:-isi}
# anything
qsubi() {
    qsub -q $qsiq -I -l nodes=$qsinodes:ppn=$qsippn,walltime=150:00:00,pmem=3000mb -X "$@"
}
# big 64bit
qsi() {
    qsub -q $qsiq -I -l nodes=$qsinodes:ppn=$qsippn,walltime=150:00:00,pmem=15000mb,arch=x86_64 -X "$@"
}

qjobid() {
    echo "$@" | sed 's/^\(\d+\).*/\1/'
}

grep_running() {
    grep "$PBSQUEUE" | grep " R "
}

qrunning() {
    # local PBSQUEUE=${1:-isi}
    PBSQUEUE=${PBSQUEUE:-isi}
    local j=`qjobid`
    local n=`qstat -a $j | grep_running | wc -l`
    echo "$n jobs running on $PBSQUEUE"
}

qusage() {
    PBSQUEUE=${PBSQUEUE:-isi}
    qstat -a | grep_running
    qrunning
}

qls() {
    PBSQUEUE=${PBSQUEUE:-isi}
    if [ "$*" ] ; then
        qstat -an `qjobid "$@"`
    else
        qstat -an1 | grep "$PBSQUEUE"
    fi
    qrunning
}

my_jobids() {
    local me=$WHOAMI
    qstat -u $me | grep $me | cut -d. -f1
}

myjobs() {
    local me=$WHOAMI
    local which=${1:-.}
    qstat -an1 -u $me | grep $me | egrep -i " $which "
}

forall_jobs() {
    local cmd=$1
    shift
    #    local jobs=`my_jobids`
    local id
    for id in `my_jobids`; do
        echo $cmd "$@" $id
        $cmd "$@" $id
    done
}

logname() {
    echo ${log:-log.`filename_from "$@"`}
}

watch() {
    disown
    echo "$@"
    while ! [ -f "$@" ] ; do
        echo waiting for "$@"
        sleep 1
    done
    tail -f "$@"
}
log() {
    local log=`logname "$@"`
    nohup "$@" > $log &
    watch $log
}
loge() {
    local log=`logname "$@"`
    nohup env "$@" > $log 2>&1 &
    watch $log
}
workflowp=~/workflow
[ -d $workflowp ] &&  workflowreal=`realpath $workflowp`
homereal=`realpath ~`
traperr
getrealprog
libg=$(echo ~graehl/u)
if ! [[ -d $libg ]] || ! [[ -f $libg/libgraehl.pl ]] ; then
    warn missing libgraehl.pl
fi

radutrees() {
    local t=$1
    require_file $t
    perl -ne '++$nnt while (/~\d+~\d+/g); ++$nw while (/\(\S+ \S+\)/g);END{print "NT=$nnt WORDS=$nw TOTAL=",$nw*2+$nnt,"\n";}' $t
}

diffweights() {
    ~/graehl/bin/diffweights.pl "$@"
}

mycond() {
    condor_q -format "%s " Iwd -format "%d " ClusterId -format '%d' DAGManJobId -format '\n' nofieldph $USER
}

rgrep()
{
    local a=$1
    shift
    if [ "$*" ] ; then
        find "$@" -exec egrep "$a" {} \; -print
    else
        find . -exec egrep "$a" {} \; -print
    fi
}

# must be called with files, not stdin
getnbest() {
    if [ 0 != `catz "$@" | grep -c '^NBEST '` ] ; then
        getpassf "$@"
    else
        getpass1 "$@"
    fi
}

getbests() {
    catz "$@" | grep '^NBEST sent'
}

getbestfull() {
    catz "$@" | egrep '^NBEST sent=[^ ]* nbest=0'
}
get10best() {
    catz "$@" | egrep '^NBEST sent=[^ ]* nbest=[0-9] '
}

getbest() {
    getbestfull "$@" |  perl -pe 's/ tree={{{[^}]*}}} derivation={{{[^}]*}}}//;s/ inside-cost\=\S+//;s/ +/ /g;'
}

escapetree() {
    perl -pe '
s/(-RRB-) *\)/$1 RRBPAREN/g;
s/(-LRB-) *\(/$2 LRBPAREN/g;
s/([()])/ $1 /g;s/ /  /g;
s/ " / "\\"" /g;
s/ ([^ ()"]+?) / "$1" /g;
s/ +/ /g;s/ +/ /g;s/ +/ /g;
s/ +\)/\)/g; s/\( +/\(/g;
s/"RRBPAREN"/\"\)\"/g;s/"LRBPAREN"/\"\(\"/g;
'  "$@"
}


bleulr() {
    perl -ne '/BLEUr4n4\[\%\] (\S*).*lengthRatio: (\S*)/; ($b,$l)=($1,$2);$_=$ARGV; s|/ibmbleu.out||; print "$_ $b $l\n"' "$@"
}
bleuparse1() {
    if [ -f "$1" ] ; then
        echo -n BLEU:
        bleulr "$1"
    else
        warn "$1 not found (bleu score file)"
    fi

}
bleuparse() {
    forall bleuparse1 "$@"
}

UTIL=${UTIL:-$(echo ~graehl/u)}
[[ -d $UTIL ]] && export PATH=$UTIL:$PATH

true
