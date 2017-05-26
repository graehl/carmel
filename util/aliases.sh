touchconf() {
    touch aclocal.m4 Makefile.in Makefile.am configure
}
bootstrap() {
    ( set -e
      autoreconf -f -i
#      aclocal
#      automake --add-missing
#      autoconf
#      libtoolize -i --recursive
    )
}
UTIL=${UTIL:-$(echo ~/u)}
. $UTIL/add_paths.sh
. $UTIL/bashlib.sh
. $UTIL/time.sh
. $UTIL/misc.sh
set -b
shopt -s checkwinsize
shopt -s cdspell
export VISUAL=emacsclient
export GCCVERSION=6.1.0
xmtc=$(echo ~/c)
xmtx=$(echo ~/x)
racer=$(echo $xmtx)
xmtxs=$xmtx/sdl
GRAEHL_INCLUDE=$xmtxs
xmtextbase=$(echo ~/c/xmt-externals)
xmtexts=$xmtextbase/Shared
xmtextc=$xmtexts/cpp
SDL_SHARED_EXTERNALS_PATH=$xmtexts
graehlmac=mbp
case $(uname) in
    Darwin)
        lwarch=Apple
        ;;
    Linux)
        lwarch=FC12
        shopt -s globstar || true
        ;;
    *)
        lwarch=Windows ;;
esac
xmtext=$xmtextbase/$lwarch
xmtextsrc=$HOME/c/xmt-externals-source
export SDL_EXTERNALS_PATH=$xmtext
export HADOOP_DIR=$SDL_EXTERNALS_PATH/../Shared/java/hadoop-2.7.3
export JAVA_HOME=$SDL_EXTERNALS_PATH/libraries/jdk1.8.0_66
py=$(echo $xmtx/python)
export WORKSPACE=$xmtx
xmtext=$xmtextbase/$lwarch
xmtextsrc=$HOME/c/xmt-externals-source
xmtlib=$xmtext/libraries
libasan=$xmtlib/gcc-6.1.0/lib64/libasan.so
libasanlocal=/local/gcc/lib64/libasan.so
SDL_EXTERNALS_JAVA=$xmtext/libraries/jdk1.8.0_66
export OMP_NUM_THREADS=1
export SDL_EXTERNALS_JAVA
if [[ -d $SDL_EXTERNALS_JAVA ]] ; then
    export JAVA_HOME=$SDL_EXTERNALS_JAVA
fi
if [[ -r $libasanlocal ]] ; then
    libasan=$libasanlocal
fi
fmake() {
    (cd ~/fairseq;./make.sh)
}
ghpull() {
    mine=${1:-origin}
    upstream=${2:-upstream}
    branchmine=${3:-master}
    branchupstream=${4:-master}
    (set -e
     set -x
    git fetch $mine
    git fetch $upstream
    working=merge.$mine.$upstream
    killbranch $working || true
    git checkout -b $working $mine/$branchmine
    git merge $upstream/$branchupstream
    git push $mine HEAD:$branchmine
    )
}
fairclean() {
    find . -name 'state*epoch*th*' -exec rm {} \;
    find . -name 'model*epoch*th*' -exec rm {} \;
    find . -name 'optmodel*th*' -exec rm {} \;
}
giza2() {
    s=$1
    t=$2
    cd `dirname $s`
    s=`basename $s`
    t=`basename $t`
    if [[ -f $s && -f $t ]] ; then
        export PATH=/home/graehl/giza:$PATH

        plain2snt $s $t
        st=${s}_$t.snt
        plain2snt $t $s
        ts=${t}_$s.snt
        if ! [[ $nonorm ]] ; then
            mkcls -p$s -V$s.vcb.classes
            mkcls -p$t -V$t.vcb.classes
            snt2cooc $st.cooc $s.vcb $t.vcb $st
            mgiza -s $s.vcb -t $t.vcb -c $st -CoocurrenceFile $st.cooc &
        fi
        if ! [[ $noinvers ]] ; then
            snt2cooc $ts.cooc $t.vcb $s.vcb $ts
            mgiza -s $t.vcb -t $s.vcb -c $ts -CoocurrenceFile $ts.cooc
        fi
        wait
    else
        echo `pwd`/$s and $t no found
    fi
}
gizaunion() {
    #giza2bal.pl from mosesdecoder
    giza2bal.pl -d $1 -i $2 | symal -alignment=union -diagonal=no -final=no -both=no
}
gizagrowdiagfinal() {
    giza2bal.pl -d $1 -i $2 | symal -alignment=grow -diagonal=yes -final=yes -both=no
}
gizagrowdiagfinaland() {
    giza2bal.pl -d $1 -i $2 | symal -alignment=grow -diagonal=yes -final=yes -both=yes
}
justalign() {
    perl -i~ -pe 's/^.*\Q{##} \E//;'
}
multevalabs() {
    local hyp=$1
    shift
    multevalhome=${multevalhome:-~/multeval}
    (
        cd $multevalhome
        ./multeval.sh eval --refs "$@" --hyps-baseline $hyp --meteor.language ${trglang:-en}
    )
}
multeval() {
    local fs=
    for f in "$@"; do
        fs+=" "$(abspath "$f")
    done
    multevalabs $fs
}
toH() {
    perl -e 'while(<>) {print "H-$.\t0\t$_"}' "$@"
}
toT() {
    perl -e 'while(<>) {print "T-$.\t$_"}' "$@"
}
unbpe() {
    sed 's/@@ //g;s/__LW_SW__ //g;'
}
toHT() {
    toH "$1"
    shift
    for f in "$@"; do
        toT "$f"
    done
}
quitchrome() {
    pkill -a -i "Google Chrome"
    pkill -a -i "Signal"
}
tmuxmax() {
    local a1
    local a2
    local tty1
    local tty2
    # List all clients, ordered by most recent activity descending
    for c in $(tmux list-clients -F "#{client_activity}___#{client_tty}" | sort -n -r); do
        if [ -z "$a1" ]; then
            a1=${c%%___*}
            tty1=${c##*___}
        elif [ -z "$a2" ]; then
            a2=${c%%___*}
            tty2=${c##*___}
        fi
        if [ -n "$a1" ] && [ -n "$a2" ]; then
            if [ "$a1" = "$a2" ]; then
                # Activity timestamps match in top 2 attached clients
                # Let's not detach anyone here!
                tmux display-message "Multiple active attached clients detected, refusing to detach" >/dev/null 2>&1
            elif [ -n "$tty1" ]; then
                # Detach all but the current client, iterating across each
                # Tempting to use detach-client -a -t here, but there's a bug
                # in there, keeping that from working properly
                tmux detach-client -t "$tty2" >/dev/null 2>&1
                a2=
            fi
        fi
    done
}
byobumax() {
    /usr/lib/byobu/include/tmux-detach-all-but-current-client
}
comm() {
    git commit -a -m "$*"
}

mvrm() {
    (set -e
     ! [[ $3 ]]
     rm "$2" || echo2 no previous existing $2
     mv "$1" "$2"
    )
}
cp2ln() {
    (
        set -x
        set -e
        ! [[ $3 ]]
        [[ $2 ]]
        [[ -e $1 ]]
        abs1=`abspath $1`
        to="$2"
        if [[ ${to%/} != $to ]] ; then
            mkdir -p $to
            to="$to$(basename $1)"
        fi
        cp "$1" "$2"
        ln -sf $(relpath "$1" "$2") "$1"
    )
}
mvtoln() {
    to="$1"
    shift
    if [[ -d $to ]] || [[ ${to%/} != $to ]] ; then
        mkdir -p $to
        for f in "$@"; do
            if [[ -e $f ]] ; then
                cp2ln "$f" "$to"
            fi
        done
    elif [[ $2 ]] ; then
        echo2 "error: too many args for a non-directory destination - use trailing / to force creation of dir"
        return 1
    elif [[ -e $to ]] ; then
        echo2 "arg1 to=$to exists and is not a directory, or you didn't add a trailing / to it"
        return 1
    else
        set -x
        cp2ln "$1" "$to"
        set +x
    fi
}

dockrun() {
    sudo nvidia-docker run --rm --volume /:/host --workdir /host$PWD
    --env PYTHONUNBUFFERED=x --env CUDA_CACHE_PATH=/host/tmp/cuda-cache "$@"
}
ntop() {
    while true; do
        nsmi
        sleep 1
    done
}
nsmi() {
    nvidia-smi "$@"
}
gpull() {
    (set -e
     cd ~/g
     git commit -a -m "${*:-pull}" || echo no changes
     git pull --rebase || rebasenext
    )
}
gpush() {
    (set -e
     cd ~/g
     gpull || echo no pull
     git push origin master
    )
}

tensorcpu() {
    # ld doesn't work
    export bazelflags="--config=opt"
    export CC=
    export CXX=
    export bazelflags="--config=opt  --action_env PATH --action_env DYLD_LIBRARY_PATH --action_env LD_LIBRARY_PATH -k"
    cd ~/tensorflow
    [[ -f .tf_configure.bazelrc ]] ||  ./configure
    make clean
    bazel build $bazelflags //tensorflow/tools/pip_package:build_pip_package
    tensorflow/tools/pip_package/build_pip_package.sh /tmp/tensorflow_pkg
    pip install /tmp/tensorflow_pkg/*.whl
}
gitsubup() {
    git submodule update --remote --merge
}
tmuxs() {
    tmux new -s "${tmuxname:-${1:-tmux}}" -n 0 "$@"
}

tensorpip() {
${pip:-pip} install -I --upgrade setuptools
${pip:-pip} install portpicker
${pip:-pip} install mock
${pip:-pip} install pep8
${pip:-pip} install pylint
${pip:-pip} install py-cpuinfo
${pip:-pip} install pandas==0.18.1
${pip:-pip} install wheel
${pip:-pip} install --upgrade six==1.10.0
${pip:-pip} install --upgrade werkzeug==0.11.10
${pip:-pip} install --upgrade protobuf==3.0.0
}
juno() {
    tmuxs jupyter notebook
}
junn() {
    cd ~/nn
    tmuxname=nn tmuxs jupyter notebook
}
tensorcpu() {
    export flags="--config=opt  --action_env PATH --action_env DYLD_LIBRARY_PATH --action_env LD_LIBRARY_PATH -k"
    bazel build $flags //tensorflow/tools/pip_package:build_pip_package
    echo 'echo "startup --max_idle_secs=100000000" > ~/.bazelrc'
}
confpython36() {
    sudo ln -svfn python-3.6.1 /usr/share/doc/python-3
    sudo ln -svfn python-3.6.1 /usr/local/share/doc/python-3
    (set -e
     ./configure --disable-multiarch --enable-optimizations --prefix=/usr/local     --enable-shared   --with-system-expat             --with-system-ffi --with-ensurepip=yes --with-universal-archs=64-bit
     make -j8
     sudo make install
     sudo chmod -v 755 /usr/local/lib/libpython3.6m.so
     sudo chmod -v 755 /usr/local/lib/libpython3.so
    )
}
macopenssl() {
    CFLAGS+=" -I/usr/local/opt/openssl/include"
    LDFLAGS+=" -L/usr/local/opt/openssl/lib"
    export CFLAGS LDFLAGS
}
macmdb() {
    cd ~/src/mdb && make XFLAGS=-I/usr/local/Cellar/berkeley-db/6.2.23/include LDFLAGS+="-L/usr/local/Cellar/berkeley-db/6.2.23/lib"
}
syncmusic() {
    adb stop-server
    (set -e
     set -x
    adb start-server
    adb devices
    cd ~/dropbox
    adb-sync music /sdcard/music
    cd ~/music
    adb-sync local /sdcard/music
    )
}
tarun() {
    local user=${1:-$USER}
    shift
    tarsnap --keyfile "$HOME/$user.tarsnap.key" "$@"
}
tarx() {
    local user=${1?tarx user archive or tarx user-...}
    local a=$2
    if [[ $a ]] ; then
        shift
    else
        a=$1
        user=`perl -e '$_=shift;print "$1" if /([a-z]+)-/;' "$a"`
    fi
    shift
    (
        echo $user $a
    d=~/$user/$a
    mkdir -p $d
    set -x
    cd $d
    tarun $user -x -f "$a" "$@"
    )
}
tarls() {
    tarsnap --keyfile "$HOME/${1:-$USER}.tarsnap.key" --list-archives | sort
}
buildclangformat() {
    (set -x
     set -e
     # choose rev <= http://llvm.org/svn/llvm-project/
     CLANG_REV=295516
     if [[ $force ]] || ! [[ -d llvm ]] ; then
         (
     rm -rf llvm
     mkdir llvm
     svn co http://llvm.org/svn/llvm-project/llvm/trunk@$CLANG_REV llvm
     cd llvm/tools
     svn co http://llvm.org/svn/llvm-project/cfe/trunk@$CLANG_REV clang
     )
     fi
     rm -rf llvm-build
     mkdir llvm-build
     cd llvm-build
     cmake -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release   -DLLVM_ENABLE_ASSERTIONS=NO -DLLVM_ENABLE_THREADS=NO ../llvm/
     make -j8
    )
    local clf=llvm-build/bin/clang-format
    ls -l $clf
     strip $clf
    ls -l $clf
}

tolower() {
    catz "$@" |  tr '[:upper:]' '[:lower:]'
}
fstshow() {
    for f in "$@"; do
        fstdraw --isymbols=${isymbols:-ascii.syms} --osymbols=${osymbols:-wotw.syms} -portrait $f | dot -Tjpg >$f.jpg; open $f.jpg
        done
}
fstcompinv() {
    for f in "$@"; do
        fstcompile --isymbols=${osymbols:-wotw.syms} --osymbols=${isymbols:-ascii.syms} < $f >$f.fst; preview $f
    done
}
fstcomp() {
    fstcompile --keep_isymbols --keep_osymbols "$@"
}
fstprinto() {
    fstprint --isymbols=${osymbols:-wotw.syms} --osymbols=${osymbols:-wotw.syms} "$@"
}
gitchangedls() {
    git diff-tree --no-commit-id --name-only -r ${1:-HEAD}
}
xmtnull=' -o /dev/null --log-config=/home/graehl/warn.xml'
timenmtchs() {
    for k in -1 10 0; do
    for f in "$@"; do
        khyps=$k xmt=~/pub/$f/xmt timenmtch
    done
    done
}
timenmtch() {
    local x=${xmt:-xmtRelease}
    local k=${khyps:-10}
    echo -n "k-hyps=$khyps $x :"
    local root=/home/graehl/bugs/xnn
    time $x --input=$root/input/nmt-char-sequence --pipeline=nmt-beam-decoder --nmt-beam-decoder.k-hyps-per-state=$khyps --config=$root/config/nmt-beam-decoder.yml -o /dev/null --log-config=/home/graehl/warn.xml
}
dups() {
    md5sum "$@" | sort | perl -e 'while(<>){$m=substr($_,0,32);$f=substr($_,34);if ($m eq $m0) { print STDERR $f0 if $f0; print $f; $f0 = undef ; } else {$f0 = $f;} $m0=$m;}'
}
pie() {
    local e=$1
    shift
    perl -i~ -pe "$e" "$@"
}
pies() {
    if [[ $2 ]] ; then
        (
        set -x
        perl -i~ -e '$from=shift;$to=shift; while(<>){s/\Q$from\E/$to/og; print}' "$@"
        shift
        if [[ $2 ]] ; then
            ag --literal "$@"
        fi
        )
    fi
}
renameprefix() {
    if [[ $1 ]] && [[ $2 ]] ; then
        local t=`mktemp`
        for f in `ls -d $1*`; do g=${f#$1}; echo mv $f $2$g; done > $t
        cat $t
        echo "move $1 => $2 ?"
        select yn in "Yes" "No"; do
            if [[ $yn == Yes ]] ; then
                (set -x
                 . $t
                )
                ls -d $2*
                echo "moved $1 => $2"
            fi
            break
        done
        rm $t
    fi
}
mendrescorelen() {
    mend;co rescorelen;. ~/tmp/co.sh;mend
}
ctpath() {
    export PYTHONPATH=/.auto/home/graehl/c/coretraining/main/3rdParty/python
    export LD_LIBRARY_PATH=/.auto/home/graehl/c/coretraining/main/lib:/.auto/home/graehl/c/coretraining/main/lib
    export PERL5LIB=/.auto/home/graehl/c/coretraining/main/kraken/xnmtrescore/App:/.auto/home/graehl/c/coretraining/main/kraken/xnmtrescore/App/bin:/.auto/home/graehl/c/coretraining/main/Shared/App:/.auto/home/graehl/c/coretraining/main/Shared/PerlLib:/.auto/home/graehl/c/coretraining/main/3rdParty/perl_libs:/.auto/home/graehl/c/coretraining/main/Shared/PerlLib/TroyPerlLib:/.auto/home/graehl/c/coretraining/main/kraken/xtrain/App:/.auto/home/graehl/c/coretraining/main/kraken/xtrain/App/bin:/.auto/home/graehl/c/coretraining/main/Shared/App
}
renamepre() {
    if [[ $1 ]] ; then
    if [[ $2 ]] ; then
        for f in $1*; do
            suf=${f#$1}
            mv $f $2$suf
        done
        for f in $(ag -l --literal "$1" $2.apex); do
            perl -i~ -pe "s#\Q$1\E#$2#g" $f
        done
        ls $2*
    fi
    fi
}
prewpm() {
    wpm `lstd $1*`
}
wpmbleu() {
    perl -e 'while(<>) {$wpm=$1 if m#Words/Minute: (?:\[[^/]*/)([^/]+)#;$lcbleu=$1 if /lcBLEU:Avg = (.+)/;$bleu=$1 if /txt:Avg = (.+)/;} $lcbleu=sprintf("%.2f", $lcbleu); $bleu=sprintf("%.2f", $bleu); $wpm=sprintf("%.1f", $wpm); print "($lcbleu) $bleu (lc)BLEU @ $wpm wpm\n"' "$@"
}
aglines() {
    ag --nogroup --nofilename "$@"
}
lstd() {
    for f in `ls -rtd "$@"`; do
        if [[ -d "$f" ]] ; then
            echo "$f"
        fi
    done
}
lst() {
    lsd -rt "$@"
}
sumperf() {
    (aglines 'Words/Minute|MaxVM' "$@" | $UTIL/summarize_num.pl)     2>/dev/null
}
wpm1() {
    (
        local d=$1
        if [[ $d ]] ; then
            d=${d%/}
            d=${d%/evalresults_avg}
            if [[ -d $d/evalresults_avg ]]  ; then
                (
                    cd "$d"
                    b=`basename $d`
                    printf '%36s \t' "${b%.re}"
                    ((cd eval_slot;aglines 'Words/Minute|MaxVM' | $UTIL/summarize_num.pl)  2>/dev/null
                     grep Avg evalresults_avg/test-results.txt evalresults_avg/test-results.txt.lcBLEU
                     ) | wpmbleu
                )
            elif [[ -d $d ]] ; then
                echo2 "  no evalresults for $d"
            fi
            cd "$d" || exit
        fi
        if [[ $verbose ]] ; then
            for f in evalresults*; do
                (ag BLEU $f)
            done
        fi
    )
}
wpm() {
    forall0 wpm1 "$@"
}

rea() {
    . ~/u/aliases.sh
}
reapex() {
    resherp=1 rmapex "$@"
}
rmapex() {
    local apex=${1%.apex}.apex
    if [[ -f $apex ]] ; then
        local stem=${apex%.apex}
        if [[ $stem ]] ; then
            mv $apex tmp.apex
            echo rm -rf $stem*
            select yn in "Yes" "No"; do
                if [[ $yn == Yes ]] ; then
                    rm -rf bak.$stem
                    mkdir bak.$stem
                    mv $stem* bak.$stem
                else
                    return
                fi
                break
            done
            mv tmp.apex $apex
            if [[ $resherp ]] ; then
                sherp $apex
            fi
        fi
    fi
}
kraken() {
    local dir=$1
    shift
    chost=c-dmunteanu2 c-s "cd $dir; LANG=;UNSUPPORTED=;PYTHONPATH=/.auto/home/graehl/c/coretraining/main/3rdParty/python;LD_LIBRARY_PATH=/.auto/home/graehl/c/coretraining/main/lib:/.auto/home/graehl/c/coretraining/main/lib;PATH=/.auto/home/graehl/c/coretraining/main/kraken/xnmtrescore/App:/.auto/home/graehl/c/coretraining/main/kraken/xnmtrescore/App/bin:/.auto/home/graehl/c/coretraining/main/Shared/App:/.auto/home/graehl/c/coretraining/main/kraken/xtrain/App:/.auto/home/graehl/c/coretraining/main/kraken/xtrain/App/bin:/.auto/home/graehl/c/coretraining/main/Shared/App:/home/hadoop/jdk1.8.0_60/bin:/home/graehl/c/xmt-externals/FC12/libraries/hadoop-0.20.2-cdh3u3/jdk1.8.0_60/bin:/home/graehl/c/xmt-externals/FC12/../Shared/java/apache-maven-3.0.4/bin:/home/graehl/c/xmt-externals/FC12/libraries/jdk1.8.0_66/bin:/home/graehl/u:/home/graehl/bin:/usr/local/bin:/usr/bin:/usr/lib64/qt-3.3/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin:.:/local/bin/usr/X11R6/bin:.;PERL5LIB=/.auto/home/graehl/c/coretraining/main/kraken/xnmtrescore/App:/.auto/home/graehl/c/coretraining/main/kraken/xnmtrescore/App/bin:/.auto/home/graehl/c/coretraining/main/Shared/App:/.auto/home/graehl/c/coretraining/main/Shared/PerlLib:/.auto/home/graehl/c/coretraining/main/3rdParty/perl_libs:/.auto/home/graehl/c/coretraining/main/Shared/PerlLib/TroyPerlLib:/.auto/home/graehl/c/coretraining/main/kraken/xtrain/App:/.auto/home/graehl/c/coretraining/main/kraken/xtrain/App/bin:/.auto/home/graehl/c/coretraining/main/Shared/App /usr/bin/perl $*"
}
nnexpanded() {
    echo -n "$1 - "; grep nn-expanded "$1" | summarize_num.pl 2>/dev/null
}
nmtsum() {
    (
        cd ~/p/nmt/
        rm  *.derivs *.perf
        for f in *.log; do
            echo $f
            g=${f%.log}
            d=$g.derivs
            fgrep 'INFO  sdl.xmt.Derivation - ' $f > $d || rm $d
            p=$g.perf
            fgrep ' - Total: processed' $f | egrep 'rescore|decode|segment' > $p
            fgrep 'avg: {rescore-lattice:' $f >> $p
            fgrep 'nn-expanded' $f | $UTIL/summarize_num.pl 2>/dev/null >> $p
        done
        for f in `ag -l nn-expanded`; do nnexpanded "$f"; done
    )
}
untr() {
    unhex "$@" |     perl -pe 's| nn-scored=\d+||;
s|prefix=\S+|prev=.'
}
unhex() {
    perl -pe 's|\b0x[0-9A-Fa-f]+|.|g;s| nn-scored=\d+||' "$@"
}
envtr() {
    branch=${1:-nmtbeam}
    bin=tmp/TestRescoreLattice.$branch
}
runtr() {
    (
    envtr $1
    filter=untr out=~/bugs/nmt/test.$branch.log c12 ./$bin
    )
}
cptr() {
    (
        envtr $1
    c-s cp x/Debug/Hypergraph/TestRescoreLattice $bin
    runtr $branch
    )
}
gitresetco() {
    local rev=$1
    shift
    (set -e
     git ls-files $rev -- "$@"
    bakthis pre.reset.`basename $rev`
    git reset $rev -- "$@"
    git checkout $rev -- "$@"
    )
}
testpipes() {
    local tmppipes=${tmppipes:-/var/tmp/testpipes}
    c-s stopyarn
    c-s rm -rf $tmppipes
    BUILD_TYPE_UC=${BUILD:-Release} c-s ~/x/RegressionTests/HadoopPipes/scripts/pipes.runner.bash $tmppipes $1
}
stopyarn() {
    local HADOOP_DIR=$SDL_EXTERNALS_PATH/../Shared/java/hadoop-2.7.3
    local log=/tmp/stop-yarn.out
    $HADOOP_DIR/sbin/stop-yarn.sh $log
    tail $log
    pgrepkill java
}
jremert() {
    j-s cp x/Release/mert/mert2 c/ct/main/LW/CME/mert2
    j-s 'cd c/ct; mend'
}
crontest() {
    ( set -e; set -x; name="mert-update2"; export LW_SHERPADIR=rename; export LW_RUNONEMACHINE=1; export CT=$HOME/c/ct; ( cd $CT/main; set -x; nohup perl Shared/Test/bin/CronTest.pl $CT -name "$name" -steps tests -tests kraken -parallel 10 ) &>crontest.$name.log; echo `pwd`/crontest.$name.log ) &
    echo "https://condor-web.languageweaver.com/CronTest_graehl/www/index.cgi?"
}
reflog() {
    git reflog --format='%C(auto)%h %<|(17)%gd %C(blue)%ci%C(reset) %s' | head -${1:-99}
}
safefilename() {
    perl -e 'for (@ARGV) { y/:/_/ }; print join " ", @ARGV' "$@"
}
cutmp4() {
    (set -x
     ffmpeg -i "$1" -ss $2 -to $3 -async 1  -c copy "${1%.mp4}.$(safefilename $2-$3).mp4"
     )
}
nbest() {
    hyp best -a feature -n "$@"
}
determinize() {
    hyp determinize   --log-config=/home/graehl/debug.xml -a feature "$@"
}
nbeststr() {
    hyp best -a feature --weight=false --quote=unquoted --nbest-index=0 "$@"
}
chomplast() {
    perl -e '
while(<>) {
    print $p if ($p);
    $p = $_;
}
chomp $p;
print $p;' "$1" > "$1.chomped" && mv "$1.chomped" "$1"
}
xmtsize() {
    du -h ~/x/Debug/xmt/lib/libxmt_shared_debug.so ~/x/Debug/xmt/xmt
}
gitchanged2() {
    git log --name-status --oneline "$1..$2"
}
cmakebuild() {
    cmake --build .
}
cmakeinstall() {
    cmake -DCMAKE_INSTALL_PREFIX=${PREFIX:-/usr/local} -P cmake_install.cmake
}
linelengths() {
    perl -lne '$c{length($_)}++ }{ print qq($_ $c{$_}) for (keys %c);' "$@" | sort -n
    echo "(LENGTH COUNT)"

}
killbranchr() {
    (set -x
     git push origin --delete "$1"
     )
}
#export SDL_LD_PRELOAD=$libasan
xmtlibshared=$xmtextbase/Shared/cpp
grepu() {
    ag --nonumbers "$@" | sort | uniq -c
}
for SDL_BOOST_MINOR in 60 59 58; do
    #BOOST_INCLUDEDIR=$SDL_SHARED_EXTERNALS_PATH/cpp/boost_1_${SDL_BOOST_MINOR}_0/include
    #BOOST_LIBDIR=$SDL_EXTERNALS_PATH/libraries/boost_1_${SDL_BOOST_MINOR}_0/lib
    if [[ -d $BOOST_LIBDIR ]] ; then
        break
    fi
done
CT=${CT:-`echo ~/c/ct/main`}
CTPERL=$CT/Shared/Test/bin/CronTest/www/perl
CTPERLLIB="-I $CT/main/Shared/PerlLib/TroyPerlLib -I $CT/main/Shared/PerlLib -I $CT/main/Shared/PerlLib/TroyPerlLib/5.10.0 -I $CT/main/3rdParty/perl_libs"
[[ -x $CTPERL ]] || CTPERL=perl
export LESS='-d-e-F-X-R'
hypdir=sdl
ostarball=/tmp/hyp-latest-release-hyp.tar.gz
osgitdir=$(echo ~/c/hyp)
osdirbuild=/local/graehl/build-hypergraphs
chosts="j c-graehl gitbuild1 gitbuild2"

chost=c-graehl
jhost=j
xmt_global_cmake_args="-DSDL_PHRASERULE_TARGET_DEPENDENCIES=1 -DSDL_BLM_MODEL=1 -DHADOOP_YARN=1"
jemalloclib=$xmtlib/jemalloc/lib/libjemalloc.so
tbbmalloclib=$xmtlib/tbb-4.4.0/lib/libtbbmalloc.so
tbbmallocproxylib=$xmtlib/tbb-4.4.0/lib/libtbbmalloc_proxy.so
tbbplay() {
    LD_PRELOAD=$tbbmallocproxylib:$tbbmalloclib "$@"
}
jeprof() {
    MALLOC_CONF="prof:true,lg_prof_sample:1,prof_accum:false,prof_prefix:jeprof.out" LD_PRELOAD=$jemalloclib "$@"
}
jeplay() {
    MALLOC_CONF="prof:true,prof_accum:false,prof_prefix:jeprof.out" LD_PRELOAD=$jemalloclib "$@"
}
jestat() {
    MALLOC_CONF="stats_print:true" LD_PRELOAD=$jemalloclib "$@"
}
mendco() {
    git checkout "$@"
    git commit -C HEAD --amend
}
mendcop() {
    mendco perfrace "$1"
}
gitnrev() {
    git show-refs "${1:-HEAD}" --count
}
lastline() {
    tail -1 "$@"
}
lastlineis() {
    local is=$1
    shift
    for f in "$@"; do
        local l=`lastline "$f"`
        if [[ $is = $l ]] ; then
            echo $f
        fi
    done
}
rmsub() {
    perl -e '$_=shift;s/'"$1"'//;print' "$2"
}
mvrmsub() {
    ag -g "$1" |
    while read f; do
        g=$(rmsub "$1" "$f")
        mv "$f" "$g"
    done
}
smallest() {
    ls -rS "$@" | head -1
}
rmsmallest() {
    local h=$1
    local g=$2
    local smallestdir=${smallestdir:-$TMPDIR/smallest}
    if [[ -f "$h" ]] && [[ -f "$g" ]] ; then
        h=$(smallest "$h" "$g")
        if [[ -d $smallestdir ]] ; then
            mv $h $smallestdir/
        fi
    fi
}
rmsmallvariant() {
    t="trash$1"
    mkdir -p "$t"
    ag -g "$1" |
    while read f; do
        rmsmallest "$f" $(rmsub "$1" "$f")
    done
}
rmvariant() {
    t="trash$1"
    mkdir -p "$t"
    ag -g "$1" |
    while read f; do
        g=$(rmsub "$1" "$f")
        if [[ -f "$g" ]] ; then
            mv "$f" "$t/"
        fi
    done
}
githeadref() {
    git show-ref -s HEAD
}
gitrmhistory() {
    (set -e
     set -x
    [[ -d .git ]]
    #githeadref > .git/shallow
    find .git/refs -type f | xargs cat | grep -v 'ref:' | sort -u > .git/shallow
    git describe --always
    git reflog expire --expire=now --all
    git reflog expire --expire=0
    git prune
    git prune-packed
    )
}
cedit() {
    local f=~/tmp/`basename $1`
    mkdir -p `dirname $f`
    echo $f
    scp "c-graehl:$1" $f
    edit $f
}
wintop() {
    # https://www.microsoft.com/resources/documentation/windows/xp/all/proddocs/en-us/tasklist.mspx?mfr=true
    tasklist
}
dumpsym() {
    ssh ${whost:-wgitbuild7} '/cygdrive/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/amd64/dumpbin.exe' /symbols "$@"
}
gitpush() {
    git push origin `gitbranch`
    git fetch origin
    gitlog
}
rdmd() {
    rdm -y 200 -u -j 4 2>&1 | tee ~/tmp/rdm.log
}
bentindex() {
    rc -C
    rc -J ../bcent
}
makertags() {
    (set -e
     cmake . -DCMAKE_EXPORT_COMPILE_COMMANDS=1
     rc -J .
    )
}
useclanglibc() {
    export CC="clang-3.8"
    export CXX="clang++-3.8 -stdlib=libc++"
    export CXXFLAGS="$CXXFLAGS -nostdinc++ -I/usr/local/opt/llvm36/lib/llvm-3.8/include/c++/v1"
    export LDFLAGS="$LDFLAGS -L/usr/local/opt/llvm36/lib/llvm-3.8/lib"
}
gitignorerm() {
    git ls-files --deleted -z | git update-index --assume-unchanged -z --stdin
}
gitdiffl() {
    git diff --stat "$@" | cut -d' ' -f2
}
linelens() {
    if [[ $words ]] ; then
        awk '{print NF}' "$@"
    else
        awk '{print length()}' "$@"
    fi
}
longlines() {
    perl -e '$n=shift;while(<>) { print if scalar $_ > $n }' "$@"
}
lenhist() {
    # histogram of line lengths
    linelens "$@" | sort -n | uniq -c
}
linehist() {
    lenhist "$@"
}
bent() {
    (
        [[ $vg ]] && vgcmd=vg
        vgscript=script.txt
        #[[ $vg ]] && vgscript=valgrind.txt
        cd ~/ent/docs
        [[ $redox ]] && ./make.sh
        cd ~
        [[ $clean ]] && rm -rf bent/
        mkdir -p bent
        set -e
        cd bent
        gccsuf=${gccsuf:--6}
        #gccsuf=
        #rm -rf entcore
        CC=gcc$gccsuf CXX=g++$gccsuf BUILD_TYPE=Debug cmakemac ../ent -DGCC_ASAN=1 "$@"
        set -x
        [[ $notest ]]  || make test
            \cd entscripttests && echo | $vgcmd ./entscripttests $vgscript
    )
}
scanbent() {
    (
        set -e
        cd ~
        mkdir -p scanbent
        cd ~/scanbent
        [[ $noclean ]] || rm -rf ent*
        clangsuf= SCAN=1 cmake38 ../ent 2>&1 | tee ~/tmp/scanbent.log
    )
}
bcent() {
    (set -e
     cd ~/bcent
     #rm -rf entcore
     cmake38 ../ent -DCMAKE_EXPORT_COMPILE_COMMANDS=1
     [[ $notest ]]  || cd entscripttests && echo | ./entscripttests
    )
}
cmake38() {
    clangsuf=-3.8
    CC=clang$clangsuf CXX=clang++$clangsuf BUILD_TYPE=Debug cmakemac "$@"
}
cmakemac() {
    (
        #export CMAKE_OSX_SYSROOT=
        #/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/
        #[[ $noclean ]] || rm -f CMakeCache.txt
        nosysroot="-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING= -DCMAKE_OSX_SYSROOT:STRING=/ -DCMAKE_MACOSX_RPATH=0 -DCMAKE_FIND_FRAMEWORK=NEVER -DCMAKE_FIND_APPBUNDLE=NEVER"
        #nosysroot=
        scanbuild=
        if [[ $SCAN ]] ; then
            scanbuild=scan-build
            echo $scanbuild
        fi
        #-DCMAKE_EXPORT_COMPILE_COMMANDS=1
        cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 -DCMAKE_C_COMPILER=${CC:-gcc$gccsuf} -DCMAKE_CXX_COMPILER=${CXX:-g++$gccsuf} $nosysroot "$@" && TERM=dumb $scanbuild make VERBOSE=1 -j ${NCPUS:-3}
    )
}

usescanbuild() {
    useclang
    scanbuild=1
    export PATH=/usr/libexec/clang-analyzer/scan-build/:$PATH
    export CCC_CC=clang
    export CCC_CXX=clang++
    export CC="ccc-analyzer"
    export CXX="c++-analyzer"
}
gsldiff() {
    (
        set -e
        sub=GSL/include
        cd ~/src/$sub
        for f in *.h; do
            diff $f $xmtextc/$sub/$f || echo $f
        done
    )
}
overgsl() {
    sub=GSL/include
    cd ~/src/$sub
    (gsldiff
     for f in *.h; do
         cp $f $xmtextc/$sub/$f
     done
    )
}
lingsl() {
    sub=GSL/include
    cd ~/src/$sub
    (gsldiff
     for f in *.h; do
         cp $f $xmtextc/$sub/$f
         scp $f $chost:c/xmt-externals/Shared/cpp/GSL/include
     done
    )
}
gcjen() {
    (cd;
     lingsl
     cjen "$@"
    )
}
substkraken() {
    (set -e
     cd ~/c/ct/main/kraken/Test/TestCases/kraken
     substi "$@" `find . -name my.params`
    )
}
ins() {
    (set -e
     local t=${1?in [1] sec, [2...]}
     shift
     echo2 "in $t sec, $*"
     sleep $t
     echo2 "$ $*"
     "$@"
    )
}
ssub() {
    ssh c-science
    #ssh c-dmuntuneau2
}
nograehl() {
    for f in *.hpp; do grep -q 'Jonathan Graehl' $f || echo $f; done
}

gitq() {
    git status -u
    #gitlog 3
}
lh() {
    ls -1sh "$@"
}
mcallgrind() {
    c-s time callgrind /home/graehl/x/Release/xmt/xmt --input=/home/graehl/tmp/rep.txt --pipeline=1best --config=/local/graehl/time/config/XMTConfig.yml --log-config=/home/graehl/warn.xml
}
findtex() {
    tlmgr search --global --file "$@"
}
installclang() {
    cd /local/graehl/src
    git clone https://github.com/rsmmr/install-clang
    cd install-clang
    ./install-clang -b Release -m -j 14 /local/clang
}
filtermsvc() {
    egrep -v 'C4251|C4267|MSB8028|C4018|C4244|C4305|: note: |C4005' "$@"
}
boostcflags() {
    local bm=59
    local bd=boost_1_${bm}_0
    CXXFLAGS="-I $SDL_SHARED_EXTERNALS_PATH/cpp/$bd/include"
    CFLAGS=$CXXFLAGS
    LDFLAGS="-L $xmtext/libraries/boost/$bd/lib"
    export CXXFLAGS
    export CFLAGS
    export LDFLAGS
}
outsidegitadd() {
    (cd `dirname $1`
     git add `basename $1`)
}
reken() {
    (set -e
     clean=1 linnplm
     clean=1 linnplm01
     linken
    )
}
lnboost() {
    suf=gcc-mt-1_59.so.59.0; for f in *$suf; do ln -sf $f ${f%-$suf}.so; done
}
mvcue() {
    (set -e
     for f in split-track*.flac; do
         ARTIST=`metaflac "$f" --show-tag=ARTIST | sed s/.*=//g`
         TITLE=`metaflac "$f" --show-tag=TITLE | sed s/.*=//g`
         TRACKNUMBER=`metaflac "$f" --show-tag=TRACKNUMBER | sed s/.*=//g`
         mv "$f" "$ARTIST - `printf %02g $TRACKNUMBER` - $TITLE.flac"
     done
    )
}
flaccue() {
    ( set -e;
      flac=$1
      test -f $flac
      cue=${2:-$flac.cue}
      cuebreakpoints $cue | shnsplit -o flac $flac
      cuetag $cue split-track*.flac
      mvcue
    )
}
ccachec() {
    for nhost in gitbuild3 gitbuild5 gitbuild6 git02; do
        nhost=$nhost n-s ccache -C
    done
}
gitundosoft() {
    git reset --soft HEAD@{1}
}
du1() {
    du -hsc "$@"
}
ctpush() {
    scpgr c/coretraining/main/kraken/xtune/App --exclude=bin --exclude=legacybin
    c-s 'cd ~/c/coretraining && mend'
}
gitff() {
    git pull --ff-only
}
emacsdbg() {
    open -a /Applications/Emacs.app --args --debug-init
}
update_terminal_cwd() {
    # Identify the directory using a "file:" scheme URL,
    # including the host name to disambiguate local vs.
    # remote connections. Percent-escape spaces.
    local SEARCH=' '
    local REPLACE='%20'
    local PWD_URL="file://$HOSTNAME${PWD//$SEARCH/$REPLACE}"
    if [[ -z $INSIDE_EMACS ]] ; then
        printf '\e]7;%s\a' "$PWD_URL"
    fi
}
sortspeed() {
    local f=$1
    shift
    perl -ne 'print if /^[0-9_]+(\S+[0-9.]+)+/' "$f" | tr _ ' ' | sort -g "$@"
}
pandocslides() {
    (
        set -e
        local f=$1
        base=${f%.md}
        [[ -f $f ]] || f=$base.md
        local out=${2:-$base-slides.html}
        rm -f $out
        ln -sf ~/c/reveal.js `dirname $out`/
        pandoc -s -t revealjs --slide-level=2 $f -o $out -V ${2:-black} && perl -i -pe 's{theme/simple}{theme/black}' $out &&
            browse $out
    )
}
panhtml() {
    (
        set -e
        local f=$1
        base=${f%.md}
        [[ -f $f ]] || f=$base.md
        local out=${2:-$base.html}
        rm -f $out
        ln -sf ~/c/reveal.js `dirname $out`/
        pandoc -s -t html5 --slide-level=2 $f -o $out -V ${2:-black} && perl -i -pe 's{theme/simple}{theme/black}' $out &&
            browse $out
    )
}

dfpercent() {
    df -h "$@" | perl -ne '$p=$1 if /(\S+)%\s+\S*$/;END{print 100-$p}'
}
dfrequire() {
    local require=${2:-5}
    local df=`dfpercent "$1"`
    if [[ $df -lt $require ]] ; then
        echo "$1 had only $df% free and we require $require%" 1>&2
        return 1
    fi
}
speedboth() {
    (set -e
     local a=$1
     local b=$2
     shift
     shift
     local A=$a
     local B=$b
     local fa=$a/speed/report.txt
     local fb=$b/speed/report.txt
     [[ -f $fa ]] || fa=$a/report.txt
     [[ -f $fb ]] || fb=$b/report.txt
     [[ -f $fa ]] || fa=$a
     [[ -f $fb ]] || fb=$b
     local using=${using:-3:2:1}
     if [[ $sync ]] ; then
         using=0:3
     fi
     local labels="with labels point pt 5 offset 2"
     labels=
     cat<<EOF>"$a-$b.cmd"
set terminal png size 1280,960 enhanced
set output "$a-$b.png"
set title "$A vs. $B speed/quality"
set xlabel "BLEU4"
set ylabel "Speed (wpm)"
set pointsize 1
set offset 1,1,1,1
plot "$fa" u $using $labels title "$A", "$fb" u $using $labels title "$B"
EOF
     rm -f "$a-$b.png"
     gnuplot -c "$a-$b.cmd"
     open "$a-$b.png"
    )
}

cpumodel() {
    ssh $1 'grep -m1 -A3 "vendor_id" /proc/cpuinfo'
}
cspeed() {
    for g in "$@"; do
        (
            cd ~/p/qf
            [[ "$rerun" ]]  && chost=git02 c-s "cd ~/p/qf; perl ~/c/coretraining/main/quality_finder/App/yas_speed_test.pl --param-file=$g/$g.params"
            set -e
            mkdir -p $g
            cd $g
            for f in plot.gif report.txt speed_bleu.xml; do
                scp $chost:p/qf/$g/speed/$f . || true
            done
        )
    done
}
cspeedboth() {
    (set -e
     cd ~/p/qf
     d=$3$1-$2
     mkdir -p $d
     cd $d
     na=$1
     nb=$2
     a=$3$1
     b=$3$2
     ln -sf ../$a $a
     ln -sf ../$b $b
     cspeed $a
     cspeed $b
     shift
     shift
     speedboth $a $b "$@"
    )
}

emacspixel() {
    GDK_USE_XFT=0 emacs "$@"
}
cpas() {
    cp -as `abspath "$@"` .
}
cpRs() {
    cp -Rs `abspath "$@"` .
}
gitxz() {
    gitroot
    name=$(basename `pwd`)
    (set -e;
     #branch=${branch:-`gitbranch`}
     git archive --format=tar --prefix=$name/ HEAD "$@" | xz -c >$name.tar.xz
    )
    ls -l `realpath $name.tar.xz`
}
targit() {
    gitroot
    name=$(basename `pwd`)
    (set -e;
     cd ..
     tar c --exclude=.git --exclude=.gitignore $name | xz -c >$name.tar.xz
     mv $name.tar.xz $name/$name.tar.xz
    )
    ls -l `realpath $name.tar.xz`
}
capturecore() {
    ulimit -c unlimited
    "$@" || gdb "$1" *core*
}
gitshowmsg() {
    git log "$@" -n 1
}
gdbrepeat() {
    gdb -ex "b exit" -ex "commands" -ex "run" -ex "end" -ex "run" --args "$@"
}
gitdeletedrev() {
    git log --all -- "$@"
}
gitbranch() {
    git rev-parse --abbrev-ref HEAD
}
gitshowrev() {
    local rev=$1
    shift
    git show $rev -- "$@"
}
gitshow() {
    local rev=$1
    shift
    local f=$1
    if [[ -f $f ]] ; then
        if ! [[ -d .git ]] ; then
            f="./$f"
        fi
    fi
    git show $rev:"$f"
}
gitdeleted() {
    git log --diff-filter=D --summary | grep delete
}
gitroot() {
    cd "$(git rev-parse --show-toplevel)"
}
gittheirs() {
    (set -e
     gitroot
     git co --theirs "$@"
     git add "$@"
     git rebase --continue
    )
}
testn() {
    (set -e;
     logdir=${logdir:-~/tmp/}
     mkdir -p $logdir
     n=$1
     shift
     for i in `seq 1 $n`; do
         if "$@" 2>$logdir/testn.$i.err >$logdir/testn.$i.out; then
             echo2 $i ok
         else
             echo2 "$* - failure on try #$i of $n"
             exit 1
         fi
     done
    )
}
tosdlext2() {
    sdlextbase=$(echo ~/c/sdl-externals)
    xmtextbase=$(echo ~/c/xmt-externals)
    require_dirs $sdlextbase $xmtextbase
    local src=$xmtextbase/$1
    local dst=$sdlextbase/$1
    cp -a $src $dst && (cd $sdlextbase && git add $1)
}
cygsshd() {
    ssh-host-config -y
    #prompt CYGWIN=: tty ntsec
    cygrunsrv -S sshd
}
xclean() {
    rm -rf ~/x/Debug ~/x/Release
    mkdir -p ~/x/Debug ~/x/Release
    ccache -C
}
linnplm01() {
    scpx ~/src/kpu/nplm/src $jhost:xs/nplm01/; j-s "cd ~/xs/nplm01;clean=$clean ./make.sh"
}
syncnplm() {
    scpx ~/src/nplm/src $jhost:xs/nplm/
}
linnplm() {
    syncnplm; j-s "cd ~/xs/nplm;clean=$clean ./make.sh"
}
syncken() {
    scpx ~/src/KenLM $jhost:xs/
}
linken() {
    syncken; j-s 'cd ~/xs/KenLM;./make-kenlm.sh'
}
scpg() {
    (
     cd
     o=`dirname $1`
     c-s `mkdir -p $o`
     for f in `ls "$1"`; do
         if [[ -f $f ]] ; then
             echo "$f => :$o"
             scp "$f" c-graehl:"$o/"
         fi
     done
    )
}
scpgr() {
    (
     cd
     local d=$1
     shift
     scpx "$d" "c-graehl:`dirname $d`" "$@"
    )
}
scpx() {
    if [[ -d $1 ]] || [[ $reverse ]]; then
        (
            set -e
            local from=$1
            #cd $from
            shift
            local to=$1
            shift
            if [[ $reverse ]] ; then
                local a=$from
                local base=`basename $from`
                from=$to/$base
                to=`dirname $a`
                mkdir -p $to
            fi
            rsync --modify-window=1 --cvs-exclude --exclude=.git -K -a "$@" $from $to
        )
    fi
}
gitundosoft() {
    git reset --soft HEAD~1
}
difflines() {
    diff -C 0 "$@" | grep '^[-+] '
    echo2 "$@"
}
hymac() {
    l /local/graehl/build-hypergraphs/Hypergraph/hyp
    cp /local/graehl/build-hypergraphs/Hypergraph/hyp ~/pub/hy/hy
    hy -h
}
reosmac() {
    (
        f=sdl/CMakeLists.*txt
        cp $xmtx/$f ~/c/hyp/$f
        cd ~/c/hyp
        scripts_dir=$xmtx/scripts
        translatesrc=$scripts_dir/release-translate-source.py
        $scripts_dir/filter-cmake-for-release.py -i $f
        $translatesrc -i $f
        mend
        uselocalgccmac
        osmake
        hymac
    )
}
macgcc() {
    uselocalgccmac
}
democompose() {
    (
        hy compose --project-output=false --in /local/graehl/xmt/RegressionTests/Hypergraph2/compose3a.hgtxt /local/graehl/xmt/RegressionTests/Hypergraph2/compose3b.hgtxt --log-level=warn
    )
}
brewdeps() {
    brew list | while read cask; do echo -ne '\x1b[1;34m'"$cask"' ->\x1b[0m'; brew deps $cask | awk '{printf(" %s ", $0)}'; echo ""; done
}
brewdependent() {
    brew uses --installed "$@"
}
checklibc() {
    otool -L "$@"
}
applehyp() {
    gcc5= gcc49= gcc47= NOLOCALGCC=1 UPDATE=0 CC=gcc CXX=g++ GCC_SUFFIX= machyp
}
retrans() {
    btrans=~/bugs/trans
    mkdir $btrans
    mv ~/Library/Preferences/org.m0k.transmission.LSSharedFileList.plist $btrans
    mv ~/Library/Preferences/org.m0k.transmission.plist $btrans
    mv ~/Library/Caches/org.m0k.transmission $btrans
}
brewboost() {
    brew install boost --c++11 --with-gcc=gcc-5
}
makeboost() {
    (
        gccprefix=${gccprefix:-/local/gcc}
        if [[ $NOLOCALGCC = 1 ]] ; then
            gccprefix=
        fi
        if [[ -d $gccprefix ]] ; then
            #CMAKE_AR=$gccprefix/gcc-ar
            #CMAKE_RANLIB=$gccprefix/gcc-ranlib
            #CMAKE_NM=$gccprefix/gcc-nm
            export PATH=$gccprefix/bin:/usr/local/bin:$PATH
            LD_RUN_PATH+=":$gccprefix/lib64"
            export LD_RUN_PATH=${LD_RUN_PATH#:}
            LD_LIBRARY_PATH="$gccprefix/lib64:$LD_LIBRARY_PATH"
            export LD_LIBRARY_PATH=${LD_LIBRARY_PATH#:}
        fi
        set -e
        cd $1
        xmtext=$2
        icusubdir=${3:-icu-55.1}
        [[ -d $xmtext ]] || return 1
        icu=$xmtext/libraries/$icusubdir
        withicu=--with-icu=$icu
        toolset=gcc
        if [[ $lwarch = Apple ]] ; then
            withicu=--with-icu=/usr/local/Cellar/icu4c/55.1
            toolset=gcc5
        fi
        if [[ $clean ]] ; then
            if [[ -x ./b2 ]] ; then ./b2 clean; fi
            rm ./b2
            rm -rf bin.v2
            rm -rf stage
        fi
        withpython="--with-python=$SDL_EXTERNALS_PATH/libraries/python-2.7.10/bin/python --with-python-root=$SDL_EXTERNALS_PATH/libraries/python-2.7.10"
        [[ -f b2 ]] || ./bootstrap.sh --with-toolset=$toolset $withicu $withpython
        local forceargs=
        if [[ $force ]] ; then
            forceargs=" -a"
        fi
        ./b2 $forceargs -q -d+2 -sICU_PATH=$icu boost.locale.iconv=off boost.locale.icu=on --threading=multi --runtime-link=shared --runtime-debugging=off --layout=versioned -j8
    )
}

macboost() {
    local bv=boost_1_59_0
    makeboost ~/src/$bv $xmtext
}
maccpu() {
    sysctl -n machdep.cpu.brand_string
}
diffhyp() {
    (
        cd ~/c/hyp
        for f in `findc sdl`; do
            [[ -f $xmtx/$f ]] || echo $f
        done
    )
}
findxcov() {
    find ${1:-.} -name 'cov.*.html' | perl -pe 's{^.*/cov\.}{};s{\.html$}{};s{_}{/}g' | sort
}
diffxcovt() {
    (
        covdir=${1:-$HOME/xcov}
        td=$HOME/tmp
        mkdir -p $td
        findxc=$td/findxc
        findxcov=$td/y
        (cd $xmtx/sdl;findc|sort) > $findxc
        diff $findxc $findxcov
        edit $findxc $findxcov
    )
}
diffxcov() {
    (
        covdir=${1:-$HOME/xcov}
        td=$HOME/tmp
        mkdir -p $td
        findxc=$td/findxc
        findxcov=$td/findxcov
        (cd $xmtx/sdl;findc|sort) > $findxc
        findxcov $covdir > $findxcov
        diff $findxc $findxcov
        edit $findxc $findxcov
    )
}
xcov() {
    gcovr -u -v -r $xmtx/sdl $xmtx/Debug -o $HOME/xcov/cov.html --html --html-details -e '.*xmt-externals.*'  -s
    #--exclude-unreachable-branches
}
gccshownative() {
    gcc -march=${1:-native} -c -o /dev/null -x c - &
    ps af | grep cc1
}
gccshowopt() {
    g++ -c -Q --help=optimizers "$@"
}
mmaptest() {
    for i in `seq 1 10`; do
        /home/graehl/pub/cmmap/xmt.sh -c /home/graehl/xeng/config/XMTConfig.yml --check-config --preload-resource lm --sleep 2000 &
        sleep 10
    done
}
gitdifftree() {
    git diff-tree --no-commit-id --name-only -r "$@"
}
grepwin() {
    cat "$@" | grep -v C4267 | grep -v C4251 | grep -v C4244 | grep -v C4018
}
mvstdoutexpected() {
    local suf=${1:-stdout-expected}
    for f in `find . -name "*.$suf"`; do
        if [[ -f $f ]] ; then
            echo $f
            git mv $f ${f%.$suf}.expected
        fi
    done
    perl -i~ -pe "s/\.$suf/.expected/" `find . -name 'regtest*.yml'`
}

gdbjam() {
    cd ~/jam
    in=$2
    [[ -f $in ]] || in="$in.in"
    cgdb --args ./$1 $in - 1 ${3:-${verbose:-1}}
}
cjam() {
    CFLAGS="-std=gnu++1z -Wno-deprecated -I ."
    if [[ $release ]]; then
        CFLAGS+=" -O3 -ffast-math -DNDEBUG"
        exesuf=.out
    else
        CFLAGS+=" -O0 -ggdb"
        exesuf=.dbg
    fi
    CFLAGS+=" -pthread"
    usegcc
    (
        src=$1
        shift
        in=$1
        shift
        base=${src%.cc}
        exe=$base$exesuf
        set -e
        cd ~/jam
        echo src=$src
        [[ -f $src ]]
        if ! [[ -f $in ]] ; then
            baselarge=$base
            baselarge=${baselarge%-small}
            baselarge=${baselarge%-large}
            ls -rt ~/Downloads/$baselarge*.in
            in=`ls -t ~/Downloads/$baselarge*.in | head -1`
            cp $in .
            in=`basename $in`
            echo input $in
            head -2 $in
            echo ...
            echo
        fi
        [[ -f $in ]]
        which $CXX
        TERM=dumb $CXX $MORECFLAGS $CFLAGS $src -o $exe
        time ./$exe "$in" "$@"
        out=${in%.in}.out
        expected=$out.expected
        if [[ -f $out ]] ; then
            if [[ -f $expected ]] ; then
                # head $out $expected
                echo compared with expected $expected
                diff $out $expected | diffstat
            else
                echo                 cp $out $expected
                cp $out $expected
            fi
        fi
    )
}
ccjam() {
    src=$1
    in=$2
    shift
    shift
    (
        set -e
        cd ~
        #scp ~/u/codejam.hh $chost:u/
        followsymlink=1 sync2 $chost jam
        if ! [[ -f jam/$in ]] ; then
            in="$in.in"
        fi
        [[ -f jam/$in ]]
        ssh $chost "release=$release MORECFLAGS=$MORECFLAGS cjam $src $in $*"
        out=jam/${in%.in}.out
        scp $chost:$out $out
        head -50 $out
        echo ...
        echo $out
    ) 2>&1 | tee ~/tmp/last.ccjam
}
mertsans() {
    for san in address memory thread; do rm -f *.o mert; CC=clang SAN=$san make -j 4 && cp mert ~/pub/mert3.$san; done
}
tailf() {
    less -W +F
    # SHIFT+F will resume the 'tailing' (as mentioned above)
    # SHIFT+G will take you to the end of the file
    # g will take you to the beginning of the file
    # f will forward you one page
    # b will take you back one page
}
cpp11() {
    local debugargs
    [[ $debug ]] && debugargs="-g -O0"
    local src=$1
    local o=$src
    shift
    o=${o%.cc}
    o=${o%.cpp}
    o=${o%.c}
    TERM=dumb ${CXX:-g++} -std=c++11 -Wno-deprecated-declarations $debugargs $src -o $o && $o "$@"
}
trackmaster() {
    git branch --set-upstream-to=origin/master master
}
machyp() {
    (set -e
     cd $osgitdir
     cd $xmtx
     mend
     ~/x/scripts/release.sh $osgitdir "$@"
     osmake
    )
}
oshyp() {
    (set -e
     cd $osgitdir
     cd $xmtx
     mend
     usegcc
     ~/x/scripts/release.sh $osgitdir "$@"
     linosmake
    ) 2>&1 | tee ~/tmp/oshyp
}
hownfc() {
    echo -n "$*: "
    EditDistance --nfc=1 --norm1=1 "$1" "$1"
}
hownfc() {
    echo -n "$*: "
    EditDistance --nfd=1 --norm1=1 "$1" "$1"
}
opentrace() {
    strace -e open "$@"
}
hexdumps() {
    if [[ "$*" ]] ; then
        for f in "$@"; do
            echo $f
            hexdump -C $f | head
            echo
        done
    else
        hexdump -C
    fi
}
lastarg() {
    for f in "$@"; do
        lastarg=$f
    done
}
gitcp() {
    lastarg
    cp "$@" && git add "$lastarg"
}
hypcmake() {
    mkdir -p ~/c/buildhyp
    cd ~/c/buildhyp
    local hsdl=../hyp/sdl/
    ~/x/scripts/filter-cmake-for-release.py < ~/x/sdl/CMakeLists.txt > $hsdl/CMakeLists.txt
    ag cpp11 $hsdl/CMakeLists.txt $hsdl/SdlHelperFunctions.cmake
    rm CMake*;cmake $hsdl && make
}
gittracked() {
    git ls-files "$@" --error-unmatch
}
cg() {
    local prog=$1
    shift
    cgdb --fullname -ex "set args $*; r" $prog
}
btop() {
    top -b -n 1 -u $USER | grep '^ *[0-9]' | grep -v top | grep -v grep | grep -v ssh | grep -v bash
}
ctop() {
    c-s "top -b -n 1 -u $USER | grep '"'^ *[0-9]'"' | grep -v top | grep -v grep | grep -v ssh | grep -v bash"
}
diffmert() {
    for MERT_SPARSE in 0 1; do
        o=~/tmp/mert.sparse.$MERT_SPARSE.txt
        init=init.30 DEBUG=1 MERT_SPARSE=$MERT_SPARSE CC=clang out12 $o cmert -j 1 "$@" || true
    done
}
reforestviz() {
    overt;cd forest-em;make bin/pwn/forestviz.debug && ~/g/forest-em/bin/pwn/forestviz.debug -n -i sample/forests.gz -o sample/forests.dot && cat sample/forests.dot
}
araeng() {
    ${pre}xmt --pipeline decode_q2 --config /build/data/AraEng_Informal_U80_v_5_4_x_2/config/XMTConfig.yml --input-type=yaml -i /home/graehl/bugs/in.yml --derivation-info=1 --detokenizer.output-type=string
}
resherpmy() {
    rm -rf sparse-* dense-*
    sherp my.apex
}
inplace() {
    local f=${1:?inplace file cmd}
    shift
    (set -e
     local tmpf=`mktemp "$f.XXXXXX"`
     "$@" < $f > $tmpf
     echo "updated $f by $*"
     mv $tmpf $f
    )
}
a2c() {
    for f in aliases.sh; do
        scp ~/u/$f $chost:u/$f
        scp ~/u/$f deep:u/$f
    done
}
all2c() {
    for f in aliases.sh misc.sh time.sh; do
        scp ~/u/$f $chost:u/$f
    done
}

ospushmend() {
    (set -e
     cd $osgitdir
     cd $xmtx
     mend
     redox= ~/x/scripts/release.sh $osgitdir "$@"
     if [[ $pull ]] ; then
         git pull --rebase
     fi
     git push
    )
}
gitrevinit() {
    git rev-list --max-parents=0 HEAD
    #but git rebase [-i] --root $tip" can now be used to rewrite all the history
    #leading to "$tip" down to the root commit.
}
latpdf() {
    (set -e
     local f=$1
     shift
     f=${f%.}
     f=${f%.tex}
     pdflatex "$f"
     bibtex "$f"
     for i in 1 2; do
         pdflatex "$f"
     done
    )
}
servi() {
    tail -f ~/serviio/log/*.log
}
gitshows() {
    git --no-pager show -s "$@"
}
gitinfo_author_get() {
    local rev=${1:-HEAD}
    gitinfo_author=`git --no-pager show -s --format='%an <%ae>' $rev`
    echo $gitinfo_author
}
gitinfo_sha1_get() {
    local rev=${1:-HEAD}
    gitinfo_sha1=`git --no-pager show -s --format='%H' $rev`
    echo $gitinfo_sha1
}
gitinfo_changeid_get() {
    local rev=${1:-HEAD}
    gitinfo_changeid=`git --no-pager show -s $rev | grep Change-Id:`
    echo $gitinfo_changeid
}
gitinfo_subject_get() {
    local rev=${1:-HEAD}
    gitinfo_subject=`git --no-pager show -s --format='%s' $rev`
    echo $gitinfo_subject
}
gitinfo() {
    local rev=${1:-HEAD}
    gitinfo_subject_get
    gitinfo_sha1_get
    gitinfo_changeid_get
    gitinfo_author_get
    showvars_optional gitinfo_subject gitinfo_author gitinfo_sha1 gitinfo_author
}
gitcherry() {
    git cherry-pick ${1:---continue}
}
gitshows() {
    git show --name-status
}
oscom() {
    (
        set -e
        if [[ $redox ]] ; then
            cd $xmtx/docs/hyp
            latpdf hyp-tutorial && mv hyp-tutorial*pdf $xmtx/hyp-tutorial.pdf
        fi
        cd $xmtx
        gitinfo $1
        shift || true
        mend || true
        stagetarball=`mktemp $ostarball.XXXXXX`
        rm -f $stagetarball
        test= tarball=$stagetarball $xmtx/scripts/release.sh
        cd $osgitdir
        tar xzvf $stagetarball
        mv $stagetarball $ostarball
        TERM=dumb git status
        msg="$*"
        if ! [[ $msg ]] ; then
            msg=$gitinfo_subject
        fi
        if [[ $mend ]] ;then
            mend
        else
            git commit -a -m "$gitinfo_subject" -m "from SDL: $gitinfo_sha1" -m "$gitinfo_changeid"        --author="$gitinfo_author"
        fi
        git show --name-status
        echo $ostarball
        pwd
    )
}
reos() {
    mend=1 linoscom
}
linoscom() {
    oscom && linosmake
}
findsmall() {
    (
        local maxsz=${1:-50k}
        shift
        #-path .git -prune -o
        find .  -type f -size -$maxsz "$@" | fgrep -v .git
    )
}
substsmall() {
    (
        local tr=${1?trfile [maxsize default 50k]}
        shift
        findsmall "$@" | xargs subst.pl --inplace --tr $tr ``
    )
}
oscptar() {
    (set -e
     if false ; then
         cd $osgitdir
         c-s "mkdir -p $osgitdir" #rm -rf $osgitdir/$hypdir;
         scp $ostarball c-graehl:$osgitdir
         # ~/c/hyp
         c-s "cd $osgitdir && rm -rf sdl; tar xzf $(basename $ostarball)"
     else
         scpx $osgitdir c-graehl:$osgitdir/..
     fi
    )
}
osmake() {
    (
        export SDL_EXTERNALS_PATH=`echo ~/c/sdl-externals/$lwarch`
        set -e
        #rm -rf $osdirbuild
        mkdir -p $osdirbuild
        uselocalgccmac
        cd $osdirbuild
        showvars_required CC CXX
        $CXX -v
        cmake $osgitdir/$hypdir "$@" && TERM=dumb make -j4 VERBOSE=1
    )
}
osmakec() {
    (
        set -e
        cd $osdirbuild
        TERM=dumb make -j3 VERBOSE=1
    )
}
osrel() {
    (
        set -e
        cd $xmtx
        mend
        #rm -rf $osgitdir
        rm -f $ostarball
        test= tarball=$ostarball $xmtx/scripts/release.sh $osgitdir
        cd $osgitdir
    )
}
osrelmake() {
    (
        set -e
        export TERM=dumb
        osrel
        oscptar
        osmake
    )
}
linosmake() {
    (
        set -e
        export TERM=dumb
        oscptar
        SDL_BUILD_TYPE=Development
        #list-xmt-includes.py:20:18:skip_ifs = ['SDL_ASSERT_THREAD_SPECIFIC', 'SDL_OBJECT_COUNT', 'SDL_ENCRYPT',
        BUILD_TYPE=Debug
        if [[ $debug ]] ; then
            BUILD_TYPE=Debug
        fi
        if [[ $release ]] ; then
            BUILD_TYPE=Debug
        fi
        #-DSDL_BUILD_TYPE=$SDL_BUILD_TYPE
        sdlbuildarg="-DCMAKE_BUILD_TYPE=$BUILD_TYPE "
        local cleanpre
        [[ $noclean ]] || cleanpre="rm -rf $osdirbuild;"
        c-s ". ~/u/localgcc.sh;$cleanpre mkdir -p $osdirbuild;cd $osdirbuild; export SDL_EXTERNALS_PATH=/home/graehl/c/sdl-externals/FC12; cmake $sdlbuildarg $osgitdir/$hypdir && TERM=dumb make -j15 VERBOSE=0 && Hypergraph/hyp compose --project-output=false --in /local/graehl/xmt/RegressionTests/Hypergraph2/compose3a.hgtxt /local/graehl/xmt/RegressionTests/Hypergraph2/compose3b.hgtxt --log-level=warn" 2>&1 | filter-gcc-errors
    )
}
osreg() {
    cd $xmtx/RegressionTests
    ./runYaml.py -x rtmp -X -c -n -b $osdirbuild "$@"
    #-v --dump-on-error
}
linosrelmake() {
    (
        set -e
        export TERM=dumb
        osrel
        linosmake
    )
}
myip () {
    curl http://ipecho.net/plain; echo
}
previewr() {
    preview `find "$@" -type f` | less
}
agerr() {
    ag "$@" -G '.*\.err.*'
}
dcondor() {
    d-s md-condor
}
cprs() {
    cp -Rs "$@"
}
cpptox2() {
    (set -e
     toxmt=$HOME/c/x2
     fromxmt=$HOME/x
     rm -rf $toxmt
     mkdir $toxmt
     cd $fromxmt
     cpcpp $toxmt
    )
}
oddlines() {
    perl -ne '$x=!$x;print if $x' "$@"
}
evenlines() {
    perl -ne '$x=!$x;print unless $x' "$@"
}
longestline() {
    perl -ne 'if (length $_ > $max) { $max = length $_; $maxline = $_; $maxno = $. } END { print "longest #$maxno = $maxline" }' "$@"
}
guestread() {
    find . -type d -exec chmod 755 {} \;
    find . -exec chmod o+r {} \;
}
makedropcaches() {
    sudo gcc ~/u/dropcaches.c -o /usr/local/bin/dropcaches; sudo chmod 5755 /usr/local/bin/dropcaches
}
bdbreload() {
    (
        set -e
        mid=${2:-`basename ${1%.db}`.reload.db}
        db_dump $1 | db_load $mid
        ls -l $1 $mid
    )
}
savelns() {
    (
        set -e
        for f in "$@"; do
            if [[ -L $f ]] ; then
                r=`readlink $f`
                echo "symlink $f => $r - replacing with copy"
                rm $f
                cp -a $r $f
            fi
        done
    )
}
cduh() {
    c-s 'd=/local/graehl/time/SE-SmallLM;du -h $d/rules.mdb{.uncompressed,}'
}
check12() {
    local out="$1"
    [[ -f $out ]] && mv "$out" "$out~"
    shift
    if "$@" > $out.new 2>&1; then
        mv $out.new $out
    else
        mv $out.new $out.err
        echo "ERROR: see $out.err"
        tail $out.err
    fi
    set +x
}
check10() {
    local outdir=${outdir:-~/tmp/check}
    mkdir -p $outdir
    local outs=
    for i in `seq 1 ${checkn:-10}`; do
        echo $i
        local f=$outdir/check.$i
        check12 $f "$@" &
        outs+=" $f"
    done
    wait
    tail $outdir/check.*err
    ls $outdir/check.*err
}
c12clang() {
    VERBOSE=1 save12 ~/tmp/c12clang cjen clang
}
gitcat() {
    git cat-file blob "$@"
}
gitcatm() {
    (
            set -x
            for f in "$@"; do
                echo2 $f
        gitcat ${rev:-origin/master}:$f
        done
    )
}
gitdiffm() {
    (set -x
     git diff ${rev:-origin/master} HEAD -- "$@"
     )
}
linuxver() {
    if [ -f /etc/redhat-release ]; then
        OS_MAJOR_VERSION=`sed -rn 's/.*([0-9])\.[0-9].*/\1/p' /etc/redhat-release`
        OS_MINOR_VERSION=`sed -rn 's/.*[0-9].([0-9]).*/\1/p' /etc/redhat-release`
        echo "RedHat/CentOS $OS_MAJOR_VERSION.$OS_MINOR_VERSION"
    elif grep -q DISTRIB_ /etc/lsb-release; then
        . /etc/lsb-release
        OS=$DISTRIB_ID
        VER=$DISTRIB_RELEASE
    elif [ -f /etc/debian_version ]; then
        OS=Debian # XXX or Ubuntu??
        VER=$(cat /etc/debian_version)
    else
        OS=$(uname -s)
        VER=$(uname -r)
    fi
}

gitbranches() {
    (
        for branch in ${1:-`git branch`}; do
            banner $branch
            git log -n 1 $branch
        done
    )
}
brewllvm() {
    brew install --HEAD ${1:-llvm} --with-libcxx --with-clang --rtti --all-targets
}
buildninja() {
    cd /local/graehl/src
    set -e
    (
        set -e
        git clone http://llvm.org/git/llvm.git
        (cd llvm/tools
         git clone http://llvm.org/git/clang.git)
        (
            cd clang/tools
            git clone http://llvm.org/git/clang-tools-extra.git extra
        )
        (
            cd llvm/projects
            git clone http://llvm.org/git/compiler-rt.git
        )
        # --with-gcc-toolchain
        sudo cp ninja /usr/bin/
    )
    (
        set -e
        git clone git://cmake.org/stage/cmake.git
        cd cmake
        git checkout next
        ./bootstrap
        make
        sudo make install
    )
}
buildclang() {
    usegcc
    cmake -DLLVM_ENABLE_PIC=ON -DLLVM_ENABLE_CXX1Y=ON -DLLVM_BUILD_STATIC=OFF /local/graehl/src/llvm
    make -j9 "$@"
    #LDFLAGS+='-pie' CFLAGS+='-fPIE' CXXFLAGS+='-fPIE'
}
cppfilenames() {
    grep -E '\.(hpp|cpp|ipp|cc|hh|c|h|C|H)$' "$@"
}
gitdiffstat() {
    git diff ${1:-HEAD^1} --shortstat
    git diff ${1:-HEAD^1} --dirstat
}
gitchangedfast() {
    git diff --cached --name-only --diff-filter=ACMRT ${1:-HEAD^1}
}
clangformatignore() {
    grep -E -v '\&\#10;|_DIAG_|></repl|> </repl' "$@"
}
clangformatargs="-style=file"
filenamesclangformat() {
    (
        set -o pipefail
        xargs -n1 clang-format $clangformatargs -output-replacements-xml | grep "<replacement " | clangformatignore >/dev/null
        if [ $? -ne 1 ]
           true
        then
            return 1
        fi
    )
}
diffnostatus() {
    (set +e
     diff "$@"
     true
    )
}
clangformatdiff() {
    for f in "$@"; do
        ff=~/tmp/clang-format.`basename $f`
        #touch $ff
        clang-format $clangformatargs $f > $ff
        # wc -l $f $ff
        diff=`diffnostatus -U0 $ff $f | perl -ne \
'chomp;if (!/^---|\+\+\+/ && /^[-+].*[^ }]$/ && !/(CLANG|GCC)_DIAG_(ON|OFF|IGNORE)|REGISTER_FEATURE/) {print STDERR "$_\n";print "1"; exit 1}'`
        if [[ $diff = 1 ]]; then
            echo $f
            return 1
        else
            echo OK: $f 1>&2
        fi
    done
}
isclangformatold() {
    (
        set -o pipefail
        clang-format $clangformatargs -output-replacements-xml "$@" | grep "<replacement " | clangformatignore > /dev/null
        if [ $? -ne 1 ]
           true
        then
            return 1
        fi
    )
}
isclangformat() {
    clangformatdiff "$@"
}
gitrootdir() {
    local root=$(git rev-parse --show-cdup)
    if [[ $root ]]  ; then
        echo $root
    else
        echo .
    fi
}
gitroot() {
    local root=$(git rev-parse --show-cdup)
    if [[ $root ]]  ; then
        cd $root
    fi
}
gitallclangformat() {
    (
        set -o pipefail
        gitchangedfast | cppfilenames | filenamesclangformat
    )
}
warnclangformats() {
    echo "WARNING: bad C++ formatting (run clang-format on changed sources): "
    fixed=
    for f in "$@"; do
        if isclangformat $f; then
            echo -n . 1>&2
        else
            fixed+=" $f"
            if [[ $fixclangformat ]] ; then
                clang-format -i $f
                preview $f
                echo fixed $f
            else
                echo $f
            fi
        fi
    done
    echo "WARNING: bad C++ formatting (run clang-format on changed sources above)"
    if [[ $fixclangformat ]] ; then
        preview $fixed
        echo fixed: $fixed
    else
        echo 'cd '`abspath $(gitrootdir)`'; for f in '"$fixed"'; do edit $f; done'
    fi
}
gitwarnallclang() {
    (
        gitroot
        warnclangformats `gitchangedfast | cppfilenames`
    )
}
gitwarnclang() {
    gitroot
    (
        set -o pipefail

        if gitallclangformat ; then
            echo "# C++ format OK"
        else
            gitwarnallclang
        fi
    )
}
gitfixclangformat() {
    bakthis preformat
    fixclangformat=1 gitwarnclangformat
}

addcremotes() {
    (
        set -e
        local csub=${1:? e.g. /home/graehl/c/xmt-externals}
        for chost in $chosts; do
            git remote add $chost ssh://graehl@$chost/$csub
        done
    )
}
if [[ $INSIDE_EMACS ]] ; then
    export PAGER=cat
elif [[ -x `which most 2>/dev/null` ]] ; then
    export PAGER=most
fi
HOST=${HOST:-`hostname`}
iops() {
    iostat –xn | grep Filesystem
    for (( i=1; i <= 100; i++ ))
    do
        iostat -xn 1 2 | grep lwfiler3-128| tail -n 1
        #| awk '{print $9}'
    done | tee /tmp/iops
}
ruledump() {
    (set -e
     require_file $1
     ${ruledumper:-~/pub/dgood/RuleDumper.sh} -c $1 -g ${2:-grammar}
    )
}
hextoescape3() {
    perl -pe 's|([0-9a-fA-F][0-9a-fA-F])|\\$1|g'
}
escape3cstr() {
    perl -i -pe 's|\\([0-9a-fA-F])|\\x$1|g'
}
ccpto() {
    (set -e
     cf=${2:-`relhome "$1"`}
     c-s "mkdir -p `dirname $cf`"
     scp -r "$1" "$chost:$cf"
    )
}
find_srcs() {
    find "$@" -name '*.java' -o -name '*.py' -o -name '*.md' -o -name '*.pl' -o -name '*.sh' -o -name '*.bat' -o -name '*.[chi]pp' -o -name '*.[ch]' -o -name '*.cc' -o -name '*.hh' -o -name '.gitignore'
}
gitdiffl() {
    git diff --stat mdbase | cut -d' ' -f2
}
gitsubset() {
    fullrepo=`pwd`
    subset=$fullrepo/../subset
    mkdir $subset
    find_srcs $fullrepo > $subset/srcs
    gitcredit -o $subset -f $subset/srcs
    cd $subset
    mkdir repo
    cd repo
    git init
    while read -d ' ' commit; do
        read author
        cp -r ../$commit/. .
        git add -A .
        git commit -m "$author import" --author="$author"
    done < ../authors
    git tag initial HEAD
    git log
    git archive --format=tar.gz -o ../initial.tar.gz -v HEAD
}
cpcpp() {
    dst=${1:-$HOME/b}
    mkdir -p $dst
    git ls | xargs tar cf - | (cd $dst && tar xvf -)
}
xcpps() {
    (set -e;
     cd $xmtxs
     x=`which $1`
     bakthis pre-$1
     mend >/dev/null 2>/dev/null
     $x `findc`
     git diff HEAD | tee ~/tmp/diff-$1
     diffstat HEAD
    )
}
gitdiffstatfull() {
    git diff --stat "$@"
}
optllvm() {
    export LLVM_DIR=/usr/local/opt/llvm
    export LDFLAGS=-L$LLVM_DIR/lib
    export CPPFLAGS=-I$LLVM_DIR/include
    export PATH=$LLVM_DIR/bin:$PATH
    export CMAKE_PREFIX_PATH=$LLVM_DIR
}
gitblame() {
    git blame -w -M -CCC "$@"
}
find_srcs() {
    findc
    find . -name '*.java' -o -name '*.py' -o -name '*.md' -o -name '*.pl' -o -name '*.sh' -o -name '*.bat'
}
findregyaml() {
    ag -g '/regtest[^/]*\.ya?ml$'
}
findyaml() {
    ag -g '\.ya?ml$'
}
findcmake() {
    find ${1:-.} -name CMakeLists\*.txt
}
findc() {
    find ${1:-.} -name '*.hpp' -o -name '*.cpp' -o -name '*.ipp' -o -name '*.cc' -o -name '*.hh' -o -name '*.c' -o -name '*.h' | sed 's/^\.\///' | grep -v trash.hpp | grep -v '^.#'
}
findentc() {
    findc entcore
    findc entutil
    findc entscripttests
    findc entuiwebsrv
}
substentc() {
    (
        substi "$@" `findentc`
    )
}

substc() {
    (
        substi "$@" `findc`
    )
}

substcpp() {
    (
        substi "$@" `findc`
    )
}
find_cpps() {
    find ${1:-.} -name '*.[chi]pp' -o -name '*.[ch]' -o -name '*.cc' -o -name '*.hh'
}
allauthorse() {
    git log --all --format='%aN %aN <%aE>' | sort -u
}
allauthors() {
    git log --all --format='%aN' | sort -u
}
blamestats() {
    git ls-tree --name-only -r HEAD | grep -E '\.(cc|h|hh|cpp|hpp|c)$' | xargs -n1 git blame -w -M --line-porcelain | grep "^author " | sort | uniq -c | sort -nr
}
run4eva() {
    c-s 'cd x/Release && forever make -j4'
}
cat4eva() {
    (
        latest=`c-s 'ls -rt /home/graehl/forever/make.-j4*' | head -2`
        echo $latest
        for f in $latest; do
            c-s ". .e;tailn=20 preview $f; grep 'error:' $f"
        done
    )
}

tree2() {
    (set -e
     to=$1
     shift
     echo to=$to
     require_dir $to
     for f in "$@"; do
         if [[ -f $f ]] ; then
             dst=$to/$f
             mkdir -p `dirname $dst`
             echo cp -a $f $dst
             cp -a $f $dst
         fi
     done
    )
}
gitshowcommit() {
    git show | head -1 || true
}
gitrecordcommit() {
    gitshowcommit > ORIGIN.git.commit
}


yumshow() {
    rpm -ql "$@"
}
findnongit() {
    find .
}
gitapplyforce() {
    gitforceapply "$@"
}
gitforceapply() {
    (
        set -e
        echo $1 ${2?gitapplyforce REV1 REV2}
        forcediff=`mktemp ~/tmp/gitapplyforce.$1-$2.XXXXXX`
        git diff $1 $2 --binary > $forcediff
        if [[ $gitclean ]]  ; then
            gitclean 1
            git apply $forcediff
            git add -A :/ # adds too much
        else
            git apply $forcediff
        fi
    )
}
cerr() {
    tailn=30 save12 ~/tmp/cerr preview `find . -name '*.err*'`
}
fireadb() {
    adb stop-server
    adb start-server
    adb connect 192.168.1.6
}
fireinstall() {
    fireadb
    adb install -r "$@"
}
firepull() {
    cd /tmp
    local d=/sdcard/android/data/org.xbmc.kodi/files/.xbmc/userdata
    local d=/sdcard/android/data/com.semperpax.spmc16/files/.spmc/userdata
    adb pull $d/advancedsettings.xml
    edit advancedsettings.xml && adb push advancedsettings.xml $d
}
jcp() {
    chost=$jhost ccp "$@"
}
mcp() {
    chost=c-mdreyer ccp "$@"
}
aganon() {
    ag --no-numbers --nogroup "$@" | perl -pe 's/^\S+://'
}
printarg1() {
    echo -n "$1: "
}
perseg() {
    perl -ne 'if (m{Total: processed.*taking (\S+)s .* per segment}) { print "$1 sec/seg\n" }' "$@"
}
avgperseg() {
    perseg "$@" | summarize_num.pl --prec 5 --avgonly 2>/dev/null
}
cstrings() {
    ldd "$@"
    nm "$@" | c++filt
}
cprs() {
    if [[ $2 ]] ; then
        cp -Rs `abspath "$1"` $2
        set +x
    fi
}
sedlns() {
    perl -pne '
BEGIN { $from=shift; $to=shift; }
chomp; $f=$_;$l=readlink($_);print "\n$f@ -> $l\n";
if ($f && $l =~ s{$from}{$to}oe) { @a=("ln", "-sfT", $l, $f); print join(" ",@a),"\n"; $ENV{dryrun} || system(@a); }
' "$@"
}
sedlinks() {
    find . -type l | sedlns "$@"
}
gitcherrytheirs() {
    git cherry-pick --strategy=recursive -X theirs "$@"
}
csuf() {
    echo ${chost#c-}
}
m12() {
    chost=c-mdreyer c12 "$@"
}
j12() {
    chost=$jhost c12 "$@"
}
y12() {
    chost=c-ydong c12 "$@"
}
c12() {
    (
        local o=${out:-$(echo ~/tmp/c12.`csuf`)}
        out12 "$o" c-s "$@"
        tail=20 preview1 "$o"
        fgrep 'Tokens=[' "$o"
        fgrep 'Total:' "$o" | egrep -v 'StringToHg|HgToString|hg-to-string|Pipeline'
        echo "$o"
    )
}
makesvm() {
    (
        set -e
        cd /Users/graehl/c/xmt-externals-source/svmtool++/svmtool++
        ./build.sh
    )
}
foreverdir=$HOME/forever
forc() {
    for chost in c-graehl git02 c-ydong; do
        chost=$chost c-s "$@"
    done
}
rmforever() {
    rm $foreverdir/*.run.*
    forc rm -rf /var/tmp/regtest*
}
cr-test() {
    (
        c-c
        b-r
        c-test "$@"
    )
}
jr-test() {
    (
        j-c
        b-r
        c-test "$@"
    )
}
j-test() {
    (
        j-c
        b-d
        c-test "$@"
    )
}
b-c() {
    BUILD=DebugClang
}
cc-test() {
    (
        c-c
        b-c
        c-test "$@"
    )
}

forever() {
    mkdir -p $foreverdir
    foreverlast=$foreverdir/`filename_from "$@"`
    foreverthis=$foreverdir/`filename_from "$@"`.run
    local i=0
    while true; do
        i=$((i+1))
        echo $foreverthis.$i
        echo
        echo ---
        "$@" 2>&1 | cat > $foreverthis.$i
        (echo; echo $foreverthis.$i) >> $foreverthis.$i
        echo
        echo ...
        sleep 5
        mv $foreverthis.$i $foreverlast
        echo $foreverlast
        if [[ $show ]] && [[ $i = 1 ]] ; then
            visual $foreverlast
        fi
    done
}
visual() {
    #--create-frame -e '(lower-frame)'
    $VISUAL --no-wait "$@"
}
edit12() {
    out12 "$@"
    visual "$1"
}
kills() {
    for f in "$@"; do
        pkill $f
        pgrepkill $f
    done
}
killxmt() {
    kills xmt cc1plus ld cmake python java
}
nohupf() {
    nohupf "$@" > nohup.`filename_from "$@"` &
}
cp3add() {
    cp "$1"/$2 $3
    git add $3
}
cpadd() {
    cp "$@"
    git add $2
}
vg12() {
    save12 ~/tmp/vg12.`csuf` c-s vg"$@"
}
diffs() {
    git diff ${1:-HEAD^1} --stat
}
upkenlm() {
    (set -e
     cd ~/src/kenlm/
     git fetch upstream
     git rebase upstream/master
    )
}
cpnplm() {
    cp $xmtxs/LanguageModel/KenLM/lm/wrappers/nplm* ~/src/kenlm/lm/wrappers/
}
comxmtken() {
    (set -e;
     cd ~/src/kenlm
     cp -a lm util $xmtxs/LanguageModel/KenLM/
     cd $xmtxs/LanguageModel/KenLM/
     git add lm util
     git commit -a -m 'kenlm'
    )
}
detumblr() {
    ls tumblr_* | perl -e '$re=q{^tumblr_(.*?)_(\d+)\.};
while(<>){chomp;push @l,$_;
$m{$1}=$2 if (/$re/ && $2 > $m{$1}) }
for(@l) {  print "rm $_\n" if (/$re/ && $m{$1} > $2) }
'
}
blame() {
    git blame -wM "$@"
}
alias gh='cd ~'
afterline() {
    perl -e '$pat=shift;while(<>) { last if m{$pat}o; };while(<>) {print}' "$@"
}
midway() {
    perl -e '$pat=shift;while(<>) { if (m{$pat}o) { print; last}  };while(<>) {print}' "$@"
}
fafterline() {
    perl -e '$pat=shift;while(<>) { last if m{\Q$pat\E}o; };while(<>) {print}' "$@"
}
fmidway() {
    perl -e '$pat=shift;while(<>) { if (m{\Q$pat\E}o) { print; last}  };while(<>) {print}' "$@"
}
gitundomaster() {
    (
        set -e
        git reset --hard ${1:-HEAD^1}
        git rebase -f master
        git push -f origin HEAD:master
    )
}
overc() {
    (
        set -e
        b=${3-~/bugs/over$chost}
        f=${2:-`basename $1`}
        log=$1
        log=`perl -e 'shift; s|/Users/graehl|~|g; print' "$log"`
        d=$b/$f
        mkdir -p $d
        cd $d
        ccp $1 j
        s=$d/do.sh
        echo chost=$chost > $s
        ccps j >> $s
        echo . $s
        sleep 3
        cd $xmtx
        . $s
    )
}
overj() {
    chost=git02 overc "$@"
}
bstart() {
    bsearchin=$1
    shift
    bsearchcmd="$*"
    showvars_required bsearchin bsearchcmd
    brun
}
brun() {
    (set -e
     if [[ -s $bsearchin ]] ; then
         splitutf8.pl $bsearchin >$bsearchin=0 2>$bsearchin=1
         if [[ -s $bsearchin=1 ]] ; then
             banner "0: $bsearchcmd $bsearchin=0 "
             $bsearchcmd $bsearchin=0 2>&1 | tee $bsearchin.log0 || true
             echo .....
             sleep 1
             banner "1: $bsearchcmd $bsearchin=1 "
             $bsearchcmd $bsearchin=1 2>&1 | tee $bsearchin.log1 || true
             preview $bsearchin.log0 $bsearchin.log1
             ls -l $bsearchin=0 $bsearchin=1
             [[ $nowc ]] || wc -w $bsearchin=0 $bsearchin=1
             banner "now bchoose 0 or bchoose 1 according to which log you like"
         else
             banner done: $bsearchin
         fi
     else
         echo no input
     fi
    )
}
bchoose() {
    local bb="$bsearchin=$1"
    require_file $bb
    echo chose $1: $bb
    bsearchin=$bb
    brun
}
uncache() {
    sudo 'sync; echo 3 > /proc/sys/vm/drop_caches'
}
reregr() {
    c-s cat $1 > $2
    c-with yregr ${regtest:-syntax}
}
olddirs() {
    find "${1:-.}" -maxdepth 1 -type d -mtime +${2:-1}
}
rmolddirs() {
    echo rmolddirs PATH DAYS removing directories modified earlier than $2 days ago in "${1:?dirname}" ...
    find "$1" -maxdepth 1 -type d -mtime +${2:-1} -exec rm -rf {} \;
}
bxmt() {
    if [[ $1 = d* ]] ; then
        BUILD=Debug bakxmt "$@"
    elif [[ $1 = w* ]] ; then
        BUILD=RelWithDebInfo bakxmt "$@"
    else
        BUILD=Release bakxmt "$@"
    fi
}
dothead() {
    if [[ -f "$1" ]] && [[ $2 -gt 0 ]] ; then
        head -$2 "$1" > "$1.$2"
    else
        error dothead file N
    fi
}
abspathf() {
    if [[ -r "$f" ]] ; then
        f=`abspath "$f"`
    elif [[ $f = -* ]] && [[ $f = *=* ]] ; then
        local p
        local second=0
        parts=`echo "$f" | tr = ' '`
        f=''
        for p in $parts; do
            if [[ $second = 1 ]] ; then
                if [[ -r $p ]] ; then
                    f+="$(abspath $p)"
                else
                    f+=$p
                fi
            else
                f+="$p="
                second=1
            fi
        done
    fi
}
sep0() {
    sepisfirst=1
}
sep() {
    if [[ $sepisfirst ]] ; then
        sepisfirst=0
        echo -n ' '
    fi
}
abspaths() {
    sep0
    for f in "$@"; do
        sep
        abspathf
        echo -n "$f"
    done
    echo
}
cmdabs() {
    sep0
    for f in $*; do
        sep
        abspathf
        echo -n "$f"
    done
    echo
}
shufi() {
    if [[ -f "$1" ]] ; then
        echo "shuf < $1 > $1.shuf"
        shuf < "$1" > "$1".shuf && mv "$1".shuf "$1"
    fi
}
addline() {
    file=$1
    shift
    if ! fgrep -q "$*" $file ; then
        echo "$*" >> $file
        tail $file
    fi
}
rpmdiffg() {
    USER=graehl rpmdiff c-jgraehl | grep ^+
}
yuminst() {
    which "$1"
    if fgrep -q "$1" ~/.yuminst ; then
        echo already have $1
    else
        #yum search "$1"
        sleep 1
        (set -e
         yum install "$1".x86_64 || yum install "$1".noarch
         addline ~/.yuminst "$@"
         rpmdiffg
        )
    fi
}
stripversion() {
    cut -d. -f1 "$@"
}
rpmnormalize() {
    stripversion | sort
}
rpmdiff() {
    local rpm1=`mktemp /tmp/rpm1.XXXXXX`
    local rpm2=`mktemp /tmp/rpm2.XXXXXX`
    rpm -qa | rpmnormalize > $rpm1
    ssh $USER@$1 'rpm -qa' | rpmnormalize > $rpm2
    diff --suppress-common-lines -u0 $rpm1 $rpm2
}
p1() {
    perl -lpE "$@"
}
n1() {
    perl -lnE "$@"
}
replines() {
    local n=${1:?[replines] repeats of input(s)}
    shift
    n1 'for$i(1..'$n'){say}' -- "$@"
}

nosleep() {
    sudo pmset -b sleep 0
}

# standby = hibernate / shutdown. messes w/ ssh
nostandby() {
    sudo pmset -b standby 0
}

########################
xmtvg="valgrind --leak-check=no --track-origins=yes --suppressions=$xmtx/jenkins/valgrind.fc12.debug.supp --num-callers=16 --leak-resolution=high"
xmtvgdb="$xmtvg --vgdb=full --db-attach=yes"
xmtmassif="valgrind --tool=massif --depth=16 --suppressions=$xmtx/jenkins/valgrind.fc12.debug.supp"

withvg() {
    prexmtsh=$xmtvg "$@"
}

withmassif() {
    prexmtsh=$xmtmassif "$@"
}

gitmodified() {
    git diff --name-only "$@"
}
removecr() {
    sed -i 's/\r//g' "$@"
}
githash() {
    git rev-parse HEAD
}
changeid() {
    local cid=`git log -1 | grep Change-Id: | cut -d':' -f2`
    echo $cid
}
rpathshow() {
    perl $xmtx/scripts/rpath.pl show ${1:-.}
}
rmrpath() {
    perl $xmtx/scripts/rpath.pl strip ${1:-.}
    perl $xmtx/scripts/rpath.pl show ${1:-.}
}
forcelink() {
    local usage="usage: forcelink src trg where trg is not a directory"
    if [[ ${3:-} ]] || ! [[ ${2:-} ]] ; then
        echo $usage
        return 1
    else
        if [[ -L "$2" ]] ; then rm -f "$2"; fi
        if [[ -d "$2" ]] ; then
            echo $usage
            return 2
        fi
        ln -sf "$@"
    fi
}
xmtbins="mert/mert2 RuleExtractor/MrNNSampleExtract Utf8Normalize/Utf8Normalize xmt/xmt xmt/XMTStandaloneClient xmt/XMTStandaloneServer Optimization/Optimize RuleSerializer/RuleSerializer RuleDumper/RuleDumper Hypergraph/hyp"
xmtpub=$(echo ~/pub)
rmxmt1() {
    (
        set -e
        mkdir -p $xmtpub
        if [[ -L $1 ]] ; then
            local d=`readlink $1`
            require_dir $d
            echo rm hash $d alias $1
            rm -rf $d
            rm $1
            local b=`basename $d`
            for f in ?????????????*/$b; do
                local change=${f%/$b}
                require_dir $change
                echo rm changeid $change alias $1
                rm -rf $change
            done
        fi
    )
}
rmxmt() {
    forall rmxmt1 "$@"
}
guesstbbver() {
    if [[ `chrpath "$1" | grep tbb-4.0` ]] ; then
        tbbver=4.0
    elif [[ `chrpath "$1" | grep tbb-4.2.3` ]] ; then
        tbbver=4.2
    else
        tbbver=42oss
    fi
}
xmtgithash() {
    ${1:-xmt} -v -D | perl -ne 'print $1 if /^Git SHA1: (\S+)/'
}
ext2pub12() {
    require_dir $2
    (
        from=$1/libraries
        ken=KenLM-5.3.0/5-gram
        libs=$ken
        #libs="$ken nplm nplm01 liblbfgs-1.10 tbb-4.4.0 lmdb apr-1.4.2 apr-util-1.3.10 zeromq-4.0.4 cmph-0.6 hadoop-hdp2.1 openssl-1.0.1e svmtool++ liblinear-1.94 protobuf-2.6.1 turboparser-2.3.1 boost_1_61_0 tinycdb-0.77 icu-55.1 db-5.3.15 zlib-1.2.8 OpenBLAS-0.2.14 bzip2 log4cxx-0.10.0 jemalloc"
        libs+=" OpenBLAS-0.2.14 ad3-1a08a9 apr-1.4.2 apr-util-1.3.10 boost_1_60_0 caffe-rc3 cmph-0.6 cryptopp-5.6.2 db-5.3.15 gflags-2.2 glog-0.3.5 hadoop-0.20.2-cdh3u3 hdf5-1.8.15-patch1 icu-55.1 jemalloc kytea-0.4.7 liblinear-1.94 lmdb-0.9.14 log4cxx-0.10.0 nplm nplm01 openssl-1.0.1e svmtool++ tbb-4.4.0 tinycdb-0.78 tinyxmlcpp-2.5.4 turboparser-2.3.1 zeromq-4.0.4 zlib-1.2.8 cnpy arrayfire-3.4.0"
        local dest=$2/lib
        mkdir -p $dest
        for d in $libs; do
            d=$from/$d/lib
            if [[ -d $d ]] ; then
                if cp -a $d/*so* $dest/; then
                    echo $d
                else
                    echo nothing for $d
                fi
            fi
        done
        cp -a $from/$ken/bin/* $dest/
        cp -a $from/gcc-${GCCVERSION:-6.1.0}/lib64/*.so* $dest/
    )
}
ext2pub() {
    ext2pub12 $xmtext $xmtpub
}
rpathorigin() {
    for f in "$@"; do
        if [[ -f $f ]] ; then
            echo $f
            chrpath -r '$ORIGIN/lib:$ORIGIN' $f || true
        fi
        if [[ -d $f ]] ; then
            rpathorigin `ls $f/*.so*`
        fi
    done
}
bakxmt() {
    ( set -e
      echo ${BUILD:=Release}
      cd $xmtx/$BUILD
      local change=`changeid`
      local hash
      local pub=${pub:-$xmtpub}
      hash=`xmtgithash xmt/xmt`
      if ! [[ $hash ]] ; then
          hash=`githash`
      fi
      mkdir -p $pub/lib
      ext2pub12 $xmtext $pub
      echo $pub/$1
      local bindir=$pub/$BUILD/$HOST/$hash
      echo $bindir
      mkdir -p $bindir
      git log -n 1 > $bindir/README
      mkdir -p $pub/$change
      rm -f $pub/latest $pub/$change/latest
      forcelink $bindir $pub/latest
      forcelink $pub/$change $pub/latest-changeid
      cp -af $xmtx/RegressionTests/launch_server.py $bindir/
      echo xmtbins: $xmtbins
      for f in $xmtbins xmt/lib/*.so TrainableCapitalizer/libsdl-TrainableCapitalizer-shared.so CrfDemo/libsdl-CrfDemo-shared.so TrainableCapitalizer/libsdl-TrainableCapitalizer_debug.so CrfDemo/libsdl-CrfDemo_debug.so; do
          local b=`basename $f`
          if [[ -f $f ]] ; then
          ls -l $f
          local bin=$bindir/$b
          cp -af $f $bindir/$b
          chrpath -r '$ORIGIN:'"$pub/lib" $bindir/$b
          if [[ ${f%.so} = $f ]] ; then
              (echo '#!/bin/bash';echo "export LD_LIBRARY_PATH=$bindir:$pub/lib"; echo "exec \$prexmtsh $bin "'"$@"') > $bin.sh
              chmod +x $bin.sh
          fi
          fi
      done
      grep "export LD_LIBRARY_PATH" $bindir/xmt.sh > $bindir/env.sh
      #rmrpath $bindir || true
      #rmrpath $pub/lib
      rpathorigin $pub/lib
      cat $bindir/README
      local pub2=~/bugs/leak
      if [[ $1 ]] ; then
          forcelink $bindir $pub/$1 && ls -l $pub/$1
          if [[ -d $pub2 ]] ; then
              forcelink $pub/$1 $pub2/$1
          fi
      fi
      $bindir/xmt.sh --help 2>&1
    )
}

cleantmp() {
    (
        df -h /tmp
        rm -rf /tmp/*
        du -h /tmp
        df -h /tmp
    ) 2>/dev/null
}
nh-for() {
    ( set -e
      for i in `seq 2 6`; do
          nhost=gitbuild$i
          echo2 nhost=$nhost "$@"
          nhost=$nhost "$@"
      done
    )
}
cs-s() {
    cs-for c-s "$@"
}
ns-for() {
    nh-for n-s "$@"
}
ns-s() {
    ns-for n-s "$@"
}
forcns() {
    cs-s "$@"
    ns-s "$@"
}
rmtmps() {
    forcns cleantmp
}
tabname() {
    printf "\e]1;$*\a"
    if [[ ${sleepname:-} ]] ; then sleep $sleepname ; fi
}

winname() {
    printf "\e]2;$*\a"
    if [[ ${sleepname:-} ]] ; then sleep $sleepname ; fi
}

tabwinname() {
    local name=$1
    shift
    if [[ $name ]] ; then
        tabname $name
    fi
    if [[ $1 ]] ; then
        winname "$@"
    fi
}

title() {
    if [[ $INSIDE_EMACS ]] ; then
        banner "$@"
    elif [[ $lwarch == Apple ]] ; then
        tabname "$@"
    else
        banner "$@"
    fi
}

xgerrit() {
    (
        cd $xmtx
        mend
        gerrit
    )
}
cxj() {
    ( set -e;
      cd $xmtextbase
      mend
      #c-s 'cd ~/c/xmt-externals && git reset --hard master && git config receive.denyCurrentBranch ignore'
      git push $chost
      c-s "cd $xmtextbase && git reset --hard HEAD && git clean -xf"
      cjen ${1:-Debug}
    )
}
whichq() {
    which "$@" 2>/dev/null || true
}
toarpa() {
    (
        cd `dirname $1`
        d=`pwd`
        b=`basename $1`
        c=${b%.gz}
        if [[ $c != $b ]] ; then
            c-s gunzip $d/$b
            b=$c
        fi
        c-s lwlmcat $d/$b > $b.arpa
        gzip $b.arpa
        wc -l $b.arpa
        git add $b.arpa.gz
    )
}
pictest() {
    readelf --relocs $1 | egrep '(GOT|PLT|JU?MP_SLOT)' > /dev/null
    if [[ $? != 0 ]]; then
        echo "PIC disabled: No relocatable sections found"
    else
        echo "PIC enabled!"
    fi
}
pullconfig() {
    (set -e
     cd /local/graehl/xmt-config
     git checkout config
     git pull --rebase origin refs/meta/config
    )
}
pushconfig() {
    (set -e
     cd /local/graehl/xmt-config
     #git reset --hard HEAD
     git checkout config
     git commit -a -m "$*"
     git pull --rebase origin refs/meta/config
     git push origin HEAD:refs/meta/config
    )
}
chostdomain=languageweaver.com
c-scan-view() {
    local port=8891
    local chostfull="$chost.$chostdomain"
    local url="http://$chostfull:${port:-}"
    echo $url
    browse $url
    c-s pgkill scan-view
    c-s scan-view --host=$chostfull --port=${port:-} --allow-all-hosts --no-browser "$@"
}
gerritlog() {
    chost=$jhost c-s tail -${1:-80} /local/gerrit/logs/error_log
}
xmtscanf() {
    blockclang=1 save12 ~/tmp/xmtscan.cpp c-with rm -rf $xmtx/DebugScan \; scanbuildnull=${scanbuildnull:-} scanbuildh=${scanbuildh:-} xmtcm DebugScan
}
xmtscan() {
    blockclang=1 save12 ~/tmp/xmtscan.cpp c-with scanbuildnull=${scanbuildnull:-} scanbuildh=${scanbuildh:-} xmtcm DebugScan
}
xscan() {
    blockclang=1 save12 ~/tmp/xmtscan.cpp c-with scanbuildnull=${scanbuildnull:-} scanbuildh=${scanbuildh:-} xmtm DebugScan
}
if [[ $HOST = $chost ]] || [[ $HOST = graehl.local ]] || [[ $HOST = graehl ]] ; then
    xmtx=/Users/graehl/x
fi
if [[ $HOST = c-ydong ]] || [[ $HOST = c-mdreyer ]] ; then
    xmtx=/.auto/home/graehl/x
fi
c-head() {
    c-s head "$@"
}
nbest1() {
    fgrep "nbest=${nbest:-1} " "$@"
}
nbest() {
    for f in "$@"; do
        echo "$f"
        nbest1 "$f" | head -10
        echo ...
    done
}
c-1best() {
    c-s nbest=$nbest nbest "$@"
}
c-cat() {
    c-s catz "$@"
}
j-cat() {
    j-s catz "$@"
}
d-cat() {
    d-s catz "$@"
}
c-sync() {
    sync2 $chost "$@"
}
home-c-sync() {
    (cd
     sync2 $chost "$@"
    )
}
c-make() {
    ctitle "$@"
    local tar=${1?target}
    shift
    (set -e;
     cd $xmtx; mend;
     local branch=`git_branch`
     pushc $branch
     c-s forceco $branch
     if [[ $BUILD = all ]] ; then
         builds="Debug Release"
     else
         builds=${BUILD:-Debug}
     fi
     for f in $builds; do
         (
        if [[ $ASAN ]] ; then
            f=${f%Asan}Asan
            echo ASAN: $f
        fi
     if [[ "$*" ]] ; then
         c-s ASAN=$ASAN BUILD=$f threads=${threads:-8} makeh $tar
         #'&&' "$@"
     else
         c-s ASAN=$ASAN BUILD=$f threads=${threads:-8} makeh $tar
     fi
     )  2>&1 | filter-gcc-errors
     done
    )
}
cs-for() {
    ( set -e
      for chost in c-ydong c-graehl c-mdreyer; do
          echo2 chost=$chost "$@"
          chost=$chost "$@"
      done
    )
}
cs-to() {
    cs-for c-to "$@"
}
c-s() {
    local chost=${chost:-c-graehl}
    local fwdenv="gccfilterargs=${gccfilterargs:-} gccfilter=${gccfilter:-} BUILD=${BUILD:-}"
    local pre=". ~/.e"
    local cdto=${cdto:-$(remotehome=/home/graehl trhomedir "$(pwd)")}
    local i=
    for i in `seq 1 ${crepeat:-1}`; do
        if false && [[ $ontunnel ]] ; then
            sshvia $chost "$pre; $fwdenv $(trhome "$trremotesubdir" "$trhomesubdir" "$@")"
        else
            sshlog $chost "$pre; $fwdenv $(trhome "$trremotesubdir" "$trhomesubdir" "$@")"
        fi
    done
}
y-s() {
    (y-c; c-s "$@")
}
g-c() {
    chost=c-graehl
}
g-s() {
    (g-c; c-s "$@")
}
j-s() {
    (j-c; c-s "$@")
}
yr-s() {
    (y-c; b-r; c-s "$@")
}
d-s() {
    (d-c; c-s "$@")
}
cs-s() {
    cs-for c-s "$@"
}
j-with() {
    chost=$jhost c-with "$@"
}
d-with() {
    chost=c-mdreyer c-with "$@"
}
g-with() {
    chost=c-graehl c-with "$@"
}
c-with() {
    (set -e;
     #touchnewer
     chost=${chost:-c-graehl}
     cd $xmtx; mend;
     local branch=`git_branch`
     pushc $branch
     c-s forceco $branch
     if [[ "$*" ]] ; then
         c-s "$@"
     fi
    )
}
d-with() {
    (d-c; c-with "$@")
}
k-with() {
    (k-c; c-with "$@")
}
y-with() {
    chost=c-ydong c-with "$@"
}
j-with() {
    (j-c; c-with "$@")
}
c-compile() {
    c-with BUILD=${BUILD:-} x-compile "$@"
}
c-xmtcompile() {
    c-with BUILD=${BUILD:-} xmtcompile "$@"
}
cr-makes() {
    (cr-c;BUILD=all c-make "$@")
}
cr-make() {
    (cr-c;c-make "$@")
}
cw-make() {
    (cw-c;c-make "$@")
}
k-make() {
    (k-c;c-make "$@")
}
kr-make() {
    (kr-c;c-make "$@")
}
j-make() {
    (j-c;c-make "$@")
}
jr-make() {
    (jr-c;c-make "$@")
}
d-make() {
    (d-c;c-make "$@")
}
dr-make() {
    (dr-c;c-make "$@")
    BUILD=Release chost=deep c-make "$@"
}

mr-make() {
    (mr-c;c-make "$@")
}
cr-c() {
    c-c
    b-r
}
cw-c() {
    c-c
    b-w
}

kr-c() {
    k-c
    b-r
}
jr-c() {
    j-c
    b-r
}
dr-c() {
    d-c
    b-r
}
b-r() {
    BUILD=Release
}
b-w() {
    BUILD=RelWithDebInfo
}
b-d() {
    BUILD=Debug
}
x-compile() {
    (
        set -e
        cd $xmtx/${BUILD:-Release}
        local dir=$1/CMakeFiles/$2.dir
        require_dir $dir
        make -f $dir/build.make $dir/src/$2.cpp.o
    )
}
xmtcompile() {
    x-compile xmt XMT
}
addpythonpath() {
    if ! [[ $PYTHONPATH ]] || ! [[ $PYTHONPATH = *$1* ]] ; then
        if [[ $PYTHONPATH ]] ; then
            export PYTHONPATH=$1:$PYTHONPATH
        else
            export PYTHONPATH=$1
        fi
    fi
}
fieldn() {
    awk '{ print $'${1:-1}'; }'
}
countcpus() {
    grep '^processor' /proc/cpuinfo | wc -l
}
ncpus() {
    if [[ $lwarch = Apple ]] || ! [[ -r /proc/cpuinfo ]] ; then
        if [[ $usecpus ]] ; then
            echo $usecpus
        else
            echo 1
        fi
    else
        local actual=`countcpus`
        local r=$((actual*2))
        if [[ `hostname` = deep ]]; then
            r=$actual
        fi
        echo $r
    fi
}
shuf() {
    awk 'BEGIN { srand() } { print rand() "\t" $0 }' | sort -n | cut -f2-
}
dtest() {
    chost=c-mdreyer c-test "$@"
}
ktest() {
    chost=c-ydong c-test "$@"
}
stun() {
    ssh -f -N tun
}
xmtpython() {
    addpythonpath $xmtx/python $xmtextbase/Shared/python
}
c-c() {
    chost=c-graehl
}
k-c() {
    chost=c-ydong
}
y-c() {
    chost=c-ydong
}
j-c() {
    chost=$jhost
}
d-c() {
    chost=deep
}
tocabs() {
    tohost $chost "$@"
}
c-to() {
    tohostp $chost "$@"
}
home-c-to() {
    (cd ~
     c-to "$@"
    )
}
newbranches() {
    cc-s "cd $xmtx;newbranch $1"
}
uprelocal() {
    upre localhost
}
upsdirect() {
    local b=${1:-`git_branch`}
    git branch --set-upstream-to origin/$b $b
}
upslocal() {
    local b=${1:-`git_branch`}
    git branch --set-upstream-to localhost/$b $b
}
save12pre() {
    local pre=$1
    shift
    pre+=".$(filename_from $*)"
    save12 "$pre" "$@"
    echo2 $pre
}
cregs() {
    save12pre ~/tmp/cregs c-s yreg "$@"
}
kregs() {
    save12pre ~/tmp/kregs krun yregr "$@"
}
xmtr=$xmtx/RegressionTests

touchnewerfile=/tmp/graehl.newer.than
touchnewer() {
    rmnewer
    touch $touchnewerfile
}
rmnewer() {
    rm -f $touchnewerfile
}
binelse() {
    if [[ -x `which $1` ]]  ; then
        echo $1
    elif [[ -x `which $2` ]] ; then
        echo $2
    else
        echo $1
    fi
}
extra_include=$xmtx
cpps_code_path=$xmtxs
cpps_defines="-DKENLM_MAX_ORDER=5 -DMAX_LMS=2 -DYAML_CPP_0_5 -I$SDL_EXTERNALS_PATH/../Shared/cpp/libraries/tinyxmlcpp-2.5.4 -I/Users/graehl/x/sdl/LanguageModel/KenLM -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers -I$SDL_EXTERNALS_PATH/../FC12/libraries/svmtool++/include/svmtool"
cpps_flags="-std=c++11 -DCPP11 -ftemplate-depth=255"
cppparses() {
    (
        set -e
        local newerarg=
        if [[ -f $touchnewerfile ]] ; then
            newerarg="! -neweraa $touchnewerfile"
        else
            touch $touchnewerfile
        fi
        if [[ $force ]] ; then
            newerarg=
        fi
        echo "find $xmtxs $newerarg -name '*.cpp'"
        ls -ul $touchnewerfile
        touch $touchnewerfile.newest
        local shuf=shuf
        if [[ $noshuf ]] ; then
            shuf=cat
        fi
        for f in "$@" `find $xmtxs $newerarg -name '*.cpp' | $shuf`; do
            if [[ -f $touchnewerfile.newest ]] ; then
                mv -f $touchnewerfile.newest $touchnewerfile
            fi
            local b=`basename $f`
            if [[ ${b#trash} = $b ]] && [[ $b != HgToReplaceFst.cpp ]] &&
                   [[ $b != cmph.cpp ]] &&
                   [[ $b != kenlm.cpp ]] &&
                   [[ ${b%Mapper.cpp} = $b ]] &&
                   [[ ${b%biglm.cpp} = $b ]] &&
                   [[ $b != TestLatticeMinBayesRisk.cpp ]]
            then
                ls -ul $f
                echo cppparse "$f" 1>&2
                cppparse "$f"
                touch $f
            fi
        done
    ) 2>&1 | tee ~/tmp/cppparses.hpp
}
showparses() {
    (set -e
     local out=${1:-~/tmp/cparse.last.cpp}
     save12 $out cppparses
     edit $out
    )
}
cppparse() {
    local cpps_code_path=${cpps_code_path:-.}
    cpps_code_path+=" $extra_include"
    local xmtshared=$SDL_EXTERNALS_PATH/../Shared/cpp/libraries
    local xmtarch=$SDL_EXTERNALS_PATH/libraries
    if [[ -d $SDL_EXTERNALS_PATH ]] ; then
        cpps_code_path+=" $(echo $xmtarch/lexertl-2012-07-26 $xmtarch/*/include $xmtshared/*/include $xmtshared/utf8)"
    fi
    local includeargs
    if [[ $cpps_code_path ]] ; then
        for f in $cpps_code_path ; do
            includeargs+=" -I $f"
        done
    fi
    local cc=`binelse clang g++`
    local warnarg=-Wall
    if [[ $cc = clang ]] ; then
        warnarg+=" -Wno-attributes -Wno-logical-op-parentheses -Wno-reorder"
    fi
    local ccarg="-x c++ -fsyntax-only $includeargs $warnarg $cpps_defines $cpps_flags"
    [[ $verbose ]] && echo $cc $ccarg "$@" 1>&2
    TERM=dumb $cc $ccarg "$@"
}
remaster() {
    ( set -e
      git fetch origin
      git reset --hard origin/master
      git checkout origin/master
      git branch -D master || true
      git checkout -b master origin/master
    )
}
yregs() {
    for i in `seq 1 10`; do save12 /tmp/krun.$i krun yregr CompoundSplitter/regtest-fin.yml | grep FAIL ; done
}
krun() {
    chost=c-ydong c-s "$@"
}
drun() {
    chost=c-mdreyer c-s "$@"
}
sd() {
    chost=c-mdreyer sc "$@"
}
if [[ $INSIDE_EMACS ]] ; then
    gitnopager="--no-pager"
else
    gitnopager=
fi

forceco() {
    local branch=${1:?args: branch}
    local xmtx=${2:-$xmtx}
    (
        set -e
        cd $xmtx
        git reset --hard $branch
        git log -n 1
    )
}
newbranch() {
    if [[ "$1" ]] ; then
        git fetch origin
        git reset --hard origin/master
        branchthis "$1"
    fi
}
pushg() {
    (
        #cd ~/g
        #mend
        #git rebase origin/master
        #cd ~
        #pushc master g
        #c-s 'forceco master g'
        cd ~
        syncc g
    )
}
lincar() {
    (set -e
     cd ~
     pushg
     c-s '. ~/.e;cd g; BOOST_SUFFIX=-gcc51-mt-1_59 buildcar'
    )
}
linfem() {
    (set -e
     pushg
     c-s 'BOOST_SUFFIX=-gcc51-mt-1_59 buildfem'
    )
}
pushc() {
    (
        local xmtx=${2:-$xmtx}
        cd $xmtx
        lock=.git/index.lock
        if [[ -f $lock ]] ; then
            echo git lock $lock exists - waiting ...
            sleep 5
            rm -f $lock
        fi
        if git diff --exit-code ; then
            echo no changes
        else
            echo changes ... amending first
            git commit --allow-empty -a -C HEAD --amend
        fi
        local b=${1:-`git_branch`}

        set -e
        if [[ ${force:-} ]] ; then
            # avoid non-fast fwd msg
            git push ${chost:-c-graehl} :$b || c-s "cd $xmtx;git reset --hard HEAD;git co origin/master other;newbranch $b; co other"
        fi
        git push ${chost:-c-graehl} +$b
    )
}
lincoxs() {
    for xmtx in $xmtx $xmtextbase; do
        lincox $xmtx
    done
}
lincox() {
    (set -e
     local xmtx=${1:-$xmtx}
     cd $xmtx
     local branch=`git_branch`
     pushc $branch $xmtx
     ctitle lincox "$@"
     c-s forceco $branch $xmtx
    )
}
c-tests() {
    cwith test=1 xmtcm
}
c-vgtest() {
    local test=$1
    shift
    c-make Test${test:-} "$@" vgTest$test
}
k-test() {
    (b-d;k-c;c-test "$@")
}
kr-test() {
    (b-r;k-c;c-test "$@")
}
d-test() {
    (b-d;d-c;c-test "$@")
}
dr-test() {
    (b-r;d-c;c-test "$@")
}
c-test() {
    local test=$1
    shift
    local rel
    if [[ $BUILD = Release ]] ; then
        rel=Release
    fi
    if [[ $ASAN ]] ; then
        rel=Asan
    fi
    local uarg
    if [[ $1 ]] ; then
        local u=$1
        shift
        uarg="--run_test=$u"
    fi
    (
        set -x
        set -e
    norun=1 ASAN=$ASAN c-make Test$test $uarg "$@"
    c-s ASAN=$ASAN Test$test$rel $uarg "$@"
    #norun= ASAN=$ASAN c-make Test${test:-} Test$test$rel $uarg "$@"
    #c-s BUILD=${BUILD:-Debug} Test$test$rel $uarg "$@"
    )
}
jr-test() {
    (b-r;j-c;c-test "$@")
}
gitmsg() {
    git log -n 1 "$@"
}
rebaseonmaster() {
    (set -x
     set -e
     local branch=`git_branch`
     local bak=$branch-for-rebase
     branchthis $bak
     local remote=${remote:-localhost}
     local onbranch=$remote-master-for-$branch
     killbranch $onbranch
     git fetch $remote
     git checkout -b $onbranch $remote/master
     git merge --squash --strategy=recursive -Xtheirs $bak
     gitmsg $bak
     git commit -a
    )
}
femacs() {
    local emacs=${emacs:-emacs}
    local emacs24=/usr/bin/emacs-24.2
    if [[ -x $emacs24 ]] ; then
        emacs=$emacs24
    fi
    fork $emacs "$@"
}
sync2emacs() {
    (
        set -x
        find ~/.emacs.d -name '*.elc' -exec rm {} \;
        sync2 $chost .emacs.d
    )
}
directssh() {
    (
        local sshdir=`echo ~/.ssh`
        cd ~/.ssh
        set -x
        cp -af $sshdir/config.direct $sshdir/config
        stun
    )
}
tunssh() {
    (
        local sshdir=`echo ~/.ssh`
        cd ~/.ssh
        cp -af $sshdir/config.tun $sshdir/config
        ssh -f -N tun
    )
}
homessh() {
    (
        local sshdir=`echo ~/.ssh`
        cd ~/.ssh
        set -e
        cp -af $sshdir/config.home $sshdir/config
        ssh -f -N tun
    )
}
less2() {
    local page=${page:-less}
    ( if ! [[ ${quiet:-} ]] ; then
          echo "$@"; echo
      fi
      "$@" 2>&1) | $page
    echo2 saved output:
    echo2 $out
}
nolog12() {
    save12 "$@" --log-config $xmtx/scripts/no.log.xml
}
dmacs() {
    fork dbus-launch --exit-with-session emacs
}
brewclang() {
    brew install --HEAD --with-clang "$@"
}
c12-clangxmt() {
    out=/tmp/clangxmt.hpp c12 xmtcm DebugClang
}
clangxmt() {
    lincox
    c-s xmtcm DebugClang
}
catgit() {
    local f=$1
    shift
    (set -e;
     if [ $# -gt 0 ] ; then
         echo -n "$@" > "$f"
     else
         cat > "$f"
     fi
     git add "$f"
    )
}

valgrind=`whichq valgrind`
gitbin=`whichq git`
git() {
    (
        $gitbin $gitnopager "$@"
    )
}
cpadd() {
    (set -e;
     cp "$1" "$2"
     git add "$2"
    )
}
touchadd() {
    (set -e;
     touch "$@"
     git add "$@"
    )
}
err() {
    echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $@" >&2
}
pkills() {
    for f in $chosts; do
        ssh $f ". ~/u/aliases.sh;pkill '$*'"
    done
}
#IFS="`printf '\n\t'`"
sshperm() {
    chmod go-w ~/
    chmod 700 ~/.ssh
    chmod 600 ~/.ssh/authorized_keys
}
setjavahome() {
    local jh=$JAVA_HOME
    if ! [[ -d $jh ]] ; then
        case $lwarch in
            Apple)
                jh=$(/usr/libexec/java_home)
                ;;
            *)
                export JAVA_HOME=$xmtext/libraries/jdk1.8.0_66
                #/home/hadoop/jdk1.6.0_24
                ;;
        esac
    fi
    export PATH=$JAVA_HOME/bin:$PATH
    export JAVA_HOME=$jh
}
setjavahome
mendgraehl() {
    mendauthor
}
mendauthor() {
    mend --reset-author
}
hangs() {
    for i in `seq -w 1 26`; do echo -n "$i "; /bin/ls /c"$i"_data | wc -l; done
    # hangs at the aforementioned numbers
}
upmaster() {
    local b=`git_branch`
    (
        git stash
        git checkout master
        git pull
        git stash pop
    )
}
localpig() {
    pig -x local "$@" -
}
testpys() {
    find . -name 'test*.py' -exec python {} \;
}
com() {
    git commit --allow-empty -m "$*"
}
branchnew() {
    git fetch origin
    git checkout -b "$1" origin/master
    git commit --allow-empty -m "$*"
}
nuser=nbuild
nidentity=$HOME/.ssh/nbuild-laptop
cuser=
cidentitty=
n-s() {
    local host=${nhost:-gitbuild1}
    local args="$host -l $nuser -i $nidentity"
    if [[ "$@" ]] ; then
        ssh $args '. ~/.e; export HOME=/home/nbuild;export PATH=/home/nbuild/local/bin:$PATH;export CCACHE_DIR=/local/nbuild/ccache;'" $*"
    else
        ssh $args
    fi
}
nhost=$jhost
ns-n() {
    nhost=gitbuild$n n-s "$@"
}
n6-s() {
    nhost=gitbuild6 n-s "$@"
}
n5-s() {
    nhost=gitbuild5 n-s "$@"
}
n4-s() {
    nhost=gitbuild4 n-s "$@"
}
n3-s() {
    nhost=gitbuild3 n-s "$@"
}
n2-s() {
    nhost=$jhost n-s "$@"
}
n1-s() {
    nhost=gitbuild1 n-s "$@"
}
nsn() {
    (set -x
     for n in `seq 3 6`; do
         ns-n "$@"
     done
    )
}

externals() {
    local newmastercmd="git fetch origin; git reset --hard origin/master; git checkout origin/master; git branch -D master; git checkout -b master origin/master"
    local ncmd="cd /jenkins/xmt-externals && ($newmastercmd)"
    bash -c "cd $xmtextbase && ($newmastercmd)"
    for host in c-graehl c-ydong c-mdreyer; do
        #localhost
        banner $host
        ssh $host "cd c/xmt-externals && ($newmastercmd)"
    done
    banner nbuild1
    nbuild1 $ncmd
    banner nbuild2
    nbuild2 $ncmd
}
HOST=${HOST:-$(hostname)}
if [[ $HOST = graehl.local ]] ; then
    CXX11=g++-4.7
else
    CXX11=g++
fi
export CCACHE_BASEDIR=$(realpath $xmtx)
chostfull=$chost.languageweaver.com
phost=pontus.languageweaver.com
HOME=${HOME:-$(echo ~)}
DROPBOX=$HOME/dropbox
octo=$HOME/src/octopress
jgpem=$octo/aws/jonathan.graehl.org.pem
jgip=ec2-54-218-0-133.us-west-2.compute.amazonaws.com
emacsd=$HOME/.emacs.d
carmeld=$HOME/g
overflowd=$HOME/music/music-overflow/
composed=$HOME/music/compose.rpp
puzzled=$HOME/puzzle
edud=$HOME/edu
gitroots="$emacsd $octo $carmeld $overflowd $composed"
nospace() {
    catz "$@" | tr -d ' '
}
spacechars() {
    catz "$@" | perl -e 'use open qw(:std :utf8); while (<>) {chomp;@chars = split //,$_; print join(" ",@chars),"\n"; }'
}
supertrims() {
    catz "$@" | perl -pe 'chomp;s/^\s+//;s/\s+$//;s/\s+/ /g;$_+="\n"'
}
trims() {
    catz "$@" | perl -pe 'chomp;s/^\s+//;s/\s+$//;$_+="\n"'
}
rtrims() {
    catz "$@" | perl -pe 'chomp;s/\s+$//;$_+="\n"'
}
translit() {
    lang=$1
    shift
    xmt --config $xmtx/RegressionTests/SimpleTransliterator/XMTConfig.yml -p translit-$lang --log-config $xmtx/scripts/no.log.xml
}
loch() {
    if ! [[ $JAVA_HOME ]]; then
        export JAVA_HOME=$SDL_EXTERNALS_JAVA
        export PATH=$JAVA_HOME/bin:$PATH
    fi
    # hadoop variables needed to run utilities at the command line,e.g.,pig/hadoop
    export HADOOP_INSTALL=/home/hadoop/hadoop/hadoop-2.0.0-cdh4.2.1
    export PATH=$HADOOP_INSTALL/bin:$PATH
    export HADOOP_CONF_DIR=/home/hadoop/loch2-client-conf
    export HADOOP_HOME=$HADOOP_INSTALL
    export PIG_HADOOP_VERSION=20
    export PIG_CLASSPATH=$HADOOP_CONF_DIR:/home/hadoop/hadoop/hadoop-2.0.0-cdh4.2.1/lib/pig/lib/jython-2.5.0.jar
}
bog() {
    if ! [[ $JAVA_HOME ]]; then
        export JAVA_HOME=$SDL_EXTERNALS_JAVA
        export PATH=$JAVA_HOME/bin:$PATH
    fi
    # hadoop variables needed to run utilities at the command line,e.g.,pig/hadoop
    export HADOOP_INSTALL=/home/hadoop/hadoop/hadoop-2.0.0-cdh4.3.0
    export PATH=$HADOOP_INSTALL/bin:$PATH
    export HADOOP_CONF_DIR=/home/hadoop/loch2-client-conf
    export HADOOP_HOME=$HADOOP_INSTALL
    export PIG_HADOOP_VERSION=20
    export PIG_CLASSPATH=$HADOOP_CONF_DIR:/home/hadoop/hadoop/hadoop-2.0.0-cdh4.3.0/lib/pig/lib/jython-standalone-2.5.2.jar
}
bak() {
    (
        local bak=${bakdir:-bak}
        mkdir -p $bak
        for f in "$@"; do
            local b=`basename $f`
            local o=$bak/$b
            if [ -r $o ] ; then
                for i in `seq 1 9`; do
                    o=$bak/$b.$i
                    [ -r $o ] || break
                done
                if [ -r $o ] ; then
                    o=$bak/$b.`timestamp`
                fi
                if [ -r $o ] ; then
                    rm -rf $o
                fi
            fi
            echo "$f => $o"
            mv $f $o
        done
    )
}
xpublish() {
    publish=1 pdf=1 save12 ~/tmp/xpublish $xmtx/scripts/make-docs.sh "$@"
}
xhtml() {
    open=1 publish= save12 ~/tmp/xhtml $xmtx/scripts/make-docs.sh "$@"
}
pmvover() {
    mvover "$@"
    pushover
}

forcecpan() {
    perl -MCPAN -e "CPAN::Shell->force(qw(install $*"
}

#e.g. makedo ce.tab uniqc 'sort $in | uniq -c'
makedo() {
    in=$1
    shift
    suf=.$1
    shift
    cat <<EOF > $in$suf.do
in=\${1%$suf}
redo-ifchange \$in
$*
EOF
    git add $in$suf.do
    echo $in$suf.do
}
do_sort() {
    makedo $1 sort 'sort $in'
}
do_shuf() {
    makedo $1 shuf 'sort -R $in'
}
do_uniqc() {
    do_sort $1 sort
    makedo $1.sort uniqc 'uniq -c $in'
}
do_uniq() {
    do_sort $1 sort
    makedo $1.sort uniq 'uniq $in'
}
editdistance() {
    local ed=${ed:-EditDistance}
    $ed --flag-segment=${chars:-false} --file1 "$1" --file2 "$2" $3
}
lc() {
    catz "$@" | tr A-Z a-z
}
do_lc() {
    makedo $1 lc 'tr A-Z a-z < $in'
}
do_hasuc() {
    makedo $1 hasuc 'grep [A-Z] $in'
}

sum() {
    perl -ne '@f=split " ",$_; $s+=$_ for (@f); END {print $s,"\n"}' "$@"
}

statbytes() {
    if [[ $# = 0 ]] ; then
        echo 0
    else
        if [[ $lwarch = Apple ]] ; then
            stat -f %z "$@"
        else
            stat -c %s "$@"
        fi
    fi
}
totalbytes() {
    statbytes "$@" | sum
}
justfiles() {
    for f in "$@"; do
        [[ -f $f ]] && echo $f
    done
}
filestatbytes() {
    statbytes `justfiles "$@"`
}
totalfilebytes() {
    totalbytes `justfiles "$@"`
}
maybeshortlines() {
    if [[ $linelen ]] ; then
        shortlines $linelen "$@"
    else
        cat "$@"
    fi
}
shortlines() {
    local linelen=${1:-80}
    shift
    local continuation=${continuation:-...}
    perl -ne 'BEGIN {$linelen=shift;$continuation=shift;$clen=$linelen-length($continuation);}
chomp;$c=length($_); print $c<=$linelen ? $_ : substr($_,0,$clen).$continuation,"\n"
' "$linelen" "$continuation" "$@"
}
#args must be files,not stdin (this is not a streaming algorithm)
sample() {
    local nlines=${1:-20}
    shift
    local linelen=${linelen:-132}
    local name=${name:-0}
    local sz=$(totalfilebytes "$@")
    #TODO: use linelen as hint or to print first whatever chars
    perl -ne 'BEGIN {$sz=shift;$nlines=shift;$name=shift;} use bytes;
if ($nlines) { $lines++; $cpre=$c||0; $c+=length($_); $left=$sz-$c; $avglen=$c/$lines || 1e100; $lineleft=$left/$avglen; if (!$lineleft || rand() < $nlines/$lineleft) { --$nlines; print "$ARGV: " if $name; print; } }
' "$sz" "$nlines" "$name" "$@" | shortlines $linelen
}
psample() {
    perl -ne 'BEGIN {$p=shift}print if rand() < $p' "$@"
}
pevery() {
    perl -ne 'BEGIN {$p=1./shift}print if rand() < $p' "$@"
}
ctestlast() {
    (
        cd $xmtx/${BUILD:-Debug}
        echo 'CTEST_CUSTOM_POST_TEST("cat Testing/Temporary/LastTest.log")' > CTestCustom.test
        make test
    )
}
xmtfails() {
    d=~/tmp
    cut -d' ' -f1 $d/fails > $d/fails.all
    cd $xmtx/RegressionTests/; grep -l sdl/xmt `cat $d/fails.all` > $d/fails.xmt
    cd $xmtx/RegressionTests/; grep -L sdl/xmt `cat $d/fails.all` > $d/fails.nonxmt
    tailn=20 preview $d/fails.xmt $d/fails.nonxmt
}
uncache() {
    sudo bash -c "sync; echo 3 > /proc/sys/vm/drop_caches"
}
fixgerrit() {
    ssh git02 "/local/gerrit/bin/gerrit.sh restart || /local/gerrit/bin/gerrit.sh start"
}
upext() {
    (set -e
     cd $xmtext
     remaster
     cd $xmtextsrc
     remaster
    )
}
gitshow() {
    sh -c "git show $*"
}
gitdiff() {
    sh -c "git diff $*"
}
pytimeit() {
    echo "$@"
    for f in pypy python ; do
        echo -n "$f: "
        $f -mtimeit -s"$@"
    done
}
gitunrm1() {
    if [[ -f "$1" ]] ; then
        warn "$1" already exists. remove it and try again?
    else
        git checkout $(git rev-list -n 1 HEAD -- "$1")^ -- "$1"
    fi
}
macxhosts() {
    sudo vi /etc/X0.hosts
}
boostmerge1() {
    (set -e;

     cd ~/src/boost
     git fetch upstream
     git checkout "$1"
     git merge remotes/upstream/master
     git push
    )
}
boostmaster() {
    (set -e;

     cd ~/src/boost
     git fetch upstream
     git checkout master
     git rebase remotes/upstream/master
     git push
    )
}
boostmerge() {

    boostmerge1 object_pool-constant-time-free
    boostmerge1 dynamic_bitset
}
reveal() {
    open -R "$@"
}
compush() {
    (set -e
     git commit -a -m "$*"
     # git pull --rebase
     git push
    )
}
mvover() {
    (set -e
     mv "$@" $overflowd/
     cd $overflowd
     git add *
    )
}
pushover() {
    (set -e
     cd $overflowd/
     git add *
     git commit -a -m more
     git pull --rebase
     git push
    )
}
mean() {
    sudo nice -n-20 ionice -c1 -n0 sudo -u $USER "$@"
}

brewhead() {
    brew unlink "$@"
    brew install "$@" --HEAD
}
overflow() {
    echo mv "$@" ~/music/music-overflow/
    mv "$@" ~/music/music-overflow/
}
forcepull() {
    git pull --rebase || (git stash; git pull --rebase; git stash pop || true)
}
pulls() {
    (set -e
     for f in $gitroots; do
         (cd $f; forcepull)
     done
     empull
    )
}
forcepush() {
    echo pushing `pwd` message = "$*"
    git commit -a -m "$*"
    forcepull
    git push
}
#modified including untracked. also includes ?? line noise output
gitmod() {
    git status --porcelain -- "*$1"
    git ls-files -mo
}
gitmodbase() {
    local ext=${1:?.extension}
    for f in `gitmod $ext`; do
        if [[ $f != '??' ]] && [[ $f != A ]] && [[ $f != M ]] ; then
            basename "$f" "$ext"
        fi
    done
}
octopush() {
    (
        cd $octo
        set -e
        git add source/_posts/*.markdown || true
        forcepush `gitmodbase .markdown`
    )
}
pushes() {
    (set -e
     for f in $gitroots; do
         (cd $f; forcepush push)
     done
    )
    octopush
}
sshjg() {
    ssh -i $jgpem ec2-user@$jgip "$@"
}
gitlsuntracked() {
    git ls-files --others --exclude-standard "$@"
}
gitexecuntracked() {
    gitlsuntracked -z |  xargs -0 --replace= {} "$@"
}
gitsync() {
    (set -e
     git pull --rebase
     git push
    )
}
web() {
    browse "$@"
}
uservm() {
    [[ -s "$HOME/.rvm/scripts/rvm" ]] && . "$HOME/.rvm/scripts/rvm"
}
rakedeploy() {
    (
        uservm
        cd ~/src/octopress
        set -e

        rake generate
        rake deploy
        git add -v -N source/_posts/*.markdown
        git commit -a -m 'deploy'
        git push origin
    )
}
chrometab() {
    open -a 'Google Chrome' "$@"
}
browse() {
    open -a 'Google Chrome' "$@"
}
rakegen() {
    (
        uservm
        cd ~/src/octopress
        rake generate[unpublished]
        web public/index.html
    )
}
rakepreview() {
    (
        uservm
        cd ~/src/octopress
        set -e
        rake generate[unpublished] &
        rake preview
        chrometab 'http://localhost:4000'
    )
}
rakedraft() {
    (
        uservm
        cd ~/src/octopress
        set -e
        rake draft["$*"]
    )
    rakepreview
}
newpost() {
    rakepost "$@"
}
rakepost() {
    (
        uservm
        cd ~/src/octopress
        set -e
        rake new_post["$*"]
        rakepreview
    )
}
splitflac() {
    #git clone git://github.com/ftrvxmtrx/split2flac.git
    split2flac "$@"
}
flacgain() {
    echo2 flac replay gain "$@"
    metaflac --no-utf8-convert --dont-use-padding --preserve-modtime --with-filename --add-replay-gain "$@"
}
mp3m4again() {
    echo2 mp3 or m4a replay gain "$@"
    aacgain -r -k -p "$@"
}
emailgains() {
    (
        set -e
        cd ~/email
        for f in *.mp3; do
            mp3m4again "$f"
        done
    )
}
mp3gains() {
    for f in "$@"; do
        mp3m4again "$f"
    done
}
gains() {
    for f in "$@"; do
        if [[ "${f%.flac}" = "$f" ]] ; then
            mp3m4again "$f"
        else
            flacgain "$f"
        fi
    done
}
gitrmsub() {
    local sub=${1:?submodule path}
    git status || return
    git config -f .git/config --remove-section submodule.$1
    git config -f .gitmodules --remove-section submodule.$1
    rm -rf $1
    git rm --cached -f $1
    git commit -a -m 'rm submodule $1'
    rm -rf $1 .git/modules/$1 .git/modules/path/to/$1
}

alias edbg="open /Applications/Emacs.app --args --debug-init"
alias cedbg="/Applications/Emacs.app/Contents/MacOS/Emacs -nw --debug-init"
overflow() {
    local overflow=`echo ~/music/music-overflow`
    require_dir "$overflow"
    mv "$@" "$overflow"
}

xsnap() {
    (set -e;
     cd $xmtx
     git stash && git stash apply
    )
}

gitsnap() {
    git stash && git stash apply
}

commg() {
    ( set -e
      cd ~/g
      gcom "$@"
    )
}

cto() {
    local dst=.
    if [[ $2 ]] ; then
        dst=$1
        shift
    fi
    (
        require_dir $dst
        for f in "$@"; do
            scp $chost:"$f" $dst
        done
    )
}
gerrit2local() {
    perl -e '$_=shift;s{C:/jenkins/workspace/XMT-Release-Windows}{/local/graehl/xmt}g;s{/local2/}{/local/}g;print' "$@"
}
nblanklines() {
    if grep -c -q ^$ "$@"; then
        echo `grep -c ^$ "$@"` "blank lines in $*"
    fi
}
ccpr() {
    recursive=1 ccp "$@"
}
ycp() {
    chost=c-ydong ccp "$@"
}
jcp() {
    chost=$jhost ccp "$@"
}
gcp() {
    chost=c-graehl ccp "$@"
}
wcp() {
    (
        set -x
        cd $xmtx/RegressionTests
        f=${2#C:/JENKINS/workspace/XMT-Release-Windows/RegressionTests/}
        f=${2#C:/JENKINS/workspace/XMT-Release-Windows-MSVC2015/RegressionTests/}
        f=${f#C:/jenkins/workspace/XMT-Release-Windows-MSVC2015/RegressionTests/}
        f=${f%.expected}
        f=${f%.stdout}
        f=${f%-stdout}
        f=${f%.windows}
        f=$f.windows
        local g=$1
        echo $f
        f=${2:-$f}
        scp Administrator@${wgitbuild:-wgitbuild2}:"$g" "$f"
        git add $f
    )
}
wcpa() {
    gitadd=1 wcp "$@"
}
ccp() {
    #uses: $chost for scp
    local n=$#
    local dst=${@:$n:$n}
    dst=`gerrit2local $dst`
    (set -e
     require_dir `dirname "$dst"`
     if [[ $(( -- n )) -gt 0 ]] ; then
         if [[ $n -gt 1 ]] ; then
             require_dir "$dst"
         fi
         for f in "${@:1:$n}"; do
             local cuserpre
             if [[ $cuser ]] ; then
                 cuserpre="$cuser@"
             fi
             local cargs
             if [[ ${cidentity:-} ]] ; then
                 cargs="-i $cidentity"
             fi
             local rargs
             if [[ $recursive = 1 ]] || [[ $recur = 1 ]] ; then
                 rargs="-r"
             fi
             echo scp $cargs $rargs $cuserpre$chost:"$f" "$dst"
             scp $cargs $rargs $cuserpre$chost:"$f" "$dst"
             #cd $xmtx
             # dst=`realpath $dst`
             #local p=`relpath $xmtx $dst`
             #echo "cd $xmtx;git add $p"
             #git add $p
             preview "$dst"
             nblanklines "$dst"
         done
     fi
    )
    if [[ $ccpgitadd = 1 ]] ; then
        (
            cd `dirname $dst`
            git add `basename $dst`
        )
    fi
}
ccpa() {
    ccpgitadd=1 ccp "$@"
}
ccp1() {
    local n=$#
    local dst=${@:$n:$n}
    ccp "$1" "$dst"
}
ncp() {
    chost=$nhost cidentity=$nidentity cuser=$nuser ccp "$@"
}
ccat() {
    c-s cat "$*"
}
csave() {
    save12 ~/tmp/c-s.`filename_from $1 $2` c-s "$@"
}
creg() {
    save12 ~/tmp/creg.`filename_from "$@"` cwith yreg "$@"
}
cregr() {
    save12 ~/tmp/cregr.`filename_from "$@"` cwith yregr "$@"
}
treg() {
    save12 /tmp/reg yreg "$@"
}
tregr() {
    save12 /tmp/reg yregr "$@"
}
cp2cbin() {
    scp "$@" c-graehl:/c01_data/graehl/bin/
}

findx() {
    find . "$@" -type f -perm -u+x
}

find_tests() {
    findx -name Test\*
}


memcheck() {
    local test=`basename $1`
    if [[ $valgrind ]] ; then
        local memlog=`mktemp /tmp/memcheck.$test.err.XXXXXX`
        local memout=`mktemp /tmp/memcheck.$test.out.XXXXXX`
        #TODO: could use --log-fd=3 >/dev/null 2>/dev/null 3>&2
        # need to test order of redirections,though
        local supp=$xmtx/jenkins/valgrind.fc12.debug.supp
        local valgrindargs="-q --tool=memcheck --leak-check=no --track-origins=yes --suppressions=$supp"
        save12 timeram $memout valgrind $valgrindargs --log-file=$memlog "$@"
        local rc=$?
        if [[ $rc -ne 0 ]] || [[ -s $memlog ]] ; then
            echo $hr
            echo "memcheck ${test:-} [FAIL]"
            echo $hr
            echo "cd `pwd` && valgrind $valgrindargs $1"
            ls -l $memout $memlog
            head -40 $memlog
            echo ...
            echo $memlog
            memcheckrc=88
            memcheckfailed+=" $test"
            echo $hr
        else
            cat $memlog 1>&2
            rm $memout
            rm $memlog
        fi
    fi
}


skiptest() {
    echo "skipping memcheck of $basetest (MEMCHECKALL=1 to include it)"
    echo $hr
    return 1
}

memchecktest() {
    ### TODO@JG: put these shell fns in a shell script and use GNU parallel (an
    ### awesome 150kb perl script) to do #threads - then we can run some of hte
    ### slower ones for validation. postponed due to decisions about accumulating
    ### failed tests' outputs (can't use shell vars)

    basetest=`basename $1`
    if [[ $MEMCHECKALL = 0 ]] ; then
        case ${basetest#Test} in
            #recommendation: exclude tests only if there are currently no errors and execution time is >10s
            LWUtil) # for some reason this one takes several minutes; the rest are 20-30 sec affairs
                skiptest ;;
            FeatureBot)
                skiptest ;;
            Vocabulary)
                skiptest ;;
            WordAligner)
                skiptest ;;
            SpellChecker)
                skiptest ;;
            LexicalRewrite)
                skiptest ;;
            RegexTokenizer)
                skiptest ;;
            Optimization)
                skiptest ;;
            Hypergraph)
                skiptest ;;
            Copy)
                skiptest ;;
            Concat)
                skiptest ;;
            BlockWeight)
                skiptest ;;
            OCRC)
                skiptest ;;
            *)
            ;;
        esac
    fi
}

memchecktests() {
    if [[ $valgrind ]] ; then
        local tests=`find_tests`
        echo "Running memcheck on unit tests:"
        echo $tests
        echo
        memcheckrc=0
        memcheckfailed=''
        for test in $tests; do
            if memchecktest ${test:-} ; then
                echo memcheck ${test:-} ...
                memcheck $test
                echo $hr
            fi
        done
        if [[ $memcheckrc -ne 0 ]] ; then
            #FINALRC=$memcheckrc
            #TODO: enable only once failures are fixed in master
            echo "memcheck failed for:$memcheckfailed"
        fi
    else
        echo "WARNING: valgrind not found. skipping MEMCHECKUNITTEST"
    fi
}

####

gdbargs() {
    local prog=$1
    shift
    gdb --fullname -ex "set args $*; r" $prog
}
rebaseadds() {
    local adds=`git ${rebasecmd:-rebase} --continue 2>&1 | perl -ne 'if (/^(.*): needs update/) { print $1," " }'`
    echo adds=$adds
    if [[ $adds ]] ; then
        git add -- $adds
    fi
    git ${rebasecmd:-rebase} --continue
}
rebasex() {
    cd $xmtx
    rebasenext
}
rebasecmdnext() {
    gitroot
    rebasecmdce `git ${rebasecmd:-rebase} --continue 2>&1 | perl -ne 'if (!$done && /^(.*): needs merge/) { print $1; $done=1; }'`
    rebaseadds
}
rebasenext() {
    rebasecmd=rebase rebasecmdnext "$@"
}
cherrynext() {
    rebasecmd=cherry-pick rebasecmdnext "$@"
}
gitconflicts() {
    git diff --name-only --diff-filter=U
}
mergenext() {
    gitroot
    gitconflicts
    f=`gitconflicts | head -1`
    if [[ -f $f ]] ; then
        edit $f && git add $f && echo "resolved $f"
    fi
}
agcmake() {
    ag -G 'CMakeLists.*txt$' "$@"
}
agsrc() {
    ag -G '\.(h|c|hh|cc|hpp|cpp|scala|py|pl|perl)$' "$@"
}
agsrce() {
    ag --nogroup --nocolor -G '\.(h|c|hh|cc|hpp|cpp|scala|py|pl|perl)$' "$@"
}

pepall() {
    pep `find . -name '*.py'`
}

flakeall() {
    flake `find . -name '*.py'`
}

pycheckall() {
    pychecker `find . -name '*.py'`
}

fork() {
    if [[ $ARCH = cygwin ]] ; then
        "$@" &
    else
        (setsid "$@" &)
    fi
}

launder() {
    xattr -d com.apple.quarantine "$@"
    xattr -d com.apple.metadata:kMDItemWhereFroms "$@"
}
interactive() {
    if [[ $- == *i* ]] ; then
        true
    else
        false
    fi
}
githistory() {
    gitstat "$@"
}
gitstat() {
    local file=$1
    shift
    git log --stat "$@" -- "$file"
}
pytest() {
    local TERM=$TERM
    if [[ $INSIDE_EMACS ]] ; then
        TERM=dumb
    fi
    TERM=$TERM py.test --assert=reinterp "$@"
}
yregs() {
    for f in "$@"; do
        yreg $f || return 1
    done
}
yregfilter() {
    grep -v '# Configuration: ' | egrep -v '^# [0-9]+ yaml subdirs|^# Found |^# Existing temp dirs'
}
yreg() {
    if [[ ${xmtShell:-} ]] ; then
        makeh xmtShell
    fi
    local args=${yargs:-}
    # -t 2
    (set -e;
     nbin=/home/nbuild/local/bin
     npython=$nbin/python2.7
     if [[ -x $npython ]] ; then
         python=$npython
     else
         python=`which python`
     fi
     pythonroot=$xmtx/python
     SDL_EXTERNALS_SHARED=$SDL_EXTERNALS/Shared
     SDL_SHARED_PYTHON=$SDL_EXTERNALS_SHARED/python
     #   export PYTHONHOME=$SDL_EXTERNALS_PATH/libraries/python-2.7.9
     #   PYTHONPATH=$PYTHONHOME/lib:$PYTHONHOME/lib/python2.7:$PYTHONHOME
     PYTHONPATH=$pythonroot:$SDL_SHARED_PYTHON:$PYTHONPATH
     export PYTHONPATH=${PYTHONPATH%:}
     export TMPDIR=${TMPDIR:-/var/tmp}
     bdir=${bdir:-$xmtx/${BUILD:=Debug}}
     export LD_LIBRARY_PATH=$bdir/xmt/lib:/local/gcc/lib64:$HOME/pub/lib:$LD_LIBRARY_PATH
     local logfile=/tmp/yreg.`filename_from "$@" ${BUILD:=Debug}`
     cd $xmtx/RegressionTests
     THREADS=${MAKEPROC:-`ncpus`}
     MINTHREADS=${MINTHREADS:-1} # unreliable with 1
     MAXTHREADS=8
     if [[ $THREADS -gt $MAXTHREADS ]] ; then
         THREADS=$MAXTHREADS
     fi
     if [[ $THREADS -lt $MINTHREADS ]] ; then
         THREADS=$MINTHREADS
     fi
     local STDERR_REGTEST
     if [[ ${verbose:-} ]] ; then
         STDERR_REGTEST="--dump-stderr"
     fi
     local memcheckparam
     if [[ ${memcheck:-} ]] || [[ ${MEMCHECKREGTEST:-} -eq 1 ]] ; then
         memcheckparam=--memcheck
     fi
     local REGTESTPARAMS="${REGTESTPARAMS:-} ${memcheckparam:-} -b $bdir -t $THREADS -x regtest_tmp_${USER}_${BUILD} ${GLOBAL_REGTEST_YAML_ARGS:-} ${STDERR_REGTEST:-}"
     if [[ ${exitfail:-} != 0 ]] ; then
         REGTESTPARAMS+=" -X"
     fi
     if [[ ${regverbose:-} != 0 ]] ; then
         REGTESTPARAMS+=" -v"
     fi
     if [[ ${regvalgrind} ]] ; then
         REGTESTPARAMS+=" --valgrind"
     fi
     if [[ ${monitor:-} ]] ; then
         REGTESTPARAMS+=" --monitor-stderr"
     fi
     xmtbins=
     if [[ ${xmtbin:-} ]] ; then
         xmtbins+=",$xmtbin"
     fi
     xmtpubdir=${xmtpubdir:-`echo ~/pub/`}
     if [[ ${xmtdir:-} ]] ; then
         xmtbins+=",$xmtpubdir/$xmtdir/xmt.sh"
     fi
     if [[ ${xmtdirs:-} ]] ; then
         for f in $xmtdirs; do
             xmtbins+=",$xmtpubdir/$f/xmt.sh"
         done
     fi
     export MAVEN_OPTS="-Xmx512m -Xms128m -Xss2m"
     export JAVA_HOME=/home/hadoop/jdk1.8.0_60
     export PATH=$JAVA_HOME/bin:$PATH
     export SDL_HADOOP_ROOT=$xmtextbase/FC12/libraries/hadoop-0.20.2-cdh3u3/
     if [[ ${1:-} ]] ; then
         local regr=$1
         shift
         for f in "$@"; do
             xmtbins+=",$xmtpubdir/$f/xmt.sh"
         done
         if [[ $xmtbins ]] ; then
             args+=" --xmt-bin $xmtbins"
         fi
         if [[ -d $regr ]] ; then
             save12 $logfile $python ./runYaml.py $args $REGTESTPARAMS "$regr"  | yregfilter
         else
             if ! [[ -f $regr ]] ; then
                 regr=${regr#regtest-}
                 regr=${regr%.yml}
                 regr=regtest-$regr.yml
             fi
             regr=$(basename $regr)
             save12 $logfile $python ./runYaml.py $args $REGTESTPARAMS -y $regr | yregfilter
         fi
     else
         save12 $logfile $python ./runYaml.py $args $REGTESTPARAMS | yregfilter
     fi
     set +x
     echo saved log:
     echo
     echo $logfile
    )
}


#TODO: screws up line editing
bashcmdprompt() {
    unset PROMPT_COMMAND
    case $HOST in
        graehl.local)
            DISPLAYHOST=mac ;;
        c-ydong*)
            DISPLAYHOST=kohli ;;
        c-graehl*)
            DISPLAYHOST=g ;;
        *)
            DISPLAYHOST=$HOST
    esac

    export PS1="\e]0;[$DISPLAYHOST] \w\007[$DISPLAYHOST] \w\$ "
    #set -T
    trap 'printf "\\e]0;[$DISPLAYHOST] %b\\007" $BASH_COMMAND' DEBUG
}
cmdprompt() {
    bashcmdprompt
    export PS1="[$DISPLAYHOST] \w\$ "
    trap 'echo -n "cd `pwd` && ";printf "%q " $BASH_COMMAND;echo' DEBUG
}

to5star() {
    mv "$@" ~/music/local/[5star]/
}
toscraps() {
    mv "$@" ~/dropbox/music-scraps/
}
stt() {
    StatisticalTokenizerTrain "$@"
}
sj() {
    chost=$jhost sc
}
sk() {
    chost=c-ydong sc
}
sd() {
    chost=c-mdreyer sc
}
kwith() {
    (
        chost=c-ydong c-with "$@"
    )
}
dwith() {
    (
        chost=c-mdreyer c-with "$@"
    )
}
cwith() {
    (
        chost=c-graehl c-with "$@"
    )
}
linjen() {
    cd $xmtx
    local branch=${branch:-`git_branch`}
    pushc $branch $xmtx
    ctitle jen "$@"
    (set -e;
     tmp2=`tmpnam`
     tmp=`echo ~/tmp`
     mkdir -p $tmp
     c-s forceco $branch 2>$tmp2
     tail $tmp2
     rm $tmp2
     log=$tmp/linjen.`csuf`.$branch.$BUILD
     mv $log ${log}2 || true
     GCCVERSION=${GCCVERSION:-6.1.0}
     gccprefix=$xmtextbase/FC12/libraries/gcc-$GCCVERSION
     #gccprefix=/local/gcc
     echo linjen:
     c-s GCC_SUFFIX=$GCC_SUFFIX nohup=$nohup JAVA_HOME=/home/hadoop/jdk1.8.0_60 SDL_HADOOP_ROOT=$xmtextbase/FC12/libraries/hadoop-0.20.2-cdh3u3/ SDL_ETS_BUILD=$SDL_ETS_BUILD NO_CCACHE=$NO_CCACHE gccprefix=$gccprefix GCCVERSION=$GCCVERSION SDL_NEW_BOOST=${SDL_NEW_BOOST:-1} NOLOCALGCC=$NOLOCALGCC gccprefix=$gccprefix GCCVERSION=$GCCVERSION SDL_BUILD_TYPE=$SDL_BUILD_TYPE SDL_BLM_MODEL=${SDL_BLM_MODEL:-1} NOPIPES=${NOPIPES:-1} RULEDEPENDENCIES=${RULEDEPENDENCIES:-1} USEBUILDSUBDIR=1 UNITTEST=${UNITTEST:-1} CLEANUP=${CLEANUP:-0} UPDATE=0 threads=${threads:-} VERBOSE=${VERBOSE:-0} ASAN=$ASAN ALL_SHARED=$ALL_SHARED SANITIZE=${SANITIZE:-address} ALLHGBINS=${ALLHGBINS:-0} NO_CCACHE=$NO_CCACHE jen "$@" 2>&1) | tee ~/tmp/linjen.`csuf`.$branch | ${filtercat:-$filtergccerr}
}
jen() {
    echo jen:
    cd $xmtx
    local build=${1:-${BUILD:-Release}}
    if ! [[ $SDL_BUILD_TYPE ]] ; then
        SDL_BUILD_TYPE=Development
        if [[ $build = Release ]] ; then
            SDL_BUILD_TYPE=Production
        fi
        #SDL_BUILD_TYPE=Production
    fi
    shift
    local log=$HOME/tmp/jen.log.${HOST:=`hostname`}.`timestamp`
    . xmtpath.sh
    usegcc
    if [[ $HOST = $chost ]] ; then
        export USE_BOOST_1_50=1
    fi
    if [[ ${memcheck:-} ]] ; then
        MEMCHECKUNITTEST=1
    else
        MEMCHECKUNITTEST=0
    fi
    if [[ ${memcheckall:-} ]] ; then
        MEMCHECKALL=1
    else
        MEMCHECKALL=0
    fi
    local nightlyargs
    if [[ ${nightly:-} ]] ; then
        nightlyargs="--memcheck --speedtest"
    fi
    local threads=${MAKEPROC:-`ncpus`}
    set -x
    if [[ $HOST = pwn ]] ; then
        UPDATE=0
    fi
    local ccargs=
    local cc
    local cxx
    if [[ -d $gccprefix ]] ; then
        cc=$gccprefix/bin/gcc
        cxx=$gccprefix/bin/g++
    fi
    nohupcmd=
    if [[ $nohup ]] ; then
        nohupcmd=nohup
    fi
    cmake=${cmake:-} CC=$cc CXX=$cxx GCC_SUFFIX=$GCC_SUFFIX gccprefix=$gccprefix SDL_ETS_BUILD=$SDL_ETS_BUILD GCCVERSION=$GCCVERSION SDL_NEW_BOOST=$SDL_NEW_BOOST NOLOCALGCC=$NOLOCALGCC RULEDEPENDENCIES=${RULEDEPENDENCIES:-1} NOPIPES=${NOPIPES:-1} SDL_BLM_MODEL=${SDL_BLM_MODEL:-1} USEBUILDSUBDIR=${USEBUILDSUBDIR:-1} CLEANUP=${CLEANUP:-0} UPDATE=$UPDATE MEMCHECKUNITTEST=$MEMCHECKUNITTEST MEMCHECKALL=$MEMCHECKALL DAYS_AGO=14 EARLY_PUBLISH=${pub2:-0} PUBLISH=${PUBLISH:-0} SDL_BUILD_TYPE=$SDL_BUILD_TYPE NO_CCACHE=$NO_CCACHE NORESET=1 SDL_BUILD_TYPE=${SDL_BUILD_TYPE:-Production} $nohupcmd jenkins/jenkins_buildscript --threads $threads --regverbose $build ${nightlyargs:-} "$@" 2>&1 | tee $log
    if [[ ${pub2:-} ]] ; then
        BUILD=$build bakxmt $pub2
    fi
    set +x
    wait
    echo
    echo $log
    fgrep '... FAIL' $log | grep -v postagger | sort > $log.fails
    uniqfails=$log.fails.uniq
    catz $log.fails | cut -d' ' -f1 $log.fails | uniq > $uniqfails
    nfails=`wc -l $uniqfails`
    cp $log.fails /tmp/last-fails
    echo
    cat /tmp/last-fails
    echo
    echo $log.fails
    echo "#fails = $nfails"
    echo
    catz $uniqfails >> $log.fails
    echo "#fails = $nfails" >> $log.fails
    echo overc $log
}


yjen(){
    chost=y linjen "$@"
}
jjen(){
    chost=$jhost linjen "$@"
}
djen() {
    chost=deep linjen "$@"
}
mjen() {
    chost=c-mdreyer linjen "$@"
}
rcjen() {
    chost=c-jgraehl linjen Release "$@"
}
cjen() {
    chost=c-graehl linjen "$@"
}
ymk() {
    chost=c-ydong c-make "$@"
}
dmk() {
    chost=c-mdreyer c-make "$@"
}
cmk() {
    chost=c-graehl c-make "$@"
}
gjen() {
    (set -e
     macget ${1:?mac-branch [jen args]}
     shift
     jen "$@"
    )
}
hgtrie() {
    StatisticalTokenizerTrain --whitespace-tokens --start-greedy-ascii-weight ''                      --unigram-addk=0 --addk=0 --unk-weight '' --xmt-block 0 --loop 0 "$@"
}
hgbest() {
    xmt --input-type FHG --pipeline Best "$@"
}
macflex() {
    LDFLAGS+=" -L/usr/local/opt/flex/lib"
    CPPFLAGS+=" -I/usr/local/opt/flex/include"
    CFLAGS+=" -I/usr/local/opt/flex/include"
    export LDFLAGS CPPFLAGS CFLAGS
}

xmtm() {
    (
        set -e
        racb ${1:-Debug}
        shift || true
        cd $xmtbuild
        local prog=${1:-}
        local maketar=
        if [[ ${prog:-} ]] ; then
            shift
            #rm -f {Autorule,Hypergraph,FsTokenize,LmCapitalize}/CMakeFiles/$prog.dir/{src,test}/$prog.cpp.o
            if [[ ${prog#Test} = $prog ]] ; then
                test=
                tests=
            else
                maketar=$prog
                prog=
            fi
        fi
        MAKEPROC=${MAKEPROC:-$(ncpus)}
        echo2 MAKEPROC=$MAKEPROC
        local premake
        identifybuild
        if [[ ${scanbuild:-} ]] ; then
            usescanbuild
            premake="scan-build -k"
            if [[ ${scanbuildh:-} ]] ; then
                premake+=" -analyzer-headers" # -v
            fi
            if [[ ${scanbuildnull:-} ]] ; then
                premake+="-Xanalyzer -analyzer-disable-checker=core.NullDereference"
                # -Xanalyzer -analyzer-disable-checker=core.NonNullParamChecker
            fi
        fi
        if [[ ${test:-} ]] ; then
            make check
        elif [[ $prog ]] && [[ "$*" ]]; then
            $premake make -j$MAKEPROC $prog && $prog "$@"
        else
            (
                set -x
                CCC_CC=$CCC_CC CCC_CXX=$CCC_CXX $premake make -j$MAKEPROC $maketar VERBOSE=${verbose:-0}
            )
            #VERBOSE=1
        fi
        set +x
        for t in $tests; do
            ( set -e;
              echo $t
              ts=
              [[ $BUILD = Debug ]] || ts=Release
              td=$(dirname $t)
              tn=$(basename $t)
              testexe=$td/Test$tn
              [[ -x $testexe ]] || testexe=$t/Test$tn
              $testgdb $testexe ${testarg:---catch_system_errors=no} 2>&1 | tee $td/$tn.log
            )
        done
    )
}

xmtc() {
    racb ${1:-Debug}
    shift || true
    cd $xmtbuild
    xcmake ../sdl $cmarg "$@"
}
xmtcm() {
    xmtc ${1:-Debug}
    xmtm "$@"
}
identifybuild() {
    buildcxxflags=${cxxflags:-}
    usegcc
    if [[ ${llvm:-} ]] ; then
        usellvm
    fi
    if [[ ${clang:-} ]] || [[ ${build%Clang} != $build ]] ; then
        useclang
        buildxcxxflags=-fmacro-backtrace-limit=200
    fi
    if [[ ${scanbuild:-} ]] || [[ ${build%Scan} != $build ]] ; then
        usescanbuild
    fi
}
xcmake() {
    local d=${1:-..}
    shift
    (
        rm -f CMakeCache.txt $d/CMakeCache.txt
        identifybuild
        local cxxf=${cxxflags:-}
        echo "cmake = $(which cmake)"
        CFLAGS= CXXFLAGS=${cxxf:-} CPPFLAGS= LDFLAGS=-v cmake $xmt_global_cmake_args $d -Wno-dev "$@"
    )
}
savecppch() {
    local code_path=${1:-.}
    local OUT=${2:-$HOME/tmp/cppcheck-$(basename `realpath $code_path`)}
    save12 $OUT cppch "$code_path"
    echo2 $cppcheck_cmd
}
cppch() {
    # Suppressions are comments of the form "// cppcheck-suppress ErrorType" on the line preceeding the error
    # see https://www.icts.uiowa.edu/confluence/pages/viewpage.action?pageId=63471714
    local code_path=${1:-.}
    local extra_include_paths=${extra_include_paths:-$code_path}
    if [[ $cppcheck_externals ]] ; then
        extra_include_paths+=" $(echo $SDL_EXTERNALS_PATH/libraries/*/include)"
    fi
    local extrachecks=${extrachecks:-performance}
    if [[ $extrachecks = none ]] ; then
        extrachecks=
    fi
    local ignoreargs
    local ignorefiles="" # list of (basename) filenames to skip
    if [[ $ignorefiles ]] ; then
        for f in $ignores ; do
            ignoreargs+=" -i $f"
        done
    fi
    local includeargs
    if [[ $extra_include_paths ]] ; then
        for f in $extra_include_paths ; do
            includeargs+=" -I $f"
        done
    fi
    local extraargs
    if [[ $extrachecks ]] ; then
        for f in $extracheck ; do
            extraargs+=" --enable=$f"
        done
    fi
    cppcheck_cmd="cppcheck -j 3 --inconclusive --quiet --force --inline-suppr \
        --template ' {file}: {line}: {severity},{id},{message}' \
        $includeargs $ignoreargs "$code_path" --error-exitcode=2"
    cppcheck -j 3 --inconclusive --quiet --force --inline-suppr     --template ' {file}: {line}: {severity},{id},{message}'     $includeargs $ignoreargs "$code_path" --error-exitcode=2
}
coma() {
    git commit -a -m "$*"
}
assume() {
    git update-index --assume-unchanged "$@"
}
unassume() {
    git update-index --no-assume-unchanged "$@"
}
assumed() {
    git ls-files -v | grep ^h | cut -c 3-
}
snapshot() {
    git stash save "snapshot: $(date)" && git stash apply "stash@ {0}"
}

pepignores=E501
#,E401
flake() {
    flake8 --ignore=$pepignores "$@"
}
pep() {
    pep8 --ignore=$pepignores "$@"
}
bakdocs() {
    (
        cd $xmtx/docs
        scp *.r *.rmd *.md *.txt $chost:projects/docs
    )
}
metaflacs() {
    for f in *.flac; do
        metaflac --with-filename --preserve-modtime --dont-use-padding "$@" "$f"
    done
}
cuesplit() {
    local cue=${1:?args file.cue [file.flac]}
    local flac=${2:-${cue%.cue}.flac}
    showvars_required cue flac
    require_files "$cue" "$flac"
    mkdir -p split
    shntool split -f "$cue" -o 'flac flac --output-name=%f -' -t '%n-%p-%t' "$flac"
    for f in *.flac; do
        if [[ "$f" != "$flac" ]] ; then
            flacgain "$f"
            mv "$f" split
        fi
    done
    #    shntool split -f "$cue" -o 'flac flacandgain %f' "$flac"
}
stripx() {
    strip "$@"
    upx "$@"
}
cpstripx() {
    local from=$1
    local to=${2:?from to}
    if [[ -d $to ]] ; then
        to+="/$(basename $from)"
    fi
    (set -e
     require_file "$from"
     cp "$from" "$to"
     chmod +x "$to"
     stripx "$to"
    )
}
topps() {
    local pid=${1?args PID N secperN}
    save12 ${out:-top.$pid} top -b -n ${2:-1000} -d ${3:-10} -p $pid
}
toppsq() {
    topps "$@" 2>/dev/null >/dev/null
}
lnccache() {
    local f
    local ccache=$(echo ~/bin/ccache)
    for f in "$@" ; do
        local g=$ccache-`basename $f`
        cat > $g <<EOF
#!/bin/bash
exec $ccache $f "\$@"
EOF
        chmod +x $g
        echo $g
    done
}
findsize1() {
    tailn=100 preview `find . -name '*pp' -size 1` > ~/tmp/size1
}
expsh() {
    perl -ne 'chomp;print qq {echo "$_"; pwd; hostname; date; $_; echo "No errors from $_" 1>&2\n}'
}
condorexp() {
    local f=$1/jobs.condor
    require_file $f
    condor_submit $f
    echo ls log/$f/
}
exp() {
    local f=${1%.sh}
    (set -e
     require_file $f.sh
     create-dir.pl $f
     condorexp $f
    )
}
redodo() {
    for f in "$@"; do
        redo-ifchange ${f%.do}
    done
}
reallr() {
    redodo `find ${*:-.} -name '*.do'`
}
cleanall() {
    noredo=1 clean=1 reall "$@"
}
reall() {
    local redos
    for d in ${*:-.}; do
        for f in $d/*.do; do
            redos+=" ${f%.do}"
        done
    done
    if [[ $clean ]] ; then
        rm -fv $redos
        for f in $redos; do
            rm -fv $f.redo?.tmp
        done
    fi
    if ! [[ $noredo ]] ; then
        (
            redo-ifchange $redos
        )
    fi
}
re() {
    redo-ifchange "$@"
}
dedup() {
    local f="$(filename_from $*)"
    catz "$@" | sort | uniq > $f.uniq
    (wc $* ; wc $f.uniq) | tee $f.uniq.wc
}
savedo() {
    local dir=${1:?arg1: dir to save .do outputs to}
    mkdir -p $dir
    local cmd=cp
    local cmdargs=-a
    if [[ $mv ]] ; then
        cmd=mv
        cmdargs=
    fi
    for f in `find . -name '*.do'`; do
        local o=${f%.do}
        if [[ -f $o ]] ; then
            cp -a $f $dir/$f
            $cmd $cmdargs $o $dir/$o
            ls -l $dir/$f $dir/$o
        fi
    done
}
sherp() {
    local CT=${CT:-`echo ~/c/ct/main`}
    cd `dirname $1`
    local apex=`basename $1`
    if [[ -f $apex.apex ]] ; then
        apex=$apex.apex
    fi
    local dir=${apex%.apex}
    if [[ $cleancondor ]] && [[ -d $dir ]] ; then
        echo rm -rf $dir
        rm -rf $dir
    fi
    (
        shift
        set -x
        # PATH=`dirname $CTPERL`:$PATH $CTPERL $CTPERLLIB
        perl $CT/sherpa/App/sherpa --force --apex=$apex "$@"
        #        PATH=`dirname $CTPERL`:$PATH $CTPERL $CTPERLLIB $CT/sherpa/App/sherpa --apex=$apex "$@"
        sleep 3
    )
}
resherp() {
    cleancondor=1 sherp "$@"
}
clonect() {
    git clone http://git02.languageweaver.com:29418/coretraining ct
}
macboost() {
    BOOST_VERSION=1_50_0
    BOOST_SUFFIX=
    if true ; then
        BOOST_SUFFIX=-xgcc42-mt-d-1_49
        BOOST_VERSION=1_49_0
    fi
    if [[ -d $SDL_EXTERNALS_PATH ]] ; then
        BOOST_INCLUDE=$SDL_EXTERNALS_PATH/libraries/boost_$BOOST_VERSION/include
        BOOST_LIB=$SDL_EXTERNALS_PATH/libraries/boost_$BOOST_VERSION/lib
        add_ldpath $BOOST_LIB
    fi
}
tokeng() {
    ${xmt:-$xmtx/Release/xmt/xmt} -c $xmtx/RegressionTests/RegexTokenizer/xmt.eng.yml -p eng-tokenize --log-config $xmtx/scripts/no.log.xml "$@"
}

rstudiopandoc() {
    Rscript -e '
options(rstudio.markdownToHTML =
  function(inputFile,outputFile) {
    system(paste("pandoc",shQuote(inputFile),"--webtex","--latex-engine=xelatex","--self-contained","-o",shQuote(outputFile)))
  }
)
'
}
panpdf() {
    local in=${1?in}
    local inbase=${in%.md}
    local out=${2:-$inbase.pdf}
    (
        texpath
        if [[ ! -f $in ]] ; then
            in=$inbase.md
        fi
        set -e
        require_file $in
        local geom="-V geometry=${paper:-letter}paper -V geometry=margin=${margin:-0.5cm} -V geometry=${orientation:-portrait} -V geometry=footskip=${footskip:-20pt}"
        #+implicit_figures
        #+line_blocks
        fontarg=
        if [[ $fonts ]] ; then
            fontarg="-V mainfont=${mainfont:-Constantia} -V sansfont=${sansfont:-Corbel} -V monofont=${monofont:-Consolas}"
        fi
        pandoc --webtex --latex-engine=xelatex --self-contained -r markdown+line_blocks -t latex -w latex -o $out --template ~/u/xetex.template --listings $fontarg  -V fontsize=${fontsize:-10pt} -V documentclass=${documentclass:-article}  $in
        if [[ $open ]] ; then
            open $out
        fi
    )
}
panwebtex() {
    pandoc --webtex --latex-engine=xelatex --self-contained -t html5 -o $1.html -c ~/u/pandoc.css $1.md
}
rmd() {
    RMDFILE=${1?missing RMDFILE.rmd}
    RMDFILE=${RMDFILE%.rmd}
    (
        set -e
        cd `dirname $RMDFILE`
        RMDFILE=`basename $RMDFILE`
        Rscript -e "require(knitr); require(markdown); require(ggplot2); require(reshape); knit('$RMDFILE.rmd','$RMDFILE.md');"
        #markdownToHTML('$RMDFILE.md','$RMDFILE.html',options=c('use_xhml'))
        pandoc --webtex --latex-engine=xelatex --self-contained -t html5 -o $RMDFILE.html -c ~/u/pandoc.css $RMDFILE.md
        if [[ $pdf ]] ; then
            panpdf $RMDFILE.md
        elif [[ $open ]] ; then
            open $RMDFILE.html
        fi
    )
}
gitgrep() {
    local expr=$1
    shift
    echo2 $expr
    git rev-list --all -- "$@"
    git rev-list --all -- "$@" | (
        while read revision; do
            git grep -e "$expr" $revision -- "$@"
        done
    )
    # git rev-list --all -- "$@" | xargs -I {} git grep -e "$expr" {} -- "$@"
}
subsync() {
    git submodule sync "$@"
}
mp3split() {
    mp3splt "$@"
}
printyaml() {
    ruby -ryaml -e 'y '"$*"
}
GRAEHLSRC=${GRAEHLSRC:-`echo ~/g`}
GLOBAL_REGTEST_YAML_ARGS="-c -n -v --dump-on-error"
maco=10.110.5.15
macl=192.168.1.7
tunrdp() {
    ssh -L33391:$1:3389 -p 4640 graehl@pontus.languageweaver.com
}
tsp() {
    tunrdp tsp-xp1
}
lagra() {
    #tunrdp lagraeh02
    tunrdp 10.110.5.5
}
sox() {
    connect -S 127.0.0.1:12345 "$@"
}
gitp() {
    GIT_PROXY_COMMAND=c:/msys/bin/sox.bat git "$@"
}
gitpf() {
    gitp fetch http://localhost:29418/xmt "$@"
}

giteol() {
    if ! [[ -f .gitattributes ]] ; then
        echo '* text=auto' > .gitattributes
        git add .gitattributes
        git commit -m 'auto line-end'
    fi
    git rm --cached -r .
    # Remove everything from the index.

    git reset --hard
    # Write both the index and working directory from git's database.

    git add .
    # Prepare to make a commit by staging all the files that will get normalized.

    # This is your chance to inspect which files were never normalized. You should
    # get lots of messages like: "warning: CRLF will be replaced by LF in file."

    git commit -m "Normalize line endings"
    # Commit
}
spd() {
    $xmtx/scripts/speedtest-stat-tok.sh "$@"
}
gpush() {
    (set -e
     pushd ~/g
     gcom "$@"
     git pull
     git push
    )
}
mendre() {
    (set -e
     cd $xmtx
     mend
     bakre "$@"
    )
}
bakx() {
    (set -e
     cd $xmtx
     bakthis "$@"
    )
}
bakre() {
    bakthis ${1:-prebase}
    upre
}
tryre() {
    local branch=$(git_branch)
    (set -e
     local to=$branch.re
     git branch -D $to || true
     git checkout -b $to
     up
     git rebase master || git rebase --abort
     co $branch
     git branch -D $to
    )
}

upre() {
    local remote=${1:-origin}
    local loosearg
    if [[ $loose ]] ; then
        #will keep *their* (master) whitespace, unlike 'merge'
        loosearg="-Xignore-all-space -Xdiff-algorithm=histogram"
    fi
    (set -e
     set -x
     git fetch $remote
     git rebase -Xpatience $loosearg ${rev:-$remote/master}
    ) || git rebase --continue
}
showbest() {
    for f in "$@" ; do
        echo $f 1>&2
        HgBest -n 1 $f -y 1 -Y 1 2>/dev/null
        echo
    done
}
randomport() {
    perl -e 'print int(12000+rand(10000))'
}
ssht() {
    ssh -p $tunport $tunhost "$@"
}
clonex() {
    (
        set -e
        git clone ssh://localhost:29418/xmt x
        cd x
        machost=192.168.1.7
        git remote add macw ssh://$machost:22/Users/graehl/c/xmt
        scp $machost:/Users/graehl/c/xmt/.git/config/commit-msg .git/config/commit-msg
    )
}
tunsocks() {
    # ssh -f -N -D 1080 -p $tunport $tunhost "$@"
    #ssh -D 1082 -f -C -q -N -p $tunport $tunhost
    ssh -f -C -q -N -D 12345 -p $tunport $tunhost
}
tunport() {
    local port=${1:?usage: port [host] via tunhost:tunport $tunhost:$tunport}
    local host=${2:-git02.languageweaver.com}
    local lport=${3:-${port:-}}
    ssh -N -L ${port:-}:$host:$lport -p $tunport $tunhost "$@"
}
ssht() {
    ssh -p $tunport $tunhost "$@"
}
tunmac() {
    tunport 22 $maco &
}
sshtun() {
    ssh -f -N tun
}
sshmaster() {
    fork ssh -MNn $chost
}
tungerrit() {
    #tunport 29418 git02.languageweaver.com
    ssh -L 29418:git02.languageweaver.com:29418 -L 3391:172.20.1.122:3389 -p $tunport $tunhost -N &
}
conf64() {
    ./configure --prefix=/msys --host=x86_64-w64-mingw32 "$@"
}
zillaq() {
    perl -n -e 'print $1,"\n" if m#<LocalFile>(.*)/[^/]*</LocalFile>#' "$@" | sort | uniq -c
}
cpfrom() {
    cp "$2" "$1"
}
ccpfrom() {
    scp $chost:"$2" "$1"
}
conflicts() {
    perl -ne 'print "$1 " if /^CONFLICT.* in (.*)$/; print "$1 " if /^(.*): needs merge$/' "$@"
}
gitcontinue() {
    git mergetool && git rebase --continue
}
gitrebase() {
    (set -e
     local last=${1:?usage: from-branch [HEAD]}
     git rebase "$@" || gitcontinue
    )
}
filtergccerr=filter-gcc-errors
filtergcc() {
    "$@" 2>&1 | $filtergccerr
}
gitstashun() {
    echo stashing unadded changes only so you can commit part
    git stash --keep-index
}
gitreplace() {
    local branch=$(git_branch)
    local to=${1:?arg1: destination branch e.g. master. replace by current branch}
    (
        set -e
        showvars_required branch to
        git merge --strategy=ours --no-commit $to
        git commit -m "replace $to by $branch by merging"
        git checkout $to
        git merge $branch
    )
}
gitrecycle() {
    git log --diff-filter=D --summary | grep delete
}
useclang() {
    local ccache=${ccache:-$(echo ~/bin/ccache)}
    local ccachepre="$ccache-"
    ccacheppre=
    export CC="${ccachepre}clang"
    export CXX="${ccachepre}clang++"
    export PATH=/local/clang/bin:$PATH
    export LD_LIBRARY_PATH=/local/clang/lib:$LD_LIBRARY_PATH
}

gcc48=
gcc47=1
gcc49=1
gcc5=1
gcc6=1
#TestWeight release optimizer problem
usegcc() {
    GCC_SUFFIX=
    if [[ $gcc6 ]] && [[ -x `which gcc-6 2>/dev/null` ]] ; then
        GCC_SUFFIX=-6
    elif [[ $gcc5 ]] && [[ -x `which gcc-5 2>/dev/null` ]] ; then
        GCC_SUFFIX=-5
    elif [[ $gcc49 ]] && [[ -x `which gcc-4.9 2>/dev/null` ]] ; then
        GCC_SUFFIX=-4.9
    elif [[ $gcc48 ]] && [[ -x `which gcc-4.8 2>/dev/null` ]] ; then
        GCC_SUFFIX=-4.8
    elif [[ $gcc47 ]] && [[ -x `which gcc-4.7 2>/dev/null` ]] ; then
        GCC_SUFFIX=-4.7
    fi
    local ccache=${ccache:-$(echo ~/bin/ccache)}
    ccachepre=$ccache-
    if [[ $noccache ]] || ! [[ -x ${ccachepre}gcc ]] ; then
        ccachepre=
    fi
    echo2 GCC_SUFFIX $GCC_SUFFIX
    export CC="${ccachepre}gcc${GCC_SUFFIX:-}"
    export CXX="${ccachepre}g++${GCC_SUFFIX:-}"
}
usepython() {
    SDL_EXTERNALS_PYTHONHOME=$SDL_EXTERNALS_PATH/libraries/python-2.7.10
    export PATH=$SDL_EXTERNALS_PYTHONHOME/bin:$PATH
    export LD_LIBRARY_PATH=$SDL_EXTERNALS_PYTHONHOME/lib:$LD_LIBRARY_PATH
}
usegccnocache() {
    export CC="gcc"
    export CXX="g++"
}
usellvm() {
    local ccache=${ccache:-$(echo ~/bin/ccache)}
    local gcc=${GCC_PREFIX:-/local/gcc}
    local llvm=/usr/local/llvm
    export PATH=$llvm/bin:$llvm/sbin:$PATH
    local gccm=$gcc/lib/gcc/x86_64-apple-darwin11.4.0
    export LIBRARY_PATH=$llvm/lib:$gccarch/4.7.1:$gcc/lib/gcc:$gcc/lib
    export CC="$ccache $llvm/bin/clang"
    export CXX="$ccache $llvm/bin/clang++"
}
withcc() {
    local source=$1
    require_file $source
    shift
    (local sdir=`dirname $source`
     cd $sdir
     local exe=$(basename $source .cc)
     exe=${exe%.cpp}
     exe=${exe%.c}
     local bexe=${bindir:-$sdir}/$exe
     local cxx=${CXX:-/local/gcc/bin/g++}
     [[ -x $cxx ]] || cxx=/mingw/bin/g++
     [[ -x $cxx ]] || cxx=g++
     set -e
     $cxx -O3 --std=c++${cxxstd:-11} $source -o $bexe
     if [ "$run $*" ] ; then
         $bexe "$@"
     fi
    )
}
dumbmake() {
    if [[ ${DUMB:-} ]] ; then
        /usr/bin/make VERBOSE=1 "$@" 2>&1
        #| sed -e 's%^.*: error: .*$%\x1b[37;41m&\x1b[m%' -e 's%^.*: warning: .*$%\x1b[30;43m&\x1b[m%'
    else
        /usr/bin/make "$@"
    fi
}
substxmt() {
    (
        cd $xmtxs
        substi "$@" $(ack --ignore-dir=LWUtil --ignore-dir=graehl --cpp -f)
    )
}
c-maker() {
    BUILD=Release c-make "$@"
}
kmaker() {
    chost=c-ydong BUILD=Release c-make "$@"
}
kmake() {
    chost=c-ydong BUILD=Debug c-make "$@"
}
linregr() {
    c-s yregr "$@"
}
ctitle() {
    title "$chost:" "$@"
}
rmautosave() {
    find . -name '\#*' -exec rm {} \;
}
gitdiffsub() {
    PAGER= git diff "$@"
    PAGER= git diff --submodule "$@"
}
gitdiff() {
    git difftool --tool=opendiff "$@"
}
ret() {
    #cpshared
    commt "$@"
}
shortstamp() {
    date +%m.%d_%H.%M
}
xmtcmsave() {
    local BUILD=${1:-Release}
    cd $xmtx
    local gbranch=$(git_branch)
    if [[ $gbranch = "(no branch)" ]] ; then
        gbranch=${branch:-headless}
    fi
    local binf=~/bin/xmt.$gbranch.`shortstamp`
    BUILD=${BUILD:-} makeh xmt && cp $xmtx/$BUILD/xmt/xmt $binf && ls -al $binf && [[ $clean ]] && rm ~/bin/xmt.$gbranch.* && cp $xmtx/$BUILD/xmt/xmt $binf
    echo $binf
    newxmt=$binf
}
pgkill() {
    local prefile=`mktemp /tmp/pgrep.pre.XXXXXX`
    pgrep "$@" | grep -v pkill | grep -v sshd | grep -v macs > $prefile
    cat $prefile
    if false && [[ -x /usr/bin/pkill ]] ; then
        /usr/bin/pkill "$@";
    else
        if [[ $KILL9 ]] ; then
            KILLSIG=-9
        fi
        local sudo=
        if [[ $KILLSUDO ]] ; then
            sudo=sudo
        fi
        set -x
        $sudo kill $KILLSIG `cat $prefile | cut -c1-5`
        set +x
        # | xargs kill
        echo before:
        cat $prefile
        sleep 1
    fi
    echo killed:
    cat $prefile | cut -c1-5
    rm $prefile
    echo now:
    pgrep "$@"
}
gccopt3() {
    local f=$1
    shift
    local ofile=~/tmp/gopt3.E.`basename $f`
    ${CXX:-g++} -fdump-tree-all -O${optimize:-3} -c "$f" "$@" -E -o $ofile
    echo $ofile

}
gccasm3() {
    local f=$1
    shift
    local ofile=~/tmp/gasm3.`basename $f`.s
    local lfile=~/tmp/gasm3.`basename $f`.lst
    ${CXX:-g++} -S -fverbose-asm -O${optimize:-3} -c "$f" "$@" -o $ofile
    if [[ $lwarch = Apple ]] ; then
        cp $ofile $lfile
    else
        as -alhnd $ofile > $lfile
    fi
    echo $lfile
}
sortk3() {
    local f=$1
    if [[ -f $f ]] && ! [[ $2 ]] ; then
        local sf=$f.sorted
        sort -k3 -n "$f" > $sf
        if ! diff -q $sf $f ; then
            echo sorted
            mv $sf $f
        else
            echo was already sorted
            rm $sf
        fi
    fi
}
xmtinstall() {
    (set -e
     cd $xmtx
     local gitbranch=${gitbranch:-${2:-master}}
     stamp=$gitbranch.`shortstamp`
     xmtcm Release
     cp $xmtx/Release/xmt/xmt ~/bin/xmt.$stamp
     installed=$xmtx/Release.$stamp
     cp -a $xmtx/Release $installed
     find $installed -name CMakeFiles -exec rm -rf {} \; || true
     find $installed -name Makefile -exec rm {} \;
     find $installed -name \*.cmake -exec rm {} \;
     ls -lr $xmtx/Release.$stamp
    )
}
mv1() {
    local to=$1
    shift
    if [[ $2 ]] ; then
        echo2 'mv1 expects 1 or 2 args (last is source file,first is dest)'
    else
        if [[ $1 ]] ; then
            mv "$1" $to
        else
            mv "$1" .
        fi
    fi
}
forpaste() {
    local f
    while read f; do
        "$@" $f
    done
}
yregr() {
    BUILD=Release yreg "$@"
}
yrega() {
    BUILD=DebugAsan yreg "$@"
}
rexmt() {
    (set -e
     cd $xmtx
     git pull --rebase
     xmtm "$@"
    )
}
rexmtr() {
    BUILD=Release rexmt
}
rexmty() {
    (set -e
     rexmt
     yreg "$@"
    )
}
rexmtry() {
    BUILD=Release rexmty
}
echo2() {
    echo "$@" 1>&2
}
page12() {
    psave12 /tmp/page12 "$@"
}
psave12() {
    page=less save12 "$@"
}
filterblock() {
    if [[ ${blockclang:-} ]] ; then
        blockline="clang: error: linker command failed with exit code"
    fi
    if [[ ${blockline:-} ]] ; then
        grep -v "$blockline" --line-buffered
    else
        cat
    fi
}
save12() {
    local out="$1"
    local filter=cat
    local filterarg=''
    [[ -f $out ]] && mv "$out" "$out~"
    shift
    local cmd=`abspaths "$@"`
    echo2 "$cmd"
    echo2 saving output $out
    ( if ! [[ ${quiet:-} ]] ; then
          echo "$@"; echo
      fi
      "$@" 2>&1) | filterblock | tee $out | ${filtercat:-$filtergccerr} | ${page:-cat}
    echo2 saved output:
    echo2 $out
    echo2 "$cmd"
}
out12() {
    local out="$1"
    local filter=cat
    local filterarg=''
    [[ -f $out ]] && mv "$out" "$out~"
    shift
    local cmd=`abspaths "$@"`
    echo2 "$cmd"
    echo2 saving output $out
    ( if ! [[ ${quiet:-} ]] ; then
          echo "$@"; echo
      fi
      "$@" 2>&1) | filterblock > $out.new;  mv $out.new $out
    echo2 saved output:
    echo2 $out
    echo2 "$cmd"
}
save2() {
    local out="$1"
    [[ -f $out ]] && mv "$out" "$out~"
    shift
    echo2 saving output $out
    ( if ! [[ ${quiet:-} ]] ; then
          echo2 "$@"; echo2
      fi
      "$@" ) 2>&1 2>$out.stdout | tee $out | ${page:-cat}
    echo
    tail $out $out.stdout
    echo2 saved output:
    echo2 $out $out.stdout
}
expn() {
    perl -e '
$n=shift;
$m=$n;
($m,$n)=($1,$2) if $n=~/(\d+)-(\d+)/;
print STDERR "$m...$n doublings of each line...\n";
while (<>) {
 chomp;
 for $i (1..$n) {
  print "$_\n" if $i>=$m;
  $_="$_ $_";
 }
}
' "$@"
}
vocab() {
    perl -ne 'chomp;for (split) {
push @w,$_ unless ($v {$_}++);
}
END { print "$v {$_} $_\n" for (@w) }
' "$@"
}
xmtspeed() {
    logf=${logf:-~/tmp/xmtspeed.`shortstamp`}
    save12 $logf xmt -c /c07_data/mhopkins/trainings/eng-chi-xmt/EC.mrx.pbmt.noisy.sep10.DeTok/XMTConfig.yml -p final-decode-no-cap-detok -i /home/akariuki/CA/Tests/CA-7xxx/xline.in.txt -o xline.out.txt 2>&1
}
linbuild() {
    cd $xmtx
    local branch=$(git_branch)
    mend
    c-s build=$build branch=$branch regs=$regs test=${test:-} all=$all reg=$reg macbuild $branch "$@" 2>&1 | tee ~/tmp/linbuild.$branch.`shortstamp` | $filtergccerr
}
splitape() {
    local file=${1%.ape}
    file=${file%.}
    (
        set -e
        cuebreakpoints "$file.cue" > "$file.offsets.txt"
        shntool split -f "$file.offsets.txt" -o ape -n "$file" "$file.ape"
    )
}
unhyphenate() {
    perl -pe 's/(?<=\w\w)- (?=\w+)//' "$@"
}
cpshared() {
    for f in $xmtxs/graehl/shared/*; do
        cp $f ~/g/graehl/shared
    done
}
commshared() {
    cpshared
    commt "$@"
}
pgrep() {
    local flags=ax
    ps $flags | fgrep "$@" | grep -v grep
}
pgrepn() {
    pgrep "$@" | cut -f1 -d' '
}
pgrepkill() {
    pgkill "$@"
    sleep 1
    KILL9=1 pgkill "$@"
}
overwrite() {
    if [[ $3 ]] ; then
        echo error: should be overwrite target src = cp src target
    fi
    cp "${2}" "${1%,}"
}
dryreg() {
    local args=${dryargs:--d}
    (set -e;
     cd $xmtx/RegressionTests
     if [[ -d $1 ]] ; then
         ./runYaml.py $args -b $xmtx/${BUILD:-Debug} -n -v "$@"
     elif [[ -f $1 ]] ; then
         ./runYaml.py $args -b $xmtx/${BUILD:-Debug} -n -v -y "$(basename $1)"
     elif [[ $1 ]] ; then
         ./runYaml.py $args -b $xmtx/${BUILD:-Debug} -n -v -y \*$1\*
     else
         ./runYaml.py $args -b $xmtx/${BUILD:-Debug} -n -v
     fi
    )
}

bakthis() {
    local branch=$(git_branch)
    if [[ $1 ]] ; then
        local suf=
        if [[ $1 ]] ; then
            suf=".$1"
        fi
        local to=$branch.b$suf
        killbranch $to
        git checkout -b "$to"
        git checkout $branch
    fi
}

savethis() {
    local branch=$(git_branch)
    if [[ $1 ]] ; then
        git checkout -b "$1"
        git checkout $branch
    fi
}
killbranch() {
    forall killbranch1 "$@" || true
}
killbranch1() {
    git branch -D "$1" || true
}
if [[ $HOST = c-ydong ]] || [[ $HOST = c-mdreyer ]] ; then
    export PATH=$SDL_EXTERNALS_PATH/tools/cmake/bin:$PATH
fi

ffmpegaudio1() {
    local input=$1
    ffmpeg -i "$input" -acodec copy "${input%.mp4}.aac"
}
ffmpegaudio() {
    forall ffmpegaudio1 "$@"
}
macget() {
    local branch=${1:?args: branch}
    (
        set -e
        cd $xmtx
        echo branch $branch tar $tar
        git fetch mac
        if [[ $force ]] ; then
            git branch -D $branch || true
            git checkout -b $branch remotes/mac/$branch
        fi
        git checkout remotes/mac/$branch
    )
}
macbuild() {
    local branch=${1:?args branch [target] [Debug|Release]}
    local tar=$2
    local build=${3:-${build:-Debug}}
    (
        set -e
        macget $branch
        if [[ ${test:-} ]] ; then
            test=1 xmtcm $build
        fi
        BUILD=$build makeh $tar
        if [[ $all ]] ; then
            xmtcm $build
        fi
        if [[ $reg ]] ; then
            BUILD=$build yreg $regs
        fi
    )
}
linevery() {
    test=1 all=1 reg=1 linbuild
}
mactest() {
    test=1 macbuild "$@"
}
macall() {
    all=1 mactest "$@"
}
macreg() {
    local branch=${1:-?args branch [regr-pat] [target] [Debug|Release|RelWithDebuInfo]}
    shift
    local regs=$1
    shift
    reg=1 regs=$regs macbuild $branch "$@"
}
soup() {
    ssh $souph "$@"
}
rebases() {
    perl -ne 'print "$1 " if /^(.*): needs merge/g'
    echo
}
rebasecmdce() {
    (
        for f in "$@"; do
            edit $f
        done
         rebasec "$@"
    )
}
rebasece() {
    rebasecmd=rebase rebasecmdce "$@"
}
cherryce() {
    rebasecmd=cherry-pick rebasecmdce "$@"
}
unadd() {
    if [[ $* ]] ; then
        git reset HEAD "$@"
    fi
}
g1po() {
    (
        cd $xmtxs
        export GRAEHL_INCLUDE=`pwd`
        cd $xmtxs/graehl/shared
        ARGS='--a.xs 1 2 --b.xs 2 --death-year=2011 --a.str 2 --b.str abc' cleanup=1 g1 configure_program_options.hpp -DGRAEHL_CONFIGURE_SAMPLE_MAIN=1
    )
}
overt() {
    pushd $xmtxs/graehl/shared
    gsh=$HOME/g/graehl/shared
    for f in *.?pp *.h; do
        rm -f $gsh/$f
        cp $f $gsh/$f
    done
#    cp $xmtx/scripts/gitcredit ~/c/gitcredit/
#    cp ~/c/mdb/libraries/liblmdb/mdb_from_db.{c,1} $gsh
    pushd ~/g
}
diffg() {
    (
        cd $xmtxs/graehl/shared
        gsh=$HOME/g/graehl/shared
        for f in *.?pp *.h; do
            diff $f $gsh/$f
        done
    )
}
commt() {
    (set -e
     overt
     compush "$@"
    )
}
branchthis() {
    if [[ $1 ]] ; then
        killbranch "$1"
        git checkout -b "$@"
    fi
}
linuxfonts() {
    (set -e
     sudo=sudo
     fontdir=/usr/local/share/fonts
     local t=`mktemp /tmp/fc-list.XXXXXX`
     local u=`mktemp /tmp/fc-list.XXXXXX`
     fc-list > $t
     $sudo mkdir -p $fontdir
     $sudo cp "$@" $fontdir
     $sudo fc-cache $fontdir
     fc-list > $u
     echo change in fc-list
     diff -u $t $u
    )
}
gstat() {
    git status
}
rebasec() {
    git add "$@"
    git ${rebasecmd:-rebase} --continue
}
oncommit() {
    refb=${1:-@{u}}
    refa=${2:-HEAD}
    reva=$(git rev-parse $refa)
    revb=$(git rev-parse $refb)
    echo2 "oncommit $refb $refa iff $reva = $revb"
    [[ $reva = $revb ]]
}
onupstreamcommit() {
    oncommit origin/master
}
mend() {
    (
        if [[ -x $xmtx/.git/rebase-apply ]] ; then
            echo2 mid-rebase already
            git rebase --continue
        elif oncommit; then
            git commit -a -m 'mend'
        else
            git commit --allow-empty -a -C HEAD --amend "$@"
        fi
    )
}
mendthis() {
    git commit -a --amend
}
mendq() {
    git commit --allow-empty -a -C HEAD --amend "$@"
}
xmend() {
    (
        gd=${1:-$xmtx}
        if [[ -x $gd/.git/rebase-apply ]] ; then
            echo2 mid-rebase already
            git rebase --continue
        else
            (
                pushd $gd
                git commit --allow-empty -a -C HEAD --amend "$@"
                popd
            )
        fi
    )
}


fastreg() {
    (set -e;
     cd $xmtx/RegressionTests
     for f in *; do
         [ -d $f ] && [ $f != BasicShell ] && [ $f != xmt ] && [ -x $f/run.pl ] && regressp $f
     done
    )
}
rencd() {
    local pre="$*"
    showvars_required pre
    local prein=
    local postin=" Audio Track.wav"
    local postout=".wav"
    local out=renamed
    mkdir -p $out
    echo "rencd $pre" > $out/rencd.sh
    local pfile=$out/rencd.prompt
    rm -f $pfile
    for i in `seq -f '%02g' 1 ${ntracks:-20}`; do
        local t="$pre - [$i] "
        read -e -p "$t"
        echo $REPLY >> $pfile
        cp "$prein$i$postin" "$out/$t$REPLY$postout"
    done
}
gitgc() {
    git gc --aggressive
}
branchmv() {
    git branch -m "$@"
}
gitrecase() {
    local to=${2:-$1}
    git mv $1 $1.tmp && git mv $1.tmp $to
    git commit -a -m "fix case => $to"
}
install_ccache() {
    for f in gcc g++ cc c++ ; do ln -s ccache /usr/local/bin/$f; done
}
gitclean() {
    local dry="-n"
    if [[ $1 ]] ; then
        dry="-f"
    fi
    git clean -d $dry
}
git_dirty() {
    if [[ $(git diff --shortstat 2> /dev/null | tail -n1) != "" ]] ; then
        echo -n "*"
    fi
}
git_untracked() {
    n=$(expr `git status --porcelain 2>/dev/null| grep "^??" | wc -l`)
    if [[ $n -gt 0 ]] ; then
        echo -n "($n)"
    fi
}
clonefreshxmt() {
    git clone ssh://graehl@git02.languageweaver.com:29418/xmt "$@"
    scp -p -P 29418 git02.languageweaver.com:hooks/commit-msg xmt/.git/hooks
}
forcebranch() {
    local branch=$(git_branch)
    local forceb=$1
    if [[ $forceb ]] ; then
        git branch -D $forceb || echo new branch $forceb of $branch
        echo from $branch to $forceb
        git checkout -b $forceb
    fi
}

upfrom() {
    local pullfrom=${1?args: [branch pull/rebase against] [backup#]}
    shift
    bakthis $1
    (set -e
     up $pullfrom
     git rebase $pullfrom
    )
}
breview() {
    local branch=$(git_branch)
    local rev=$branch-review
    git commit -a --amend # in case you forgot something
    (set -e
     if [[ $force ]] ; then
         forcebranch $rev
     else
         git checkout -b $rev
     fi
     up
     #git fetch origin
     #git rebase -i origin/master
     git rebase master
     git push origin HEAD:refs/for/master
     echo "git branch -D $rev # do this after gerrit merges it only"
    )
    echo ""
    git checkout $branch
    echo "git checkout $branch ; git fetch origin; git rebase origin/master # and this after gerrit merge"
}

drypull() {
    git fetch
    git diff ...origin
}
git_branch() {
    git branch --no-color 2> /dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/\1/'
}
rebase() {
    local branch=$(git_branch)
    git stash
    (set -e
     remaster
     co $branch
     git rebase master
    )
    git stash pop
    set +x
}
gd2() {
    echo branch \($1\) has these commits and \($2\) does not
    git log $2..$1 --no-merges --format='%h | Author:%an | Date:%ad | %s' --date=local
}
grin() {
    git fetch origin master
    gd2 FETCH_HEAD $(git_branch)
}
grout() {
    git fetch origin master
    gd2 $(git_branch) FETCH_HEAD
}
xlog() {
    (
        cd $xmtx
        gitlog
    )
}
gitlogp() {
    git log --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit --date=short --branches -p -$1
}
gitlog() {
    git log --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit --date=short --branches -n ${1:-20}
}
gitlog1() {
    gitlog 1
}
gitreview() {
    git review "$@"
}
gerritorigin="ssh://graehl@git02.languageweaver.com:29418/xmt"
gerritxmt() {
    echo git push $gerritorigin HEAD:refs/for/master
    git push $gerritorigin HEAD:refs/for/master
}
gerrit() {
    local gerritorigin=${1:-origin}
    echo git push $gerritorigin HEAD:refs/for/master
    [[ $dryrun ]] || git push $gerritorigin HEAD:refs/for/master
}
gerritfor() {
    local gerritorigin=origin
    local rbranch=${1:-remote-branch}
    echo git push $gerritorigin HEAD:refs/for/$rbranch
    git push $gerritorigin HEAD:refs/for/$rbranch
}
gerrit2() {
    gerritfor xmt-2.0
}
review() {
    git commit -a
    gerrit
}

push2() {
    local l=$1
    if [[ $2 ]] ; then
        l+=":$2"
    fi
    git push origin $l
}
status() {
    git status
    git branch -a
}
#branch.<branch>.remote and branch.<branch>.merge
#git config branch.master.remote yourGitHubRepo.git
config() {
    git config "$@"
}
remotes() {
    git remote -v
    git remote show origin
}
tracking() {
    git config --get-regexp ^br.*
}
gitkall() {
    gitk --all
}
co() {
    if [[ "$*" ]] ; then
        git checkout "$@"
    else
        git branch -a
    fi
}
up() {
    local branch=`git_branch`
    local upbranch=${1:-master}
    (set -e;
     git fetch origin
     co $upbranch
     git pull --stat
     git rebase
    )
    co $branch
}
branchmaster() {
    branchthis master
    git branch --set-upstream-to=origin/master master
}
newmaster() {
    (
        set -e
        git fetch origin
        git checkout origin/master
        killbranch master || true
        set -e
        git branch master --track origin/master
        git checkout master
    )
}
squash() {
    local branch=$(git_branch)
    (set -e
     remaster $branch
     git rebase -i master
    )
}
squashmaster() {
    git reset --soft $(git merge-base master HEAD)
}
branch() {
    if [[ "$*" ]] ; then
        co master
        git checkout -b "$@"
    else
        git branch -a
    fi
}
gamend() {
    if [[ "$*" ]] ; then
        local marg=
        local arg=--no-edit
        # since git 1.7.9 only.
        if [[ "$*" ]] ; then
            marg="-m"
            arg="$*"
        fi
        git commit -a --amend $marg "$arg"
    else
        git commit -C HEAD --amend
    fi
}
amend() {
    git commit -a --amend "$@"
}
xmtclone() {
    git clone ssh://graehl@git02.languageweaver.com:29418/xmt "$@"
}
findcmake() {
    find . -name CMakeLists\*.txt
}
findc() {
    find . -name '*.hpp' -o -name '*.cpp' -o -name '*.ipp' -o -name '*.cc' -o -name '*.hh' -o -name '*.c' -o -name '*.h'
}

tea() {
    local time=${1:-200}
    shift
    local msg="$*"
    [[ $msg ]] || msg="TEA TIME!"
    (date; sleep $time; growlnotify -m "$msg ($time sec)"; date) &
}

linxreg() {
    ssh $chost ". ~/a; BUILD=${BUILD:-Debug} fregress $*"
}
gcc5() {
    if [[ $HOST = $graehlmac ]] ; then
        GCC_SUFFIX=-6
        export CC=ccache-gcc$GCC_SUFFIX
        export CXX=ccache-g++$GCC_SUFFIX
        prepend_path $GCC_PREFIX
        add_ldpath $GCC_PREFIX/lib
    fi
}
gcc6() {
    if [[ $HOST = $graehlmac ]] ; then
        GCC_SUFFIX=-6
        export CC=ccache-gcc$GCC_SUFFIX
        export CXX=ccache-g++$GCC_SUFFIX
        prepend_path $GCC_PREFIX
        add_ldpath $GCC_PREFIX/lib
    fi
}

export PYTHONIOENCODING=utf_8
maketest() {
    (
        cd $xmtx/${BUILD:-Debug}
        if [[ $3 ]] ; then
            targ="-t $2/$3"
        elif [[ $2 ]] ; then
            targ="-t $2"
        fi
        local valgrindpre=
        if [[ $vg ]] && [[ $valgrind ]] ; then
            valgrindpre="$valgrind --db-attach=yes --tool=memcheck"
        fi
        local test=`find . -name Test$1`
        make VERBOSE=1 Test$1 && $valgrindpre ${test:-} $targ
    )
}
makerun() {
    (
        local exe=$1
        if [[ $exe = test ]] ; then
            exe=check
        fi
        shift
        if [[ $ASAN ]] ; then
            BUILD=${BUILD%Asan}Asan
            echo ASAN: BUILD $BUILD
        fi
        cd $xmtx/${BUILD:-Debug}
        echo -n "makerun pwd: "
        pwd
        local cpus=${threads:-`ncpus`}
        echo2 "makerun $cpus cpus ... -j$cpus"
        (set -e
         dumbmake $exe VERBOSE=1 -j$cpus || exit $?
         if [[ $exe != test ]] ; then
             set +x
             local f=$(echo */$exe)
             if ! [[ $norun ]] ; then
                 echo "exe '$exe' found: '$f' pathrun=$pathrun"
                 if [[ -f $f ]] ; then
                     if [[ $pathrun ]] ; then
                         $exe "$@"
                     else
                         $f "$@"
                     fi
                 fi
             fi
         fi
        )
    )
}
makex() {
    norun=1 makerun "$@"
}
makejust() {
    norun=1 makerun "$@"
}
makeh() {
    gcc6=1 usegcc
    showvars_required CXX CC
    if [[ $1 = xmt ]] ; then
        makerun xmtShell --help
        makerun XMTStandaloneClient --help
        makerun XMTStandaloneServer --help
        if [[ ${2:-} ]] ; then
            bakxmt $2
        fi
    elif [[ $1 = mert ]] ; then
        norun=1 makerun $1
        echo made mert
        makerun "$@" < /dev/null
    else
        makerun "$@" < /dev/null
    fi
}
hncomment() {
    fold -w 77 -s | sed "s/^/   /" | pbcopy
}
forcet() {
    for f in "$@"; do
        cp $xmtxs/graehl/shared/$f ~/t/graehl/shared
        lnshared1 $f
    done
}
no2() {
    echo cat $TEMP/no2.txt
    "$@" 2>$TEMP/no2.txt
}
fregress() {
    racer=$(echo ~/c/fresh/xmt) BUILD=${BUILD:-Debug} regress1 "$@"
}
regressdirs() {
    (
        cd $1
        for f in RegressionTests/*/run.pl ; do
            if [[ -f $f ]] ; then
                f=${f%/run.pl}
                f=${f#RegressionTests/}
                echo $f
            fi
        done
    )
}
regress1p() {
    local dir
    (
        cd ${racer:-$xmtx/}
        set -e
        for dir in "$@"; do
            local run=./RegressionTests/$dir/run.pl
            if [[ -x $run ]] ; then
                BUILD=${BUILD:-Debug} $run --verbose --no-cleanup
                set +x
            fi
        done
    )
}
regressp() {
    r="$*"
    [[ $* ]] || r=$(regressdirs ~/c/fresh/xmt)
    echo racer=$(echo ~/c/fresh/xmt) BUILD=${BUILD:-Debug} regress1p $r
    regress1p $r
}

fregressp() {
    racer=$(echo ~/c/fresh/xmt) regressp "$@"
}

regress1() {
    local dir
    for dir in "$@"; do
        (
            cd ${racer:-$xmtx}/RegressionTests/$dir
            if [[ -x ./run.pl ]] ; then
                BUILD=${BUILD:-Debug} ./run.pl --verbose --no-cleanup
            fi
        )
    done
}
regress2() {
    regress1 Hypergraph2
}
skip1blank() {
    perl -ne '++$have if /^\S/; $blank=/^\s*$/; print unless $have == 1 && $blank' "$@"
}
lwlmcat() {
    out=${2:-${1%.lwlm}.arpa}
    [[ -f $out ]] || LangModel -sa2text -lm-in $1 -lm-out "$out" -tmp ${TMPDIR:-/tmp}
    cat $out | skip1blank
}
pyprof() {
    #easy_install -U memory_profiler
    python -m memory_profiler -l -v "$@"
}
macpageoff() {
    sudo launchctl unload -w /System/Library/LaunchDaemons/com.apple.dynamic_pager.plist
}
macpageon() {
    sudo launchctl load -wF /System/Library/LaunchDaemons/com.apple.dynamic_pager.plist
}

#file arg comes first! then cmd,then args
javaperf="-XX:+UseNUMA -XX:+TieredCompilation -server -XX:+DoEscapeAnalysis"
rrpath() {
    (set -e
     f=$(whichreal "$1")
     r=${2:-'$ORIGIN'}
     echo adding rpath "$r" to $f
     [ -f "$f" ]
     addrpathb "$f" "$r" "$f".rpath && mv "$f" {,.rpath.orig} && mv "$f" {.rpath,}
    )
}
svnchmodx() {
    chmod +x "$@"
    svn propset svn:executable '' "$@"
    svn propset eol-style native "$@"
}
lc() {
    catz "$@" | tr A-Z a-z
}
cpextlib() {
    (
        local to=${1:-~/pub/lib}
        mkdir -p $to
        if [[ $lwarch = Apple ]] ; then
            dylib=dylib
        else
            dylib=so
        fi
        for f in `ls ${SDL_EXTERNALS_PATH}/libraries/*/lib/*$dylib* | egrep -v 'liblbfgs-b2a54a9-float|hadoop-0.20.2-cdh3u3|tbb-4.2.3'`; do
            ls -l $f
            cp -a $f $to
        done
    )
}
lnxmtlib() {
    d=${1?arg1: dest dir}
    mkdir -p $d
    for f in `find $xmtext/libraries -maxdepth 1 -name libd -o -name '*.so'`; do
        echo $f
        force=1 lnreal $f $d/
    done
}
countvocab() {
    catz "$@" | tr -s ' \t' '\n' | sort | uniq -c
}
nsincludes() {
    perl -ne '
$ns=1 if /namespace/;
if (/^\s*#\s*include/ && $ns) {
print;
print $ARGV,"\n";
exit;
}
' "$1"
}
nsinclude() {
    for f in "$@"; do
        nsincludes "$f"
    done
}
gitpullsub() {
    #-q
    git submodule foreach git pull -q origin master
}
espullsub() {
    #-q
    cd ~/.emacs.d
    git submodule foreach git pull -q origin master
}
sitelisp() {
    cd ~/.emacs.d
    git submodule add git://github.com/${2:-emacsmirror}/$1.git site-lisp/$1
}
emcom() {
    cd ~/.emacs.d
    git add -v *.el defuns/*.el site-lisp/*.el
    git add -v snippets/*/*.yasnippet
    gcom "$@"
}
ecomsub() {
    cd ~/.emacs.d
    git pull
    cd site-lisp
    gitpushsub *
}
empull() {
    (
        cd ~/.emacs.d
        git pull
        cd site-lisp
        gitpullsub *
    )
}
uext() {
    cd $xmtext
    svn update
}
comdocs() {
    cd $xmtx/docs
    svn change=md *.md
    change=md commit=1 revx "$@"
    makedocs
    svn commit *.pdf -m 'docs: pandoc to pdf'
}
makedocs() {
    cd $xmtx/docs
    . make.sh
}
roundshared() {
    cp $xmtxs/graehl/shared/* ~/t/graehl/shared/; relnshared
}
tagtracks() {
    for i in `seq -w 1 99`; do echo $i; id3tag --album='Top 100 Best Techno Vol.1' --track=$i $i-*.mp3 ; done
}
tagalbum() {
    for f in *.mp3; do echo $i; id3tag --album="$*" "$f" ; done
}
svnmime() {
    local type=$1
    shift
    svn propset svn:mime-type $type "$@"
}
svnpdf() {
    svnmime application/pdf *.pdf
}
xhostc() {
    xhost +192.168.131.99
}
showdefines() {
    ${CC:-g++} -dM -E "${1:--}" < /dev/null
}
showcpp() {
    (
        # https://github.com/h8liu/mcpp
        local outdir=${outcpp:-~/tmp}
        local os=
        for f in "$@"; do
            local o=$outdir/$(basename $f).pp.cpp
            mkdir -p `dirname $o`
            os+=" $o"
            rm -f $o
            f=$(realpath $f)
            #pushd /Users/graehl/x/Debug/Hypergraph &&
            local xmtlib=$xmtextbase/FC12/libraries
            boostminor=11
            CPP=${CPP:-mcpp}
            if [[ `basename $CPP` != mcpp ]] ; then
                earg='-E'
            else
                earg=-'+ -V201400L -I-'
            fi
            set -x
            $CPP -DGRAEHL_G1_MAIN -DHAVE_CXX_STDHEADERS -DBOOST_ALL_NO_LIB -DBOOST_LEXICAL_CAST_ASSUME_C_LOCALE -DBOOST_TEST_DYN_LINK -DCMPH_NOISY_LM -DHAVE_CRYPTOPP -DHAVE_CXX_STDHEADERS -DHAVE_HADDOP -DHAVE_ICU -DHAVE_KENLM -DHAVE_LIBLINEAR -DHAVE_OPENFST -DHAVE_SRILM -DHAVE_SVMTOOL -DHAVE_ZLIB -DHAVE_ZMQ -DMAX_LMS=4 -DTIXML_USE_TICPP -DUINT64_DIFFERENT_FROM_SIZE_T=1 -DU_HAVE_STD_STRING=1 -DXMT_64=1 -DXMT_ASSERT_THREAD_SPECIFIC=1 -DXMT_FLOAT=32 -DXMT_MAX_NGRAM_ORDER=5 -DXMT_MEMSTATS=1 -DXMT_OBJECT_COUNT=1 -DXMT_VALGRIND=1 -DYAML_CPP_0_5 -I$xmtx/sdl -I$xmtx         -I$xmtlibshared/sparsehash-c11/include         -I$xmtlibshared/utf8         -I$xmtlibshared/cryptopp-5.6.2/include         $earg -o "$o"  "$f" ; edit $o
            #-x c++ -std=c++11
            if false ; then
                -I$xmtlibshared/zeromq-3.2.2.2-1/include -I$xmtlib/boost_1_${boostminor}_0/include -I$xmtlib/ -I$xmtlib/lexertl-2012-07-26 -I$xmtlib/log4cxx-0.10.0/include -I$xmtlib/icu-4.8/include -I/Users/graehl/x/sdl/.. -I$xmtlib/BerkeleyDB.4.3/include -I/usr/local/include -I$xmtlib/openfst-1.2.10/src -I$xmtlib/openfst-1.2.10/src/include -I.                                                  -I $xmtlib/db-5.3.15                                                 -I $xmtlib/yaml-cpp-0.3.0-newapi/include                                                 -I$xmtlibshared/utf8 -I$xmtlibshared/openfst-1.2.10/src -I$xmtlibshared/tinyxmlcpp-2.5.4/include
            fi
            popd
        done
        echo
        echo $os
        preview $os
    )
}
pdfsettings() {
    # MS fonts - distributed with the free Powerpoint 2007 Viewer or the Microsoft Office Compatibility Pack
    #    mainfont=Cambria

    #no effect?
    margin=2cm
    hmargin=2cm
    vmargin=2cm

    #effect:
    paper=letter
    fontsize=11pt
    mainfont=${mainfont:-Constantia}
    sansfont=Corbel
    monofont=Consolas
    documentclass=scrartcl
    paper=letter

}
panda() {
    margin=1cm
    hmargin=1cm
    vmargin=1cm

    #    mainfont=Cambria
    mainfont=Constantia
    sansfont=Corbel
    monofont=Consolas
    fontsize=11pt
    documentclass=article
    #documentclass=scrartcl
    paper=letter


    if [[ $toc = 1 ]]; then
        tocparams="--toc"
    fi


    (
        for docmd in "$@"
        do
            set -e
            doc=${docmd%.md}
            doc=${doc%.}
            parentPage=XMT
            if [[ ${doc%-userdoc} != $doc ]] ; then
                parentPage=$userdocParent
            fi
            title=$doc
            tex=xelatex
            tf="$doc.tex"
            sources="$doc.md"
            params="--webtex --latex-engine=$tex --self-contained"
            html="$doc.html"
            echo generating $html for browser
            echo pandoc $params $tocparams -t html5 -o "$html" "$sources"
            pandoc $params $tocparams -t html5 -o "$html" "$sources"
            if [[ $epub = 1 ]] ; then
                local coverarg
                local cover=${cover:-cover.png}
                if [[ -f $cover ]] ; then
                    coverarg=--epub-cover-image=$cover
                fi
                pandoc -S $coverarg -o "$doc.epub" "$sources"
            fi
            if [[ $pdf = 1 ]] ; then
                fpdf="$doc.pdf"
                rm -f "$fpdf"
                echo generating $tf
                latextemplate=${latextemplate:-$(echo ~/.pandoc/template/xetex.template)}
                local ltarg
                if [[ -f $latextemplate ]] ; then
                    ltarg="--template=$latextemplate"
                fi
                pandoc $params $tocparams -t latex -w latex -o "$tf" $ltarg --listings -V mainfont="${mainfont:-Constantia}" -V sansfont="${sansfont:-Corbel}" -V monofont="${monofont:-Consolas}"  -V fontsize=${fontsize:-11pt} -V documentclass=${documentclass:-article} -V geometry=${paper:-letter}paper -V geometry=margin=${margin:-2cm} -V geometry=${orientation:-portrait} -V geometry=footskip=${footskip:-20pt} "$sources"
                (
                    echo generating $fpdf
                    #TODO: fix all latex errors. xelatex is crashing on RegexTokenizer-userdoc
                    xelatex -interaction=nonstopmode "$tf"
                    if [[ $toc = 1 ]] ; then
                        xelatex -interaction=nonstopmode "$tf"
                    fi
                ) || echo latex exit code $?
            fi
            # I tried all of these in an attempt to adjust the pdf margins more aggressively,but neither did anything beyond just setting margin:

            # -V geometry=margin=$margin -V margin=$margin -V hmargin=$hmargin -V vmargin=$vmargin -V geometry=margin=$margin -V geometry=vmargin=$vmargin -V geometry=hmargin=$hmargin -V geometry=bmargin=$vmargin -V geometry=tmargin=$vmargin -V geometry=lmargin=$hmargin -V geometry=rmargin=$hmargin
            if [[ $open = 1 ]] ; then
                if [[ $pdf = 1 ]] ; then
                    open $doc.pdf
                else
                    open $doc.html
                fi
            fi
        done
    ) || echo panda exit code $?
}
pandcrap() {
    local os=$1
    [[ $os = html ]] && os=html5
    local pdfargs=
    if [[ $os = pdf ]] ; then
        t=latex
        pdfsettings
    fi
    local f
    for f in "$@"; do
        local g=${f%.txt}
        g=${g%.md}
        g=${g%.}
        local o=$g.$os
        (
            texpath
            set -e
            # --read=markdown
            if [[ $os = pdf ]] ; then
                latextemplate=${latextemplate:-$(echo ~/.pandoc/template/xetex.template)}
                pandoc --webtex $f -o $o --latex-engine=xelatex --self-contained --template=$latextemplate --listings -V mainfont="${mainfont:-Constantia}" -V sansfont="${sansfont:-Corbel}" -V monofont="${monofont:-Consolas}"  -V fontsize=${fontsize:-11pt} -V documentclass=${documentclass:-article} -V geometry=${paper:-letter}paper -V geometry=margin=${margin:-2cm} -V geometry=${orientation:-portrait} -V geometry=footskip=${footskip:-20pt}
            else
                pandoc $f -o $o --latex-engine=xelatex --self-contained
                #--webtex
            fi
            if [[ "$open" ]] ; then open $o; fi
        )
        echo $o
    done
}
pandall() {
    pdf=1 epub=1 panda "$@"
}
pandcrapall() {
    local o=`pandcrap html "$@"`
    pandcrap latex "$@"
    #pand epub "$@"
    #pand mediawiki "$@"
    pandcrap pdf "$@"
    local f
    for f in $o; do
        open $f
    done
}
texpath() {
    for tld in `ls /usr/local/texlive/`; do
        if [[ -d $tld/bin ]] ; then
            export PATH=$tld/bin/x86_64-darwin/:$tld/bin/universal-darwin/:$PATH
            return
        fi
    done
}
sshut() {
    local dnsarg=
    if [ "$dns "] ; then
        dnsarg="--dns"
    fi
    ~/sshuttle/sshuttle $dnsarg -vvr graehl@ceto.languageweaver.com 0/0
}
dotshow() {
    local f=${1:-o}
    dot $f -Tpng -o $f.png && open $f.png
}
fsmshow() {
    local f=$1
    require_file $f
    local g=$f.dot
    HgFsmDraw "$f" > $g
    dotshow $g
}
sceto() {
    ssh -p 4640 ceto.languageweaver.com "$@"
}
bsceto() {
    homer Hypergraph
    bceto
}
bceto() {
    sceto ". a;linx Debug"
}
homer() {
    for p in "$@"; do
        scp -P 4640 -r ~/c/xmt/$p/*pp graehl@ceto.languageweaver.com:c/xmt/xmt/$p
    done
}
homers() {
    homer Hypergraph
    homer LWUtil
}
view() {
    if [[ $lwarch = Apple ]] ; then
        open "$@"
    else
        xzgv "$@"
    fi
}

replacegrep() {
    local from=$1
    local to=$2
    if [[ $to ]] ; then
        shift
        shift
        echo2 "$from = $to in $*"
        local subs=`mktemp subs.XXXXXX`
        cat > $subs <<EOF
$from	$to
EOF
        substigrep $subs --literal "$from" "$@"
        rm $subs
    fi
}

substigrep() {
    local repl=$1
    (set -e
     require_file "$repl"
     shift
     substi $repl $(ag -l "$@")
    )
}
substi() {
    (set -e
     local tr=$1
     shift
     substarg=" --inplace --tr $tr"
     if [[ $literal ]] ; then
         substarg+=" --noeregexp --nosubstregexp"
     fi
     if [[ $wholeword ]] ; then
         substarg+=" --wholeword"
     fi
     if [[ $endsword ]] ; then
         substarg+=" --endsword"
     fi
     if [[ $startsword ]] ; then
         substarg+=" --startsword"
     fi
     if [[ $subst ]] ; then
         substarg+=" -substreg"
     fi
     echo subst.pl $substarg
     if [ "$*" ] ; then
         if ! [[ ${nodryrun:-} ]] ; then
             subst.pl --dryrun $substarg "$@"
             echo
         fi
         cat $tr
         echo
         echo ctrl-c to abort
         if ! [[ $preview ]] ; then
             sleep 3
             subst.pl $substarg "$@"
         fi
     fi
    )
}
substigrepq() {
    substi $1 `ag -l $(basename $1)`
}
substrac() {
    (
        pushd $xmtxs
        substi "$@" $(ack --cpp -f)
    )
}
substyml() {
    (
        substi "$@" $(findyaml)
    )
}
substxml() {
    (
        substi "$@" $(ag -g '\.xml$')
    )
}
substcmake() {
    (
        substi "$@" $(findcmake)
    )
}
gitchange() {
    git --no-pager log --format="%ai %aN %n%n%x09* %s%d%n"
}
mflist() {
    perl -e 'while (<>) { if (/<td align="left">([^<]+)</td>/) { $n=$1; $l=1; } else { $l=0; } }'
}
showlib() {
    (
        export DYLD_PRINT_LIBRARIES=1
        "$@"
    )
}
otoollibs() {
    perl -e '
$f=shift;
$_=`otool -L $f`;
while (m|^\s*(\S+) |gm) {
print "$1 " unless $1 =~ m|^/usr/lib/|;
}
print "\n";
' $1
}
libmn() {
    local m=${1:-48}
    local n=${2:-1}
    local s=$m.$n.dylib
    for f in *.$s; do local g=${f%.$s};
                      local ss="$g.dylib $g.$m.dylib"
                      svn rm $ss
                      ln -sf $f $g.dylib
                      ln -sf $f $g.$m.dylib
                      svn add $ss
    done
}

hgrep() {
    history | grep "$@"
}
brewgdb() {
    brew install https://github.com/adamv/homebrew-alt/raw/master/duplicates/gdb.rb
}
gdbtool () {
    emacsc --eval "(gud-gdb \"gdb --annotate=3; --args $*\")"
}
svnrespecial() {
    for f in "$@"; do
        rm $f
        svn update $f
    done
}
fontsmooth() {
    defaults -currentHost write -globalDomain AppleFontSmoothing -int "${1:-2}"
}
locp=${locp:9922}
tun1() {
    #eg tun1 revboard 80 9921
    local p=${3:-${port:-}}
    if ! [[ $p ]] ; then
        p=$locp
        locp=$((locp+1))
    fi
    # ssh -L9922:svn.languageweaver.com:443 -N -t -x pontus.languageweaver.com -p 4640 &
    ssh -L$p:${1:-$chost.languageweaver.com}:${2:-22} -N -t -x ${4:-ceto}.languageweaver.com -p 4640
    set +x
    lp=localhost:$p
    echo lp
    echo $lp
}
tunsvn() {
    tun1 svn 80 $rxsvnp
}
svnroot() {
    svn info | grep ^Repository\ Root | cut -f 3 -d ' '
}
svnswitchurl() {
    svnroot | sed "s/$1/$2/"
}
svnswitchhost() {
    local r=$(svnroot)
    local u=$(svnswitchurl "$@")
    echo "$r => $u"
    if [[ $svnpass ]] ; then
        local sparg="--password $svnpass"
    fi
    if [[ $svnuser ]] ; then
        local suarg="--username graehl"
    fi
    svn relocate $suarg $sparg "$r" "$u"
}
svnswitchlocal() {
    if [[ $local ]] ; then
        svnswitchhost $lwsvnhost $localsvnhost
    else
        svnswitchhost $localsvnhost $lwsvnhost
    fi
}
brewrehead() {
    brew remove "$@"
    safebrew install -d --force -v --use-gcc --HEAD "$@"
}
brewhead() {
    safebrew install -d --force -v --use-gcc --HEAD "$@"
}
nonbrew() {
    find "$1" \! -type l \! -type d -depth 1 | grep -v brew
}
rebrew() {
    brew remove "$@"
    safebrew install "$@"
}
safebrew() {
    local log=/tmp/safebrew.log
    (
        if false ; then
            saves=$(echo /usr/local/{bin,lib,lib/pkgconfig})
            set +e
            for f in $saves; do
                echo mkdir -p $f/unbrew
                mkdir -p $f/unbrew
                set +x
                mv `nonbrew $f` $f/unbrew/
            done
            savebin=/usr/local/bin/unbrew/
            mv /usr/local/bin/auto* $savebin
        fi
        export LDFLAGS+=" -L/usr/local/Cellar/libffi/3.0.9/lib"
        export CPPFLAGS+="-I/usr/local/Cellar/libffi/3.0.9/include"
        export PATH=/usr/local/bin:$PATH
        export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
        #by default it looks there already. just in case!
        unset DYLD_FALLBACK_LIBRARY_PATH
        brew -v "$@" || brew doctor
        set +x
        if false ; then
            savedirs=
            for f in $saves; do
                savedirs+=" $f/unbrew"
                mv -n $f/unbrew/* $f/
            done
            echo $savedirs should be empty:
            ls -l $savedirs
            echo empty,i hoope
        fi
    ) 2>&1 | tee $log
    echo cat $log
}
awksub() {
    local s=$1
    shift
    local r=$1
    shift
    awk ' {gsub(/'"$s"'/,"'"$r"'")};1' "$@"
}
awki() {
    local f=$1
    shift
    cmdi "$f" awk "$@"
}
cmdi() {
    local f=$1
    shift
    local cmd=$1
    shift
    ( mv $f $f~ && $cmd "$@" > $f) < $f
    diff -b -B -- $f~ $f
}

gitsubuntracked() {
    awki .gitmodules ' {print}; /^.submodule /{ print "\tignore = untracked" }'
}

gitpush1sub() {
    local p=$1
    shift
    local msg="$*"
    local sub=$(basename $p)
    echo git submodule push $p ...
    if ! [[ -d $p ]] ; then
        echo gitsubpush skipping non-directory $p ...
    else
        sub=${sub%/}
        (set -e
         pushd $p
         gcom "$msg"
         cd ..
         git commit $sub -m "$msg"
         git push $sub -m "$msg"
        )
    fi
}
gitpushsub() {
    forall gitpushsub1 "$@"
}
gitpullsub1() {
    local p=$1
    shift
    local msg="$*"
    echo git submodule push-pull $p ...
    if ! [[ -d $p ]] ; then
        echo gitpullsub skipping non-directory $p ...
    else
        (
            set -e
            pushd $p
            local sub=$(basename $p)
            sub=${sub%/}
            local gitbranch=${gitbranch:-${2:-master}}
            banner pulling submodule $p ....
            showvars_required sub gitbranch
            ! grep graehl .git/config || gcom "$msg"
            git checkout $gitbranch
            git pull
            cd ..
            git add `basename $p`
            git commit $sub -m "pulled $p - $msg"
            [[ $nopush ]] || git push
            banner DONE pulling submodule $p.
        )
    fi
}
gitpullsub() {
    forall gitpullsub1 "$@"
}
dbgemacs() {
    ${emacs:-appemacs} --debug-init
}
freshemacs() {
    git clone --recursive git@github.com:graehl/.emacs.d.git
}
urlup() {
    #destroys hostname if you go to far
    perl -e '$_=shift;s|/[^/]+/?$||;print "$_";' "$*"
}
svnurl() {
    svn info | awk '/^URL: /{print $2}'
}
svngetrev() {
    svn ls -v "$@" | awk ' {print $1}'
}
svnprev() {
    #co named revision of file in current dir
    local f=${1?-arg1: file in cdir}
    local r=$2
    [[ $r ]] || r=PREV
    local svnu=`svnurl`
    #local svnp=`urlup $svnu`
    local svnp="$svnu"
    local d=`basename $svnu`
    local dco="co-$d-r$r"
    set -e
    showvars_required svnp dco f r
    require_file $f
    local rev=`svngetrev -r "$r" $f`
    echo ""$f at revision $rev - latest as of requested $r:""
    ! [[ -d $dco ]] || rm -rf $dco/
    svn checkout -r "$rev" $svnp $dco --depth empty || error "svn checkout -r '$rev' '$svnp' $dco --depth empty" || return
    pushd $dco
    svn update -r "$rev" $f || error "svn update $f" || return
    popd
    local dest="$f@$r"
    ln -sf $dco/$f $dest
    diff -s -u "$dest" "$f"
    echo diff -s -u "$dest" "$f"
    echo
    echo $dest
}
svnprevs() {
    local rev=$1
    shift
    for f in "$@"; do
        (svnprev $f $rev)
    done
}
pids() {
    grep=${grep:-/usr/bin/grep}
    ps -ax | awk '/'$1'/ && !/awk/{print $1}'
}
gituntrack() {
    git update-index --assume-unchanged "$@"
}
svntaglog() {
    local proj=${1:-carmel}
    local spath=${2:-"https://nlg0.isi.edu/svn/sbmt/tags/$proj"}
    showvars_required proj spath
    svn log -v -q --stop-on-copy $spath
}

svntagr() {
    svntaglog "$@" | grep " A"
}

gitsub() {
    git pull
    git submodule update --init --recursive "$@"
    git pull
}
gitco() {
    git clone --recursive "$@"
}
gitchanged() {
    git status -uno
}
gcom() {
    gitchanged
    git commit -a -m "$*"
    git push -v || true
}
scom() {
    svn commit -m "$*"
}
gscom() {
    gcom "$@"
    scom "$@"
}
acksed() {
    echo going to replace $1 by $2 - ctrl-c fast!
    (set -e
     ack -l $1
     sleep 3
     ack -l --print0 --text $1 | xargs -0 -n 1 sed -i -e "s/$1/$2/g"
     ack $1l

    )
}
retok() {
    cd $xmtxs/Tokenizer
    ./test.sh "$@"
}
hgdot() {
    local m=${3:-${1%.gz}}
    HgDraw $1 > $m.dot
    doto $m.dot $2
}
doto() {
    local t=${2:-pdf}
    local o=${3:-${1%.dot}}
    dot -Tpdf -o$o.$t $1 && open $o.$t
}

dbgemacs() {
    cd ~/bin/Emacs.contents
    MacOS/Emacs --debug-init "$@"
}
lnshared() {
    forall lnshared1 "$@"
}
lnshared1() {
    local s=~/t/graehl/shared
    local f=$s/$1
    local d=$xmtxs/graehl/shared
    local g=$d/$1

    if ! [ -r $f ] ; then
        cp $g $f
    fi
    if [ -r $f ] ; then
        if ! [ -r $g ] ; then
            ln $f $g
        fi
        if diff -u $g $f || [[ $force ]] ; then
            rm -f $g
            ln $f $g
        fi
    fi

    (cd $s; svn add "$1")
    #(cd $d; svn add "$1")
}
racershared1() {
    local s=~/t/graehl/shared
    local f=$s/$1
    local d=$xmtxs/graehl/shared
    local g=$d/$1
    if [ -f $g ] ; then
        if [ "$force" ] ; then
            diff -u -w $f $g
            rm -f $f
        fi
        ln $g $f
    fi
}
usedshared() {
    (cd $xmtxs/graehl/shared/; ls *.?pp)
}
diffshared1() {
    local s=~/t/graehl/shared/
    local f=$s/"$1"
    local d=$xmtxs/graehl/shared/
    diff -u -w $f $d/$1
}
diffshared() {
    forall diffshared1 $(usedshared)
}
relnshared() {
    lnshared $(usedshared)
}
lnhgforce() {
    for b in "$@"; do
        for t in $b egdb$b gdb$b vg$b; do
            echo $t
            ln -sf $xmtx/run.sh ~/bin/$t
            ln -sf $xmtx/run.sh ~/bin/${t}Asan
            ln -sf $xmtx/run.sh ~/bin/${t}Clang
            ln -sf $xmtx/run.sh ~/bin/${t}Release
            ln -sf $xmtx/run.sh ~/bin/${t}RelWithDebInfo
        done
    done
}
lnhg1() {
    local branch=${branch:-Debug}
    local d=$xmtx/$branch
    local pre=$1
    shift
    rpathsh=$xmtlib/rpath.sh
    echo "$d" > ~/bin/racerbuild.dirname
    for f in $d/$pre*; do
        [[ -x $pathsh ]] && $rpathsh $f 1
        if [[ ${f%.log} = $f ]] ; then
            basename $f
            local b=`basename $f`
            lnhgforce $b
        fi
    done
}
rpathhg() {
    rpathsh=$xmtx/3rdParty/Apple/rpath.sh
    local b=${1:-Debug}
    find $xmtx/$b -type f -exec $rpathsh {} \;
}
lnhg() {
    lnhg1 "$1"/"$1"
}
lntest() {
    lnhg1 "$1"/Test
}
lnhgs() {
    (
        branch=${1:-Debug}
        lnhg1 ProcessYAML/ProcessYAML
        lnhg1 ProcessYAML/ProcessYAML
        lnhg1 ProcessYAML/Test
        lnhg1 RuleDumper/RuleDumper
        lnhg1 SyntaxBased/Test
        lnhg1 LanguageModel/LMQuery/LMQuery
        lnhg1 LWUtil/Test
        lnhg1 Grammar/Test
        lnhg1 FeatureBot/Test
        lnhg1 Utf8Normalize/Utf8Normalize
        lnhg1 BasicShell/xmt
        lnhg1 OCRC/OCRC
        lnhg1 AutoRule/Test
        lnhg1 RuleSerializer/Test
        lnhg1 Hypergraph/Test
        lnhg1 Config/Test
        lnhg1 Hypergraph/Hg
        lnhg1 StatisticalTokenizer/StatisticalTokenizer
        lnhg1 LmCapitalize/LmCapitalize
        lnhg1 ValidateConfig/ValidateConfig
        lntest Preorderer
        lntest SearchReplace
    )
}
rebuildc() {
    (set -e
     s2c
     ssh $chost ". ~/a;HYPERGRAPH_DBG=${HYPERGRAPH_DBG:-$verbose} tests=${tests:-Hypergraph/Empty} xmtm Debug"
    )
}
ackc() {
    ack --ignore-dir=3rdParty --pager="less -R" --cpp "$@"
}

horse=~/c/horse
horsem() {
    (
        export LW64=1
        export LWB_JOBS=5
        cd $horse
        perl GenerateSolution.pl
        make
    )
}
sa2c() {
    s2c
    (cd
     for d in x/sdl/ ; do
         sync2 $chost $d
     done
    )
}
s2c() {
    #elisp x/3rdParty
    (cd ~
     set -e
     c-sync u .gitconfig
     c-to x/run.sh
     c-to x/xmtpath.sh
     #chost=ceto c-sync u g script bugs .gitconfig
    )
}
syncc() {
    sync2 $chost "$@"
}
svndry() {
    svn merge --dry-run -r BASE:HEAD ${1:-.}
}
sconsd() {
    scons -Q --debug=presub "$@"
}

prependld() {
    if [[ -d $1 ]] ; then
        if [[ $lwarch = Apple ]] ; then
            DYLD_FALLBACK_LIBRARY_PATH=$1:$DYLD_FALLBACK_LIBRARY_PATH
            export DYLD_FALLBACK_LIBRARY_PATH=${DYLD_FALLBACK_LIBRARY_PATH#:}
        else
            LD_LIBRARY_PATH=$1:$LD_LIBRARY_PATH
            export LD_LIBRARY_PATH=${LD_LIBRARY_PATH#:}
        fi
    fi
}
#sudo gem install git_remote_branch --include-dependencies - gives the nice 'grb' git remote branch cmd
#see aliases in .gitconfig #git what st ci co br df dc lg lol lola ls info ign
addld() {
    if [[ $lwarch = Apple ]] ; then
        if ! fgrep -q "$1" <<< "$DYLD_FALLBACK_LIBRARY_PATH" ; then
            prependld "$@"
            export DYLD_FALLBACK_LIBRARY_PATH=$1:$DYLD_FALLBACK_LIBRARY_PATH
        else
            true || echo2 "$1 already in DYLD_FALLBACK_LIBRARY_PATH"
        fi
    else
        if ! fgrep -q "$1" <<< "$LD_LIBRARY_PATH" ; then
            prependld "$@"
        else
            true || echo2 "$1 already in LD_LIBRARY_PATH"
        fi
    fi
}

showlds() {
    if [[ $lwarch = Apple ]] ; then
        env | grep DYLD_ 2>&1
    else
        env | grep LD_LIBRARY_PATH 2>&1
    fi
}

failed() {
    racb
    testlogs $(find . -type d -maxdepth 2)
}
testlogs() {
    for f in "$@"; do
        local g=$f/Testing/Temporary/LastTestsFailed.log
        if [ -f $g ] ; then
            tailn=30 preview $g
        fi
    done
}
corac() {
    svn co http://svn.languageweaver.com/svn/repos2/cme/trunk/xmt
    cd racerx
}
#sets cmarg
racb() {
    setjavahome
    build=${build:-Release}
    build=${1:-$build}
    shift || true
    if [[ ${debug:-} = 1 ]] ; then
        build=Debug
    fi
    xmtbuild=$xmtx/$build
    export SDL_EXTERNALS_PATH=$xmtext
    mkdir -p $xmtbuild
    cd $xmtbuild
    local buildtyped=$build
    if [[ ${build#Debug} != $build ]] ; then
        buildtyped=Debug
    elif [[ ${build#Release} != $build ]] ; then
        buildtyped=Release
    fi

    local allarg=
    local ALLHGBINS=1
    if [[ ${ALLHGBINS:-} ]] ; then
        allarg="-DAllHgBins=1"
    fi
    if [[ ${nohg:-} ]] ; then
        allarg=
    fi
    cmarg="-DCMAKE_BUILD_TYPE=${buildtype:-$buildtyped} ${allarg:-} $*"
    if [[ $HOST = $graehlmac ]] || [[ $OS = Darwin ]] ; then
        cmarg+=" -DLOG4CXX_ROOT=/usr/local"
    fi
    CMAKEARGS=${CMAKEARGS:- -DCMAKE_COLOR_MAKEFILE=OFF -DCMAKE_VERBOSE_MAKEFILE=OFF}
    cmarg+=" $CMAKEARGS"
    if true ; then
        if [[ $CC ]] ; then
            cmarg+="  -DCMAKE_C_COMPILER=$CC"
        fi
        if [[ $CXX ]] ; then
            cmarg+="  -DCMAKE_CXX_COMPILER=$CXX"
        fi
    fi
}
racd() {
    cd $xmtx
    svn diff -b
}
urac() {
    (
        cd $xmtx
        if ! [ "$noup" ] ; then svn update ; fi
    )
}
crac() {
    pushd $xmtx
    local dryc=
    if [[ ${dryrun:-} ]] ; then
        dryc=echo
    fi
    local carg=
    if [[ $change ]] ; then carg="--changelist $change"; fi
    $dryc svn commit $carg -m "$*"
    popd
}
commx() {
    crac "$@"
}
svndifflines() {
    diffcontext=0
    echo changed wc -l:
    svndiff | wc -l
}
svndiff() {
    local diffcontext=${diffcontext:-8}
    svn diff --diff-cmd diff --extensions "-U $diffcontext -b"
}
svndifflog() {
    local lastlog=${1:-5}
    shift
    svn log -l $lastlog --incremental "$@"
    svndiff
}
drac() {
    cd $xmtx/3rdparty
    banner 3rdparty svn
    svndifflog 2 "$@"
    echo; echo
    banner racerx svn
    cd $xmtx
    svndifflog 5 "$@"
}
runc() {
    ssh $chost "$*"
}
enutf8=en_US.UTF-8
c-l() {
    ssh $chost "$@"
}
gl() {
    chost=c-graehl c-l "$@"
}
jl() {
    chost=$jhost c-l "$@"
}
kl() {
    chost=c-skohli c-l "$@"
}
ml() {
    chost=c-mdreyer c-l "$@"
}
sc() {
    ssh $chost "$@"
    #        LANG=$enutf8 mosh --server="LANG=$enutf8 mosh-server" $chost "$@"
}
topon() {
    thostp $phost "$@"
}
fromc() {
    fromhost $chost "$@"
}
clocal() {
    (
        set -e
        if [ "$1" ] ; then cd $1* ; fi
        ./configure --prefix=/usr/local "$@" && make -j && sudo make install
    )
}
msudo() {
    make -j 4 && sudo make install
}
cgnu() {
    for f in "$@"; do
        g=$f-${ver:-latest}.tar.${bzip:-bz2}
        (set -e
         curl -O http://ftp.gnu.org/gnu/$f/$g
         tarxzf $g
        )
    done
}
emacsapp=/Applications/Emacs.app/Contents/MacOS/
emacssrv=$emacsapp/Emacs
emacsc() {
    if [ "$*" ] ; then
        if [[ -x ~/bin/eclient.sh ]] ; then
            ~/bin/eclient.sh "$@"
        else
            $emacsapp/bin/emacsclient -a $emacssrv "$@"
        fi
    else
        $emacssrv &
    fi
}

svnln() {
    forall svnln1 "$@"
}
svnln1() {
    ln -s $1 .
    svn add $(basename $1)
}


hemacs() {
    nohup ssh -X hpc-login2.usc.edu 'bash --login -c emacs' &
}
to3() {
    #-f ws_comma
    2to3 -f all -f idioms -f set_literal "$@"
}
to2() {
    #-f set_literal #python 2.7
    2to3 -f idioms -f apply -f except -f ne -f paren -f raise -f sys_exc -f tuple_params -f xreadlines -f types "$@"
}

ehpc() {
    local cdir=`pwd`
    local lodir=`relhome $cdir`
    ssh $HPCHOST -l graehl ". .bashrc;. t/graehl/util/bashlib.sh . t/graehl/util/aliases.sh;cd $lodir && $*"
}
reltohpc() {
    local cdir=`pwd`
    local lodir=`relhome $cdir`
    ssh $HPCHOST -l graehl "mkdir -p $lodir"
    scp "$@" graehl@$HPCHOST:$lodir
}

splitcomma() {
    clines "$@"
}

getwt() {
    local f="$1"
    shift
    clines "$@" | fgrep -- "$f"
}
imira() {
    local hm=$HOME/hiero-mira
    local m=$HOME/mira
    rm -rf $hm
    mkdir -p $hm
    local b=/home/nlg-03/mt-apps/hiero-mira/20110804
    cp -pR $b/* $hm/
    qmira
}
qmira() {
    local hm=$HOME/hiero-mira
    local m=$HOME/mira
    for f in $m/{genhyps,log,trainer,sbmt_decoder}.py; do
        cp -p $f $hm/
    done
    (
        cd
        cd $hm
        pycheckers {genhyps,log,trainer,sbmt_decoder}.py
    )

}
vgnorm() {
    perl -i~ -pe 's|^==\d+== ?||;s|\b0x[0-9A-F]+||g' "$@"
}

pycheckers() {
    (
        pycheckerarg=${pycheckerarg:- --stdlib --limit=1000}
        [ -f setup.sh ] && . setup.sh
        for f in "$@" ; do
            pychecker $pycheckerarg $f 2>&1 | grep -v 'Warnings...' | grep -v 'Processing module ' | tee $f.check
        done
    )
}
#simpler than pychecker
pycheck() {
    python -c "import ${1%.py}"
}

cmpy() {
    python setup.py ${target:-install} --home $FIRST_PREFIX
}
cmpyh() {
    python setup.py ${target:-install} --home ~
}
backupsbmt() {
    mkdir -p $1
    #--exclude Makefile\* --exclude auto\* --exclude config\*
    #--size-only
    rsync --modify-window=1 --verbose --max-size=500K --cvs-exclude --exclude '*~' --exclude libtool --exclude .deps --exclude \*.Po --exclude \*.la --exclude hpc\* --exclude tmp --exclude .libs --exclude aclocal.m4 -a $SBMT_TRUNK ${1:-$dev/sbmt.bak}
    #-lprt
    # cp -a $SBMT_TRUNK $dev/sbmt.bak
}
build_sbmt_variant() {
    #target=check
    variant=debug boostsbmt "$@"
}

boostsbmt() {
    (
        set -e
        local h=${host:-$HOST}
        nproc_default=5
        if [[ "$h" = cage ]] ; then
            nproc_default=7
        fi
        nproc=${nproc:-$nproc_default}
        variant=${variant:-$releasevariant}
        local linking="link=static"
        linking=
        branch=${branch:-trunk}
        trunkdir=${trunkdir:-$SBMT_BASE/$branch}
        [ -d $trunkdir ] || trunkdir=$HOME/t
        showvars_required branch trunkdir
        pushd $trunkdir
        mkdir -p $h
        local prefix=${prefix:-$FIRST_PREFIX}
        [ "$utility" ] && target="utilities//$utility"
        target=${target:-install-pipeline}
        #
        #--boost-location=$BOOST_SRCDIR
        #-d 4
        local barg boostdir
        local builddir=${build:-$h}
        if [[ $boost ]] ; then
            boostdir=$HOME/src/boost_$boost
            builddir="${builddir}_boost_$boost"
        fi
        if [[ $boostdir ]] ; then
            [[ -d $boostdir ]] || boostdir=/usr/local
            barg="--boost=$boostdir"
        fi
        execpre=${execpre:-$FIRST_PREFIX}
        if [[ $variant = debug ]] ; then
            execpre+=/debug
        fi
        if [[ $variant = release ]] ; then
            execpre+=/valgrind
        fi
        bjam cflags=-Wno-parentheses cflags=-Wno-deprecated cflags=-Wno-strict-aliasing -j $nproc $target variant=$variant toolset=gcc --build-dir=$builddir --prefix=$prefix --exec-prefix=$execpre $linking $barg "$@" -d+${verbose:-2}
        set +x
        popd
    )
}
tmpsbmt() {
    local tmpdir=/tmp/trunk.graehl.$HOST
    backupsbmt $tmpdir
    trunkdir=$tmpdir/trunk boost=${boost:-1_35_0} boostsbmt "$@"
}
vgsbmt() {
    variant=release tmpsbmt
}
dusort() {
    perl -e 'require "$ENV{HOME}/u/libgraehl.pl";while (<>) {$n=()=m#/#g;push @ {$a[$n]},$_;} for (reverse(@a)) {print sort_by_num(\&first_mega,$_); }' "$@"
}
realwhich() {
    whichreal "$@"
}
check1best() {
    echo 'grep decoder-*.1best :'
    perl1p 'while (<>) { if (/sent=(\d+)/) { $consec=($1==$last)?"(consecutive)":""; $last=$1; log_numbers("sent=$1"); if ($n {$1}++) { count_info_gen("dup consecutive $ARGV [sent=$1 line=$.]");log_numbers("dup $ARGV $consec: $1") } } } END { all_summary() }' decoder-*.1best
    grep -i "bad_alloc" logs/*/decoder.log
    grep -i "succesful parse" logs/*/decoder.log | summarize_num.pl
    grep -i "pushing grammar" logs/*/decoder.log | summarize_num.pl
    # perl1p 'while (<>) { if (/sent=(\d+)/) { next if ($1==$last); $last=$1; log_numbers($1); log_numbers("dup: $1") if $n {$1}++; } } END { all_summary() }' decoder-*.1best
}

blib=$d/bloblib.sh
[ -r $blib ] && . $blib
em() {
    nohup emacs ~/t/sbmt_decoder/include/sbmt/io/logging_macros.hpp ~/t/sblm/sblm_info.hpp &
}
libzpre=/nfs/topaz/graehl/isd/cage/lib
HPF="$USER@$HPCHOST"
browser=${browser:-chrome}
ltd() {
    lt -d "$@"
}
comjam() {
    (
        set +e
        cd ~/t
        mv Jamroot Jamroot.works2; cp Jamroot.svn Jamroot; svn commit -m "$*"; cp Jamroot.works2 Jamroot
    )
}
upjam() {
    (
        set +e
        cd ~/t
        mv Jamroot Jamroot.works2; cp Jamroot.svn Jamroot; svn update; cp Jamroot.works2 Jamroot
    )
}
ld() {
    l -d "$@"
}
flv2aac() {
    local f=${1%.flv}
    set -o pipefail
    local t=`mktemp /tmp/flvatype.XXXXXX`
    local ext
    local codec=copy
    ffmpeg -i "$1" 2>$t || true
    if fgrep -q 'Audio: aac' $t; then
        ext=m4a
    elif fgrep -q 'Audio: mp3' $t; then
        ext=mp3
    else
        ext=wav
        codec=pcm_s16le
    fi
    local mapa
    if [[ $stream ]] ; then
        mapa="-map 0:$stream"
    fi
    if [[ $start ]] ; then
        mapa+=" -ss $start"
    fi
    if [[ $time ]] ; then
        mapa+= " -t $time"
    fi
    local f="$1"
    shift
    ffmpeg -i "$f" -vn $mapa -ac 2 -acodec $codec "$f.$ext" "$@"
}
tohpc() {
    cp2 $HPCHOST "$@"
}
cp2hpc() {
    cp2 $HPCHOST "$@"
}
buildpypy() {
    (set -e
     #http://codespeak.net/pypy/dist/pypy/doc/getting-started-python.html
     opt=jit
     [[ $jit ]] || opt=2
     # mar 2010 opt=2 is default (jit is 32-bit only)
     cd pypy/translator/goal
     CFLAGS="$CFLAGS -m32" ${python:-python2.7} translate.py --opt=$opt -O$opt --cc=gcc targetpypystandalone.py
     ./pypy-c --help
    )
}
safepath() {
    if [[ $ONCYGWIN ]] ; then
        cygpath -m -a "$@"
    else
        abspath "$@"
    fi
}
alias rot13="tr '[A-Za-z]' '[N-ZA-Mn-za-m]'"
cpdir() {
    local dest
    for dest in "$@"; do true; done

    [ -f "$dest" ] && echo file $dest exists && return 1
    mkdir -p "$dest"
    # [ -d "$dest" ] || mkdir "$dest"
    cp "$@"
    set +x
}
jobseq() {
    seq -f %7.0f "$@"
}
lcext() {
    perl -e 'for (@ARGV) { $o=$_; s/(\.[^. ]+)$/lc($1)/e; if ($o ne $_) { print "mv $o $_\n";rename $o,$_ unless $ENV{DEBUG};} }' -- "$@"
}

mkstamps() {
    (
        set -e
        local stamp=${stamp:-`mktemp /tmp/stamp.XXXXXX.tex`}
        stamp=${stamp%.tex}.tex
        mkdir -p "`dirname '$stamp'`"
        local title=${title:-""}
        local papertype=${papertype:-letter}
        local npages=${npages:-5}
        local headpg=${headpg:-R}
        local footpg=${footpg:-L}
        local botmargin=${botmargin:-1.3cm}
        local topmargin=${topmargin:-2cm}
        local leftmargin=${leftmargin:-0.4cm}
        local rightmargin=${rightmargin:-$leftmargin}
        showvars_required papertype npages stamp
        cat > "$stamp" <<EOF
        \documentclass[${papertype}paper,english] {article}
        %,9pt
        \usepackage[T1] {fontenc}
        \usepackage {pslatex} % needed for fonts
        \usepackage[latin1] {inputenc}
        \usepackage {babel}
        \usepackage {fancyhdr}
        %\usepackage {color}
        %\usepackage {transparent}
        \usepackage[left=$leftmargin,top=$botmargin,right=$leftmargin,bottom=$botmargin,${papertype}paper] {geometry}
        \title {$title}
        %\pagestyle {plain}
        \fancyhead {}
        \fancyhead[$headpg] {\thepage}
        \fancyfoot {}
        \fancyfoot[$footpg] {\thepage}
        \renewcommand {\headrulewidth} {0pt}
        \renewcommand {\footrulewidth} {0pt}
        \pagestyle {fancy}
        \begin {document}
        %\maketitle
        %\thispagestyle {empty}
        \begin {center}
        \footnotesize
        %\copyright 2008 Some Institute. Add your copyright message here.
        \end {center}
EOF

        for i in `seq 1 $npages`; do
            # echo '\newpage' >> $stamp
            echo '\mbox {} \newpage'         >> $stamp
        done

        echo '\end {document}' >> $stamp

        local sbase=${stamp%.tex}
        lat2pdf $sbase
        echo $sbase.pdf
    ) | tail -n 1
}
pdfnpages() {
    pdftk "$1" dump_data output - | perl -ne 'print "$1\n" if /^NumberOfPages: (.*)/'
}
pdfstamp() {
    if [ -f "$1" ] ; then
        if [ -f "$2" ] ; then
            pdftk "$1" stamp "$2" output "$3"
        else
            cp "$1" "$3"
        fi
    else
        cp "$2" "$3"
    fi
}
pdfoverlay() {
    # pdftk bot stamp top (1 page at a time)
    (set -e
     local bot=$1
     # bot=`abspath $bot`
     shift
     local top=$1
     # top=`abspath $top`
     shift
     local out=${1:--}
     shift

     local npages=`pdfnpages "$top"`

     botpre=`tmpnam`.bot
     toppre=`tmpnam`.top
     allpre=`tmpnam`.all

     showvars_required npages botpre toppre allpre

     pdftk "$bot" burst output "$botpre.%04d.pdf"
     pdftk "$top" burst output "$toppre.%04d.pdf"

     local pages=
     for i in $(seq -f '%04g' 1 $npages); do
         pdfstamp $botpre.$i.pdf $toppre.$i.pdf $allpre.$i.pdf
         pages+=" $allpre.$i.pdf"
     done
     pdftk $pages cat output "$out"
     ls -l "$out"
     pdfnpages "$out"
    )
}
pdfnumber1() {
    (
        set -e
        local in=$1
        local inNOPDF=${in%.PDF}
        if [ "$in" != "$inNOPDF" ] ; then
            lcext "$in"
            in=$inNOPDF.pdf
        fi
        local inpre=${1%.pdf}
        shift
        in=`abspath "$in"`
        require_file "$in"
        local stamp=${stamp:-`tmpnam`}
        local stampf=$(stamp=$stamp npages=$(pdfnpages "$in") mkstamps)
        local out=${out:-$inpre.N.pdf}
        pdfoverlay "$in" "$stampf" "$out"
        rm "$stamp"*
        echo "$out"
    )
}
pdfnumber() {
    forall pdfnumber1 "$@"
}
exists_1() {
    [ -f "$1" ] || [ -d "$1" ]
}
based_paths_r() {
    local p=$1
    shift
    local d
    local r
    while true; do
        d=`dirname $p`
        r=`based_paths $d "$@"`
        [ -L $p ] || break
        exists_1 && break
    done
    echo "$r"
}
based_paths() {
    local base=$1
    [ "$base" ] || $base+=/
    shift
    local f
    local r=
    for f in "$@"; do
        r+=" $base$f"
    done
}

to8() {
    for f in "$@"; do
        iconv -c --verbose -f ${encfrom:-UTF16} -t ${encto:-UTF8} "$f" -o "$f"
    done
}

getcert() {
    local REMHOST=$1
    local REMPORT=${2:-443}
    echo | openssl s_client -connect ${REMHOST}:${REMPORT} 2>&1 |sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p'
}

pmake() {
    perl Makefile.PL PREFIX=$FIRST_PREFIX
    make && make install
}
unlibz() {
    mv $libzpre/libz.a $libzpre/unlibz.a
}

relibz() {
    mv $libzpre/unlibz.a $libzpre/libz.a
}

export PLOTICUS_PREFABS=$HOME/isd/lib/ploticus/prefabs
HPCHOST=hpc-login2.usc.edu
HPC32=hpc-login1.usc.edu
HPC64=hpc-login2.usc.edu
WFHOME=/lfs/bauhaus/graehl/workflow
WFHPC='~/workflow'


gensym() {
    perl -e '$"=" ";$_="gensym @ARGV";s/\W+/_/g;print;' "$@" `nanotime`
}

currydef() {
    local curryfn=$1
    shift
    eval function $curryfn { "$@" '$*'\; }
}

curry() {
    #fixme: global output variable curryout,since can't print curryout and capture in `` because def would be lost.
    curryout=`gensym $*`
    eval currydef $curryout "$@"
}

pdfcat() {
    gs -sDEVICE=pdfwrite -dNOPAUSE -dQUIET -dBATCH -sOutputFile=- "$@"
}

pdfrange() {
    local first=${1:-1}
    local last=${2:-99999999}
    shift
    shift
    verbose2 +++ pdfrange first=$first last=$last "$@"
    gs -sDEVICE=pdfwrite -dNOPAUSE -dQUIET -dBATCH -dFirstPage=$((first)) -dLastPage=$last -sOutputFile=- "$@"
}

pdf1() {
    pdfhead -1 "$@"
}

pdfcat1() {
    mapreduce_files pdf1 pdfcat "$@"
}

pdftail() {
    local from=${from:--10}
    if [ "$from" -lt 0 ] ; then
        from=$((`nlines "$@"`-$from))
    fi
    pdfrange $from 99999999 "$@"
}

#range: 1-based double-closed interval e.g. (1,2) is first 2 lines
range() {
    local from=${1:-0}
    local to=${2:-99999999}
    shift
    shift
    perl -e 'require "$ENV{HOME}/u/libgraehl.pl";$"=" ";' -e '$F=shift;$T=shift;&argvz;$n=0;while (<>) { ++$n;;print if $n>=$F && $n<=$T }' $from $to "$@"
    if false ; then
        if [ "$from" -lt 0 ] ; then
            tail $from "$@" | head -$to
        else
            head -$to "$@" | tail +$from
        fi
    fi
}

lastpr() {
    perl -ne 'while (/(P=\S+ R=\S+)/g) { $first=$1 unless $first;$last=$1};END {print "[corpus ",($first eq $last?$last:"$last (i=0: $first)"),"]\n"}' "$@"
}

nthpr() {
    perl -ne '$n=shift;while (/(P=\S+ R=\S+)/g) { $first=$1 unless $first;$last=$1;last if --$n==0;};END {print "[corpus ",($first eq $last?$last:"$last (i=0: $first)"),"]\n"}' "$@"
}
bigclm() {
    perl -i~ -pe ' s/lw/big/g if /clm-lr/' *.sh
}
scts() {
    local l=${1:-ara}
    grep BLEU detok $l-*/$l-decode-iter-*/ibmbleu.out | bleulr
}
mvpre() {
    local pre=$1
    shift
    for f in "$@"; do
        mv $f `dirname $f`/$pre.`basename $f`
    done
}
ofcom() {
    pushd ~/t/graehl/gextract/optfunc
    gcom "$@"
}

pwdh() {
    local d=`pwd`
    local e=${d#$HOME/}
    if [ $d != $e ] ; then
        echo '~/'$e
    else
        e=${d#$WFHOME/}
        if [ $d != $e ] ; then
            echo "$WFHPC/$e"
        else
            echo $d
        fi
    fi
}
lnh() {
    ssh $HPCHOST "cd `pwdh` && ln -s $*" && ln -s "$@"
}
rmh() {
    ssh $HPCHOST "cd `pwdh` && rm $*" && rm "$@"
}
exh() {
    ssh $HPCHOST "cd `pwdh` && $*" && $*
}
double1() {
    catz "$@" "$@" > 2x."$1"
}
doublemany() {
    for f in "$@"; do
        double1 $f
    done
}

doubleinplace() {
    local nano=`nanotime`
    local suffix="$$$nano"
    for f in "$@"; do
        mv $f $f.$suffix
        cat $f.$suffix $f.$suffix > $f && rm $f.$suffix
    done
}

split() {
    cat "$@" | tr ' ' '\n'
}


cdr() {
    cd $(realpath "$@")
}
cdw() {
    cd $WFHOME/workflow/tune
}

ln0() {
    mkdir -p $2
    pushd $2
    for f in ../$1/*-*0000; do ln -s $f .; done
    [ "$3" ] && rm *-$3*0000
    ls
    popd
}

cpsh() {
    mkdir -p $2
    cp $1/*.sh $2
    ssh $HPCHOST "cd `pwdh` && mkdir -p $2"
}
cprules() {
    cpsh $1 $2
    pushd $2
    ln -s ../$1/* {xrsdb0000,rules0000} .
    popd
}
srescue() {
    find "$@" -name '*.dag.rescue*'
}
drescue() {
    (
    for f in `srescue "$@"`; do
        g=${f%.rescue001}
        g=${g%.rescue}
        echo $f
        if [[ $f != $g ]] ; then
            banner diff -u "$g" "$f"
            diff -u "$g" "$f"
        fi
    done
    )
}
rmrescue() {
    find "$@" -name '*.dag.rescue*' -exec rm {} \;
}
rescue() {
    for f in `srescue "$@"`; do
        pushd `dirname $f`
        condor_submit_dag `basename $f`
        popd
    done
}
csub() {
    pushd `dirname $1`
    condor_submit_dag `basename $1`
    popd
}
casubr() {
    casub `ls -dtr *00* | tail -1` "$@"
}

alias lsd="ls -al | egrep '^d'"
releasevariant="release debug-symbols=on --allocator=tbb"
variant=$releasevariant
qsi2() {
    local n=${1:-1}
    shift
    qsinodes=$n qsippn=2 qsubi -X "$@"
}
#variant=""
#
alias pjam="bjam --prefix=$FIRST_PREFIX -j4"
alias sjam="bjam $sbmtargs"
alias grepc="$(which grep) --color -n"
buildgraehl() {
    local d=$1
    local v=$2
    (set -e
     uselocalgccmac
     pushd $GRAEHLSRC/$1
     #export GRAEHL=$GRAEHLSRC
     #export TRUNK=$GRAEHLSRC
     #export SHARED=$TRUNK/shared
     #export BOOST=$BOOST_INCLUDE
     [ "$clean" ] && make clean
     #[ "$noclean" ] || make clean

     #LDFLAGS+="-ldl -pthread -lpthread -L$FIRST_PREFIX/lib"
     #LDFLAGS+="-ldl -pthread -lpthread -L$FIRST_PREFIX/lib"
     set -x
     pwd
     BOOST_INCLUDEDIR=/usr/local/include
     BOOST_LIBDIR=/usr/local/lib
     args="BOOST_LIBDIR=$BOOST_LIBDIR BOOST_INCLUDEDIR=$BOOST_INCLUDEDIR LIBDIR+=$BOOST_LIBDIR CMDCXXFLAGS+=-I$BOOST_INCLUDEDIR BOOST_SUFFIX=$BOOST_SUFFIX"
     make $args -j$MAKEPROC
     make $args install
     set +x
     popd
     if [ "$v" ] ; then
         pushd $FIRST_PREFIX/bin
         cp carmel carmel.$v
         cp carmel.static carmel.$v
         popd
     fi
    )
}
testcar() {
    (set -e
     cd ~/g/carmel/test
     ./runtests.sh
     dir=$(pwd)/logs/
     log=$dir/`ls -rt $dir | head -1`
     preview $log
    )

}
buildcar() {
    (set -e;
     TERM=dumb buildgraehl carmel "$@"
     testcar
    )
}
buildcar98() {
    (set -e;
     TERM=dumb CXX98=1 buildgraehl carmel "$@"
     testcar
    )
}
buildfem() {
    TERM=dumb buildgraehl forest-em "$@"
}

buildboost() {
    (
        set -e
        local withouts
        [ "$without" ] && withouts="--without-mpi --without-python --without-wave"
        [ "$noboot" ] || ./bootstrap.sh --prefix=$FIRST_PREFIX
        ./bjam cxxflags=-fPIC --threading=multi --runtime-link=static,shared --prefix=$FIRST_PREFIX $withouts --runtime-debugging=off -j$MAKEPROC install
        #--layout=tagged --build-type=complete
        # ./bjam --threading=multi --runtime-link=static,shared --runtime-debugging=on --variant=debug --layout=tagged --prefix=$FIRST_PREFIX $withouts install -j4
        # ./bjam --layout=system --threading=multi --runtime-link=static,shared --prefix=$FIRST_PREFIX $withouts install -j4
    )
}

MYEMAIL="graehl@gmail.com"


grepat() {
    local a=$1
    shift
    if [[ "$*" ]] ; then
        find "$@" -exec egrep "$a" {} \; -print
    else
        find . -exec egrep "$a" {} \; -print
    fi
}

frgrep() {
    local a=$1
    shift
    find . -exec fgrep "$@" "$a" {} \; -print
}

findpie() {
    local pattern=$1
    shift
    if [ "$2" ] ; then
        local name="-name $2"
        shift
    fi
    echo "find . $name $* -exec perl -p -i -e $pattern {} \;"
}


other() {
    local f=${1:?'returns $otherdir/`basename $1`'}
    showvars_required otherdir
    echo $otherdir/`basename $f`
}

diffother() {
    local f=$1
    shift
    local o=`other $f`
    require_files $f $o
    diff -u "$@" $f $o
}

other1() {
    local f=${1:?'returns $other1dir/$1 with $other1dir in place of the first part of $1'}
    showvars_required other1dir
    other1=$other1 perl -e 'for (@ARGV) { s|^/?[^/]+/|$ENV{other1}/|; print "$_\n"}' "$@"
}

diffother1() {
    local f=$1
    shift
    local o=`other1 $f`
    require_files $f $o
    diff -u "$@" $f $o
}

lastlog() {
    "$@" 2>&1 | tee ~/tmp/lastlog
}

perlf() {
    perldoc -f "$@" | cat
}

alias gc=gnuclientw

callgrind() {
    local exe=${1:?callgrind program args. env cache=1 branch=1 cgf=outfilename}
    #dumpbefore dumpafter zerobefore = fn-name,or dumpevery=1000000
    local base=`basename $1`
    shift
    local cgf=${cgf:-/tmp/callgrind.$base.`shortstamp`}
    echo $cgf
    local cachearg
    local brancharg
    if [[ $cache ]] ; then
        cachearg=--cache-sim=yes
    fi
    if [[ $branch ]] ; then
        brancharg=--branch-sim=yes
    fi
    (
        set -x
        valgrind --tool=callgrind $cachearg $brancharg --callgrind-out-file=$cgf --dump-instr=yes -- $exe "$@"
    )
    #local anno=~/tmp/callgrind.ArcIterator.cpp
    #callgrind_annotate $cgf $xmtxs/PhraseBased/src/ArcIterator.cpp > $anno
    #preview $anno $cgf
    #&& /Applications/qcachegrind.app $cgf
    echo $cgf
    tail -2 $cgf
}
VGARGS="--num-callers=16 --leak-resolution=high --suppressions=$HOME/u/valgrind.supp"
vg() {
    local darg=
    local varg=
    local suparg=
    if [[ $gdb ]] ; then
        gdb --args "$@"
        return
    fi
    [ "$debug" ] && darg="--db-attach=yes --db-command=cgdb"
    [ "$vgdb" ] && varg="--vgdb=full"
    [ "$sup" ] && suparg="--gen-suppressions=yes"
    local lc=
    if [ "$noleak" ] ; then
        lc=no
    else
        lc=yes
    fi
    if [ "$reach" ] ; then
        lc=full
        reacharg="--show-reachable=yes"
    fi
    GLIBCXX_FORCE_NEW=1 valgrind $darg $varg $suparg --leak-check=$lc $reacharg --tool=memcheck $VGARGS "$@"
}
vgf() {
    vg "$@"
    # | head --bytes=${maxvgf:-9999}
}
vgslow() {
    vg --track-origins=yes "$@"
}
#--show-reachable=yes
#alias vgfast="GLIBCXX_FORCE_NEW=1 valgrind --db-attach=yes --leak-check=yes --tool=addrcheck $VGARGS"
alias vgfast=vg
alias massif="valgrind --tool=massif --depth=5"

gpp() {
    g++ -x c++ -E "$@"
}

hrs() {
    if [ $# = 1 ] ; then
        from=$1
        to=.
    else
        COUNTER=1
        from=''
        while [ $COUNTER -lt $# ]; do
            echo $from
            echo ${!COUNTER}

            from="$from ${!COUNTER}"
            let COUNTER=COUNTER+1
        done
        to=${!#}
    fi
    echo rs \"$from\" $shpchost:$to
    rs $from $shpchost:$to
}


JOBTIME=${JOBTIME:-23:50:00}
qg() {
    qstat -au graehl
}
qsd() {
    qsub -q default -I -l walltime=$JOBTIME,pmem=4000mb "$@"
}

getattr() {
    attr=$1
    shift
    perl -ne '/\Q'"$attr"'\E=\ {\ {\ {([^}]+)\}\}\}/ or next;print "$ARGV:" if ($ARGV ne '-'); print $.,": ",$1,"\n";close ARGV if eof' "$@"
}
alias sa=". ~/u/aliases.sh"
alias sbl=". ~/u/bashlib.sh"
alias sl=". ~/local.sh"

tag_module() {
    local ver=$1
    local module=${2:-mini_decoder}
    showvars_required ver module
    if [ "$ver" ] ; then
        local to=tags/$module/version-$ver
        if [ "$force" ] ; then
            svn rm -m "redo $to" $SBMT_SVNREPO/$to
        fi
        cp_sbmt $to $3
    fi
}

tag_fem() {
    local ver=$1
    tag_module $ver forest-em graehl
}

tag_carmel() {
    local ver=$1
    tag_module $ver carmel graehl
}

tag_mini() {
    tag_module "$@"
}


lastbool() {
    echo $?
}


greph() {
    fgrep "$@" $HISTORYOLD
}

export IGNOREEOF=10

#alias hrssd="hrs ~/dev/syntax-decoder/src dev/syntax-decoder"
rgrep() {
    a=$1
    shift
    find . \( -type d -and -name .svn -and -prune \) -o -exec egrep -n -H "$@" "$a" {} \; -print
}
frgrep() {
    a=$1
    shift
    find . \( -type d -and -name .svn -and -prune \) -o -exec fgrep -n -H "$@" "$a" {} \; -print
}
dos2unix() {
    perl -p -i~ -e 'y/\r//d' "$@"
}
isdos() {
    perl -e 'while (<>) { if (y/\r/\r/) { print $ARGV,"\n"; last; } }' "$@"
}
psgn1() {
    psgn $1 | head -1
}
psgn() {
    psg $1 | awk ' {print $2}'
}
openssl=/usr/local/ssl/bin/openssl
certauth=/web/conf/ssl.crt/ca.crt
wgetr() {
    wget -r -np -nH "$@"
}
cleanr() {
    find . -name '*~' -exec rm {} \;
}
alias perl1="perl -e 'require \"\$ENV{HOME}/u/libgraehl.pl\";\$\"=\" \";' -e "
alias perl1p="perl -e 'require \"\$ENV{HOME}/u/libgraehl.pl\";\$\"=\" \";END {println();}' -e "
alias perl1c="perl -ne 'require \"\$ENV{HOME}/u/libgraehl.pl\";\$\"=\" \";END {while ((\$k,\$v)=each \%c) { print qq {\$k: \$v };println();}' -e "
alias clean="rm *~"

alias cpup="cp -vRdpu"
# alias hem="pdq ~/dev/tt; hscp addfield.pl forest-em-button.sh dev/tt; popd"

# alias ls="/bin/ls"
RCFILES=~/isd/hints/{.bashrc,aliases.sh,bloblib.sh,bashlib.sh,add_paths.sh,inpy,dir_patt.py,.emacs}
e() {
    local host=$1
    shift
    ssh -t $host -l graehl <<EOF
"$@"
EOF
}
ehost() {
    local p=`homepwd`
    local host=$1
    shift
    ssh -t $host -l graehl <<EOF
cd $p
"$@"
EOF
    #"/bin/bash"' --login -c "'"$*"'"'
}
tarxzf() {
    catz "$@" | tar xvf -
}
wxzf() {
    local b=`basename "$@"`
    wget "$@"
    tarxzf $b
}

pdq() {
    local err=`mktemp /tmp/pdq.XXXXXX`
    pushd "$@" 2>$err || cat $err 1>&2
    rm -f $err
}
pawd() {
    perl1p "print &abspath_from qw(. . 1)"
}
alias sl=". ~/isd/hints/bashlib.sh"
alias hh="comh;huh"
if [ $HOST = TRUE ] ; then
    alias startxwin="/usr/X11R6/bin/startxwin.sh"
    alias rr="ssh nlg0 rdec"
    alias rdb="ssh nlg0 rdecb"
    alias rrs="ssh nlg0 rdecs"
fi

cb() {
    pushd $BLOBS/$1/unstable
}

allscripts="bashlib libgraehl qsh decoder"
allblobs="bashlib libgraehl qsh decoder/decoder-bin decoder rule-prep/identity-ascii-xrs rule-prep/add-xrs-models"
refresh_all() {
    (
        set -e
        for f in $allblobs; do redo $f latest ; done
    )
}
refresh_scripts() {
    (
        set -e
        for f in $allscripts; do redo $f latest ; done
    )
}
new_all() {
    (
        set -e
        for f in $allblobs; do blob_new_latest $f ; done
    )
}

xscripts() {
    find $1 -name '*.pl*' -exec chmod +x {} \;
    find $1 -name '*.sh*' -exec chmod +x {} \;
}

if [[ ${ONCYGWIN:=} ]] ; then
    sshwrap() {
        local which=$1
        shift
        /usr/bin/$which -i c:/cache/.ssh/id_dsa "$@"
    }

    wssh() {
        sshwrap ssh "$@"
    }

    wscp() {
        sshwrap scp "$@"
    }

fi


dvi2ps() {
    papertype=${papertype:-letter}
    local b=${1%.dvi}
    local d=`dirname $1`
    local b=`basename $1`
    pushd $d
    b=${b%.dvi}
    dvips -t $papertype -o $b.ps $b.dvi
    popd
}

latrm() {
    rm -f *-qtree.aux $1.aux $1.dvi $1.bbl $1.blg $1.log
}
cplatex=~/texmf/pst-qtree.tex
[ -f $cplatex ] || cplatex=
latp() {
    local d=`dirname $1`
    local b=`basename $1`
    pushd $d

    [ "$cplatex" ] && cp $cplatex .
    latrm $b
    local i
    for i in `seq 1 ${latn:=1}`; do
        if [ "$batchmode" ] ; then
            latex "\batchmode\input $b.tex"
        else
            latex $b.tex
        fi
    done
    [ "$cplatex" -a -f "$cplatex" ] && rm $cplatex
    popd
}
latq() {
    local f=${1%.tex}
    latp $f && dvi2pdf $f
    latrm $f
}
dvi2pdf() {
    local d=`dirname $1`
    local b=`basename $1`
    pushd $d
    b=${b%.dvi}
    dvi2ps $b
    ps2pdf $b.ps $b.pdf || true
    popd
}
lat2pdf() {
    latq "$@"
}
lat2pdf_landscape() {
    papertype=landscape lat2pdf $1
}

vizalign() {
    echo2 vizalign "$@"
    local f=$1
    shift
    local n=`nlines "$f.a"`
    if [ "$n" = 0 ] ; then
        warn "no lines to vizalign in $f.a"
    fi
    local lang=${lang:-chi}
    # if [ -f $f.info ] ; then
    # viz-tree-string-pair.pl -t -l $lang -i $f -a $f.info
    # else
    quietly viz-tree-string-pair.pl -c "$f" -t -l $lang -i "$f" "$@"
    # fi
    quietly lat2pdf_landscape $f
}
lat() {
    papertype=${papertype:-letter}
    latp $1 && bibtex $1 && latex $1.tex && ((latex $1.tex && dvips -t $papertype -o $1.ps $1.dvi) 2>&1 | tee log.lat.$1)
    ps2pdf $1.ps $1.pdf
}

g1() {
    local ccmd="g++"
    local linkcmd="g++"
    local archarg=
    if [[ $OS = Darwin ]] ; then
        ccmd="g++-6"
        linkcmd="g++-6"
        #archarg="-arch x86_64"
        macboost
    fi
    local program_options_lib="-lboost_program_options$BOOST_SUFFIX -lboost_system$BOOST_SUFFIX"
    local source=$1
    shift
    [ -f "$source" ] || cd $GRAEHL_INCLUDE/graehl/shared
    pwd
    ls $source
    local out=${OUT:-$source.`filename_from $HOST "$*"`}
    (
        set -e
        local flags="-std=c++11 $CXXFLAGS $MOREFLAGS -I$GRAEHL_INCLUDE -I$BOOST_INCLUDEDIR -DGRAEHL_G1_MAIN"
        #-Wno-pragma-once-outside-header
        showvars_optional ARGS MOREFLAGS flags
        if ! [ "$OPT" ] ; then
            flags="$flags -O0"
        fi
        #$ccmd $archarg $MOREFLAGS -ggdb -fno-inline-functions -x c++ -DDEBUG -DGRAEHL__SINGLE_MAIN $flags "$@" $source -c -o $out.o
        #$linkcmd $archarg $LDFLAGS $out.o -o $out $program_options_lib 2>/tmp/g1.ld.log
        $ccmd $program_options_lib -L$BOOST_LIB $MOREFLAGS $archarg -g -fno-inline-functions -x c++ -DDEBUG -DGRAEHL__SINGLE_MAIN $flags "$@" $source -o $out
        #$archarg
        LD_RUN_PATH=$BOOST_LIB $gdb ./$out $ARGS
        if [[ $cleanup = 1 ]] ; then
            rm -f $out.o $out
        fi
        set +e
    )
}
gtest() {
    MOREFLAGS="$GCPPFLAGS" OUT=$1.test ARGS="--catch_system_errors=no" g1 "$@" -DGRAEHL_TEST -DGRAEHL_INCLUDED_TEST -ffast-math -lboost_unit_test_framework${BOOST_SUFFIX:-mt} -lboost_random${BOOST_SUFFIX:-mt}
}

gsample() {
    local s=$1
    shift
    OUT=$s.sample ARGS=""$@"" g1 $s -DGRAEHL_SAMPLE
}


conf() {
    ./configure $CONFIG "$@"
}

myconfig() {
    local C=$CONFIGBOOST
    [ "$noboost" ] && C=$CONFIG
    showvars_required C BUILDDIR LD_LIBRARY_PATH
    [ -x configure ] || ./bootstrap
    [ "$distclean" ] && [ "$BUILDDIR" ] && [ -d $BUILDDIR ] && rm -rf $BUILDDIR
    mkdir -p $BUILDDIR
    pushd $BUILDDIR
    [ "$reconfig" ] && rm -f Makefile
    echo ../configure $C $myconfigargs "$@"
    [ -f Makefile ] || ../configure $C $myconfigargs "$@"
    popd
}

doconfig() {
    noboost=1 myconfig
}

dobuild() {
    if doconfig "$@" ; then
        pushd $BUILDIR && make && make install
        popd
    fi
}

makesrilm_c() {
    make OPTION=_c World
}

set_i686() {
    set_build i686
}
set_i686_debug() {
    set_build i686 -O0
}

upt() {
    pushd ~/t
    svn update
    popd
}

mlm=~/t/utilities/make.lm.sh
[ -f $mlm ] && . $mlm

cwhich() {
    l `which "$@"`
    cat `which "$@"`
}

vgx() {
    (
        #lennonbin
        local outarg smlarg out vgprog dbarg leakcheck
        [ "$out" ] && outarg="--log-file-exactly=$out"
        [ "$xml" ] && xmlarg="--xml=yes"
        [ "$xml" ] && out=${out:-valgrind.xml}
        vgprog=valgrind
        [ "$valk" ] && vgprog=valkyrie
        [ "$valk" ] || leakcheck="--leak-check=yes --tool=memcheck"
        [ "$debug" ] && dbarg="--db-attach=yes"
        GLIBCXX_FORCE_NEW=1 $vgprog $xmlarg $leakcheck $dbarg $VGARGS $outarg "$@"
        [ "$xml" ] && valkyrie --view-log $out
    )
    #--show-reachable=yes
}

unshuffle() {
    perl -e 'while (<>) {print;$_=<>;die unless $_;print STDERR $_}' "$@"
}

ngr() {
    local lm=$1
    shift
    ngram -ppl - -debug 1 -lm $lm "$@"
}


compare_length() {
    extract-field.pl -print -f text-length,foreign-length "$@" | summarize_num.pl
}

hypfromdata() {
    perl -e '$whole=$ENV{whole};$ns=$ENV{nsents};$ns=999999999 unless defined $ns;while (<>) { if (/^(\d+) -1 \# \# (.*) \# \#/) { if ($l ne $1) { print ($whole ? $_ : "$2\n");last unless $n++ < $ns; $l=$1; } } } ' "$@"
}

stripbracespace() {
    perl -pe 's/^\s+([ {}])/$1/' "$@"
}

# input is blank-line separated paragraphs. print first nlines of first nsents paragraphs. blank=1 -> print separating newline
firstlines() {
    perl -e '$blank=$ENV{blank};$ns=$ENV{nsents};$ns=999999999 unless defined $ns;$max=1;$max=$ENV{nlines} if exists $ENV{nlines};$nl=0;while (<>) { print if $nl++<$max; if (/^$/) {$nl=0;print "\n" if $blank;last unless $n++ < $ns;} }' "$@"
}

stripunknown() {
    perl -pe 's/\([A-Z]+ \@UNKNOWN\@\) //g' "$@"
}

fixochbraces() {
    # perl -ne '
    # s/^\s+\ {/ \ {\n/ if ($lastcond);
    # s/\s+$//;
    # print;
    # $lastcond=/^\s+(if|for|do)/;
    # print "\n" unless $lastcond;
    # END {
    # print "\n" if $lastcond;
    # }
    # ' "$@"
    perl -e '$/=undef;$_=<>;print $_
$ws=q {[ \t]};
$kw=q {(?:if|for|while|else)};
$notcomment=q {(?:[^/\n]|/(?=[^/\n]))};
s#(\b$kw\b$notcomment+)$ws*(?:(//[^\n]*\n)|\n)$ws*\ {$ws*#$1 { $2#gs;
s/}\s*else/} else/gs;
print;
# @lines=split "\n";
# for (@lines) {
# s#(\b$kw\b$notcomment*)(//.*?)\s*\ {\s*$#$1 { $2#;
# print "$_\n";
# }
' "$@"
    #s|(\b$kw\b$notcomment*)(//[^\n]*)(\ {$ws*)\n|$1$3\n$2\n|gs;

}


tviz() {
    (
        set -e
        captarg=
        work=${work:-tviz}
        if [ "$caption" ] ; then
            captionfile=`mktemp /tmp/caption.XXXXXXXX`
            echo "$caption" > $captionfile
            captarg="-c $captionfile"
        fi
        escapetree | treeviz -s -p 'graph [fontsize=12,labelfontsize=10,center=0]; node [fontsize=10,height=.06,margin=".04,.02",nodesep=.02,shape=none] ;'"$*" $captarg > $work.dot
        require_files $work.dot
        out=$work
        dot -Tpng < $work.dot > $out.png
        require_files $out.png
        rm -f $captionfile
        ls $work. {dot,png}
        if ! [[ $noview ]] ; then
            firefox $work.png
        fi
    )
}
treevizn() {
    (
        set -e
        local graehlbin=`echo ~/bin`
        export PATH=$BLOBS/forest-em/latest:$graehlbin:$PATH
        local n=$1
        shift
        local f=$1
        shift
        showvars_required n f
        require_files $f
        local dir=`dirname -- $f`
        local file=`basename -- $f`
        local outdir=$dir/treeviz.$file
        local workdir=$outdir/work
        mkdir -p $outdir
        mkdir -p $workdir
        local work=$workdir/line$n
        local out=$outdir/line$n
        local captionfile=`mktemp /tmp/caption.XXXXXXXX`
        showvars_required out captionfile
        get $n $f > $work.1best
        require_files $work.1best
        extract-field.pl -f hyp -pass $work.1best > $work.hyp
        require_files $work.hyp
        orig_pos=`extract-field.pl -f unreranked-n-best-pos $work.1best`
        (echo -n "line $n of $f (orig #$orig_pos): "; cat $work.hyp; ) > $captionfile
        require_files $captionfile
        cat $work.1best | extract-field.pl -f tree -pass | escapetree > $work.treeviz.tree
        if [ "$*" ] ; then
            cat $work.treeviz.tree | treeviz -s -p 'graph [fontsize=12,labelfontsize=10,center=0]; node [fontsize=10,height=.06,margin=".04,.02",nodesep=.02,shape=none] ;'"$*" -c $captionfile > $work.dot
        else
            cat $work.treeviz.tree | treeviz -s -p 'graph [fontsize=12,labelfontsize=10,center=0]; node [fontsize=10,height=.06,margin=".04,.02",nodesep=.02,shape=none] ;' -c $captionfile > $work.dot
        fi
        require_files $work.dot
        dot -Tpng < $work.dot > $out.png
        require_files $out.png
        rm -f $captionfile
    )
}

wtake() {
    wget -r -l1 -H -t1 -nd -N -np -A.mp3 -erobots=off "$@"
}
alias slo='ssh login.clsp.jhu.edu -l jgraehl'
cplo1() {
    local dest=`relpath ~ $1`
    local dd=`dirname $dest`
    [ "$dd" == "." ] || elo mkdir -p "$dd"
    [ -d "$1" ] && dest=$dd
    echo scp -r "$1" jgraehl@login.clsp.jhu.edu:"$dest"
    scp -r "$1" jgraehl@login.clsp.jhu.edu:"$dest"
}
rfromlo1() {
    local dest=`relpath ~ $1`
    mkdir -p `dirname $dest`
    echo scp -r jgraehl@login.clsp.jhu.edu:"$dest" "$1"
    scp -r jgraehl@login.clsp.jhu.edu:"$dest" "$1"
}
flo() {
    local f
    for f in "$@"; do
        echo scp -r jgraehl@login.clsp.jhu.edu:"$f" .
        scp -r jgraehl@login.clsp.jhu.edu:"$f" .
    done
}

elo() {
    local cdir=`pwd`
    local lodir=`relpath ~ $cdir`
    ssh login.clsp.jhu.edu -l jgraehl ". .bashrc;. isd/hints/aliases.sh;cd $lodir && $*"
    # "$@"
}
eboth() {
    echo "$@"
    "$@" && echo && echo HPC: && echo && ehpc "$@"
}
DefaultBaseAddress=0x70000000
DefaultOffset=0x10000

cygbase() {
    local BaseAddress=${base:-$DefaultBaseAddress}
    local Offset=${off:-$DefaultOffset}
    local f=$1
    shift
    [ -f "$f" ] || f=c:/cygwin/bin/$f
    rebase -v -d -b $BaseAddress -o $Offset $f "$@"
}

export SVNAUTHORS=~/isd/hints/svn.authorsfile
clonecar() {
    local CR=https://nlg0.isi.edu/svn/sbmt/
    git config svn.authorsfile $SVNAUTHORS && git svn --authors-file=$SVNAUTHORS clone --username=graehl --ignore-paths='^(NOTES.*|scraps|syscom|tt|xrsmodels|Jamfile|dagtt)' ---trunk=$CR/trunk/graehl "$@"
    #--tags=$CR/tags --branches=$CR/branches
    #-r 3502 at
    # https://nlg0.isi.edu/svn/sbmt/trunk/graehl@3502
    #e4bd1e594dd7051a9e50561d19bdc31139ba1159

    #--no-metadata
}


par() {
    local npar=${npar:-20}
    local parargs=${parargs:-"-p 9g -j $npar"}
    local logdir=${logdir:-`filename_from log "$@"`}
    logdir="`abspath $logdir`"
    mkdir -p "$logdir"
    showvars_required npar parargs logdir
    logarg="-e $logdir"
    $WSMT/vest/parallelize.pl $parextra $parargs $logarg -- "$@"
}
mkdest() {
    if [ "$2" ] ; then
        dest=$2
    else
        dest=`dirname "$1"`
        mkdir -p "$dest"
    fi
}

fromhost1() {
    (
        cd
        mkdest "$@"
        user=${user:-`userfor $host`}
        echo scp -r "$user@$host:$1" "$dest"
        scp -r $user@$host:"$1" "$dest"
    )
}
fromhost() {
    local host=$1
    shift
    host=$host forall fromhost1 "$@"
}
relhomeby() {
    ${relpath:-$UTIL/relpath} ~ "$@"
}

shost1() {(set -e
           local portarg=
           if [[ ${port:-} ]] ; then
               portarg=-P${port:-}
           fi
          )}
tohost1() {
    (
        set -e
        f=$(relhomeby $1)
        cd
        local u
        user=${user:-`userfor $host`}
        [ "$user" ] && u="$user@"
        local portarg=
        if [[ ${port:-} ]] ; then
            portarg=-P${port:-}
        fi
        echo scp $portarg -r "$f" "$u$host:`dirname $f`/"
        if [ "${dryrun:-}" ] ; then
            showvars_optional relpath
        else
            scp $portarg -r "$f" "$u$host:`dirname $f`"
        fi
    )
}
tohostp1() {
    relpath=$UTIL/relpathp tohost1 "$@"
}
tohost() {
    local host=$1
    shift
    host=$host forall tohost1 "$@"
}
tohostp() {
    local host=$1
    shift
    host=$host forall tohostp1 "$@"
}

fromhpc() {
    fromhost $HPCHOST "$@"
}
fromhpc1() {
    host=$HPCHOST fromhost1 "$@"
}
tcmi() {
    local d=$1
    local f=$1
    d=${d%.tar.bz2}
    d=${d%.tar.gz}
    d=${d%.tgz}
    d=${d%.tar.xz}
    shift
    tarxzf "$f" && cd "$d" && cmi "$@"
}
wtx() {
    wget "$1" && tarxzf $(basename $1)
}
wcmi() {
    wget "$1" && tcmi "$(basename $1)"
}
cmi() {
    if ./configure $CONFIG "$@"; then
        make -j 4
        make && make install
    fi
}

cpanforce() {
    perl -MCPAN -e '*CPAN::_flock = sub { 1 }; install( "Test::Output" );' "$@"
}

bleulog() {
    perl -ne 'BEGIN {$x=shift};print "$x\t$1\n" if /BLEU(?: score)? = ([\d.]+)/' -- "$@"
}

firstnum() {
    # 1: string to find number in: .*prefix.*(\d[\d.e-+]*)
    # 2: optional match - first number after match gets printed.
    perl -e '$_=shift; $p=shift; $_=$1 if /\Q$p\E(.*)/;if (/((?:[-+\d]|(?<=\.)\.)[-\d.e+]*)/) { $_=$1;s/\.$//;print "$_\n"}' "$@"
}

shortsh1() {
    perl -i~ -pe 's/^\s*function\s+(\w+)\s*\ {/\1() {/' -- "$@"
}
shortsh() {
    shortsh1 *.sh "$@"
}
conff=`echo `
allconff="$conff elisp"
rsync_exclude="-C --exclude=.git/ --cvs-exclude"
sync2() {
    (
        cd
        local h=$1
        shift
        local u
        user=${user:-$(userfor $h)}
        [ "$user" ] && u="$user@"
        local f
        local darg
        [ "${dryrun:-}" ] && darg="-n"
        echo sync2 user=$user host=$h "$@"
        symarg=
        [[ $followsymlink ]] && symarg=L
        (for f in "$@"; do if [ -d "$f" ] ; then echo $f/; else echo $f; fi; done) | rsync $darg -avruz$symarg -e ssh --files-from=- ${rsyncargs:-} ${rsync_exclude:-} . $u$h:${dest:=.}
    )
}
syncto() {
    sync2 "$@"
}
userfor() {
    case $1 in
        login*) echo -n jgraehl ;;
        *) echo -n graehl ;;
    esac
}
syncfrom() {
    (
        cd
        local h=$1
        shift
        local u
        user=${user:-`userfor $h`}
        [ "$user" ] && u="$user@"
        local f
        local darg
        [[ ${dryrun:-} ]] && darg="-n"
        echo syncFROM user=$user host=$h "$@"
        sleep 2

        (for f in "$@"; do echo $f; done) | rsync $darg ${rsync_exclude:-} -avruz -e ssh --files-from=- $u$h:. .
    )
}
alias qs="qstat -f -j"
qdelrange() {
    for i in `seq -f "%.0f" "$@"`; do
        qdel $i
    done
}
cdech() {
    $cdec --help 2>&1 | more
}
gitundo() {
    git stash save --keep-index
}
gitundorm() {
    git stash drop
}
alias comlo=ws10com
gitcom() {
    git commit -a -m "$*"
}
rwpaths() {
    perl -i -pe 's#/home/jgraehl/#/home/graehl/#g;s#/export/ws10smt/#/home/graehl/e/#g' "$@"
}
s0() {
    ssh -l jgraehl a0${1:-3}
}
alias ec='emacsclient -n '
gitsvnbranch() {
    git config --get-all svn-remote.$1.branches
}
gdcommit() {
    (cd $WSMT
     git svn dcommit --commit-url https://graehl:ts7sr8dG9nj2@ws10smt.googlecode.com/svn/trunk
    )
}
refargs() {
    local f
    local r
    local refarg=${refarg:--R}
    for f in "$@"; do
        r="$r $refarg $f"
    done
    echo $r
}
makesrilm() {
    (set -e;
     if [ -f "$1" ] ; then
         mkdir -p srilm
         cd srilm
         tarxzf ../$1
         local d=`realpath .`
         perl -i -pe 's {# SRILM = .*} {SRILM = '$d'}' Makefile
         shift
     fi
     head Makefile
     [ "$noclean" ] || make cleanest OPTION=_c
     local a64
     [ "$force64" ] && a64="MACHINE_TYPE=i686-m64"
     MACHINE_TYPE=`sbin/machine-type`
     for d in bin lib; do
         ln -sf $d/${MACHINE_TYPE}_c $d/${MACHINE_TYPE}
     done
     make $a64 World OPTION=_c "$@"
    )
}
page() {
    "$@" 2>&1 | more
}
page2() {
    page=more save12 "$@"
}
scr() {
    screen -UaARRD
}
scrls() {
    screen -list
}

#for cygwin:
altinstall() {
    local b=$1
    local ver=$2
    local exe=$3
    local f=/usr/bin/$b-$ver$exe
    showvars_required b ver f
    require_file $f && /usr/sbin/update-alternatives --install /usr/bin/$b$exe $b $f 30
}

plboth() {
    local o=$1
    local oarg="-landscape"
    [ "${port:-}rait" ] && oarg=
    shift
    pl -png -o $o.png "$@"
    pl -ps -o $o.ps $oarg "$@"
    ps2pdf $o.ps $o.pdf
}

graph() {
    local xaxis=${xaxis:-1}
    local yaxis=${yaxis:-2}
    local zaxis=${zaxis:-3}
    local xlbl="$1"
    local ylbl="$2"
    shift
    shift
    [ "$zlbl" ] && zarg="y2=$zaxis ylbl2=$zlbl"
    [ "$zlbl" ] && zword=".and.$zlbl"
    local ylbldistance=${ydistance:-'0.7"'}
    local scale=${scale:-1.4}
    [ "$topub" ] && pubb=~/pub/
    local name=${name:-`filename_from $in.$ylbl$zword.vs.$xlbl$zword`}
    local obase=${obase:-$pubb$name}
    mkdir -p `dirname $obase`
    local title=${title:-$obase}
    local darg
    if [ "$data" ] ; then
        darg="data=$data"
        if [ "$pubb" ] ; then
            banner cp "$data" $pubb/
            cp "$data" $pubb/
        fi
    fi

    pubb=$pubb plboth $obase -prefab lines $darg x=$xaxis xlbl="$xlbl" y=$yaxis ylbl="$ylbl" ylbldistance=$ylbldistance title="$title" ystubfmt '%6g' ystubdet="size=6" -scale $scale $zarg "$@"
    set +x
    local of=$obase.png
    ls -l $obase.* 1>&2
    echo $of
}


graph3() {
    local y=$1
    local ylbl="$2"
    local y2=$3
    local ylbl2="$4"
    local y3=$5
    local ylbl3="$6"
    local ylbldistance=${7:-'0.7"'}
    local name=${name:-$data}
    local obase=${obase:-$opre$name.x_`filename_from $xlbl`.$y.`filename_from $ylbl`.$y2.`filename_from $ylbl2`.$y3.`filename_from $ylbl3`}
    local of=$obase.png
    local ops=$obase.ps
    #yrange=0
    local yrange_arg
    [ "$ymin" ] && yrange_arg="yrange=$ymin $ymax"
    #pointsym=none pointsym2=none
    title=${title:-"$ylbl $ylbl2 $ylbl3 vs. $xlbl"}
    xlbl=${xlbl:=x}
    showvars_required obase xlbl ylbl ylbl2 ylbl3
    showvars_optional yrange yrange_arg title
    require_files $data
    plboth $obase -prefab lines data=$data x=1 "$yrange_arg" y=$y name="$ylbl" y2=$y2 name2="$ylbl2" y3=$y3 name3="$ylbl3" ylbldistance=$ylbldistance xlbl="$xlbl" title="$title" ystubfmt '%4g' ystubdet="size=6" linedet2="style=1" linedet3="style=3" -scale ${scale:-1.4}
    echo $of
}

cppdb() {
    local f=$1
    shift
    g++ -x c++ -dD -E -I. -I.. -I../.. -I$FIRST_PREFIX/include "$@" "$f" > "$f.ii"
    preview "$f.ii"
}

clear_gcc() {
    unset C_INCLUDE_PATH
    unset CPPFLAGS
    unset LD_LIBRARY_PATH
    unset LIBRARY_PATH
    unset LDFLAGS
    unset CXXFLAGS
    unset CFLAGS
    unset CPPFLAGS
}

first_gcc() {
    clear_gcc
    export LD_LIBRARY_PATH=$FIRST_PREFIX/lib:$FIRST_PREFIX/lib64
    #    export C_INCLUDE_PATH=$FIRST_PREFIX/include
    export CXXFLAGS="-I$FIRST_PREFIX/include"
    export LDFLAGS="-L$FIRST_PREFIX/lib -L$FIRST_PREFIX/lib64"
}

dmakews() {
    g++ --version
    debug=1 MAKEPROC=4 cmakews "CXXFLAGS+=-fdiagnostics-show-option" "$@"
}

checkws() {
    debug=1 MAKEPROC=4 cmakews check
}

uninstgcc() {
    (
        set -e
        cd $FIRST_PREFIX
        cd bin
        mkdir uninstgcc
        mv *gcc* *g++* cpp gcov uninstgcc/
    )
}

confws() {
    (
        cd $WSMT
        ./configure --with-srilm=`realpath ~/src/srilm` --prefix=$FIRST_PREFIX "$@"
    )
}

fastflags() {
    CFLAGS+=-Ofast
    CXXFLAGS+=-Ofast
    export CFLAGS CXXFLAGS
    # turns on fast-math,and maybe some unsafe opts beyond -O3
}

slowflags() {
    CFLAGS+=-O0
    CXXFLAGS+=-O0
    export CFLAGS CXXFLAGS
}
makecdec() {
    make cdec CXXFLAGS+="-Wno-sign-compare -Wno-unused-parameter -loolm -ldstruct -lmisc -lz -lboost_program_options -lpthread -lboost_regex -lboost_thread -O0 -ggdb"
}
carmelv() {
    grep VERSION ~/t/graehl/carmel/src/carmel.cc
}
pdf2() {
    batchmode=1 latn=2 lat2pdf_landscape "$@"
}
alias ..="cd .."
alias c=cd
if [[ $OS = Darwin ]] ; then
    alias l="ls -alFG"
    alias lo="ls -alHLFCG"
    export CLICOLOR=1
else
    alias l="ls -alF --color=auto"
    alias lo="ls -alHLFC --color=auto"
fi
g() {
    git "$@"
}
getcert() {
    local REMHOST=$1
    local REMPORT=${2:-443}
    echo |\
        openssl s_client -connect ${REMHOST}:${REMPORT} 2>&1 |\
        sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p'
}
showprompt() {
    echo $PS1 | less -E
    echo $PROMPT_COMMAND | less -E
}
ming() {
    local src=$1
    shift
    local exe=$src
    exe=${exe%.cc}
    exe=${exe%.cpp}
    exe=${exe%.c}
    /mingw/bin/g++ --std=c++11 $src -o $exe "$@" && ./$exe
}

gjam() {
    GJAMROUND=$DROPBOX/jam/1c
    GJAMDIR=$GJAMROUND/a
    local in=$1
    local basein=`basename $in`
    if [[ $basein = A-* ]] ; then
        GJAMDIR=$GJAMROUND/a
    elif [[ $basein = B-* ]] ; then
        GJAMDIR=$GJAMROUND/b
    elif [[ $basein = C-* ]] ; then
        GJAMDIR=$GJAMROUND/c
    fi

    shift
    local cc=$1
    if ! [[ -f $cc ]] ; then
        cc=solve.cc
    fi
    if ! [[ -s $cc ]] ; then
        cd $GJAMDIR
        shift
    fi
    if [[ $in ]] ; then
        cp $in in
    fi
    require_file in
    set -e

    if [[ $no11 ]] ; then
        g++ -O -g $cc -o solve
    else
        $CXX11 -O3 --std=c++11 $cc -o solve
    fi
    time ./solve
    set +x
    out=out
    expect=$in.expect
    [ -f $expect ] || expect=
    if [[ $in ]] ; then
        basein=$(basename $in)
        out=${basein%.in}.out
        cp out $out
    else
        in=in
    fi
    wrong=${out%.out}.wrong
    tailn=20 preview $out $expect
    wc -l $in $out
    if [[ -f $wrong ]] ; then

        diff -u $out $wrong
        set +x
    fi
}


jpgclean() {
    exiftool -all= ${*:-*.jpg}
}

bd() {
    local OLDPWD=`pwd`
    local NEWPWD=
    local index=
    if [ "$1" = "-s" ] ; then
        NEWPWD=`echo $OLDPWD | sed 's|\(.*/'$2'[^/]*/\).*|\1|'`
        index=`echo $NEWPWD | awk ' { print index($1,"/'$2'"); }'`
    else
        NEWPWD=`echo $OLDPWD | sed 's|\(.*/'$1'/\).*|\1|'`
        index=`echo $NEWPWD | awk ' { print index($1,"/'$1'/"); }'`
    fi

    if [ $index -eq 0 ] ; then
        echo "No such occurrence."
    fi

    echo $NEWPWD
    cd "$NEWPWD"
}
diffbranch() {
    local diff=~/tmp/$1.diff
    sh -c "cd `pwd`; git diff $1 $1^1 -- > $diff"
    head -100 $diff
    echo $diff
}
if [[ -x $xmtxspath.sh ]] ; then
    . $xmtxspath.sh
fi

### ssh tunnel:

chost=c-graehl
ontunnel=
if [[ $HOST = $graehlmac ]] ; then
    ontunnel=1
    /sbin/ifconfig > /tmp/ifcfg
    if grep -q 10.110 /tmp/ifcfg; then
        ontunnel=
    fi
fi
trremotesubdir=/local/graehl/xmt
trhomesubdir=x
trhome() {
    perl -e '
$remotesubdir=shift;
$homesubdir=shift;
$homesubdir="$homesubdir/" if $homesubdir;
if ($remotesubdir) {
  for (@ARGV) { $_ =~ s{(/Users/graehl|/home/graehl|\~)/$homesubdir}{$remotesubdir/}; }
}
print join(" ",@ARGV)' -- "$@"
}
trhomedir() {
    trhome ${remotehome:-'~'} '' "$@"
}
tunhost="pontus.languageweaver.com"
tunport=4640
sshvia() {
    local lport=`randomport`
    local rhost=$1
    shift
    echo via $lport to $rhost: ssh "$@" 1>&2
    local permit="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no "
    ssh -f -L $lport:$rhost:22 -p $tunport $tunhost "sleep 6" && ssh $permit -p $lport localhost "$@"
}
sshlog() {
    echo ssh "$@" 1>&2
    time ssh "$@"
}
if ! [[ $MAKEPROC ]] ; then
    if [[ $lwarch = Apple ]] ; then
        MAKEPROC=2
    elif [[ $HOST = c-graehl ]] ; then
        MAKEPROC=13
    else
        MAKEPROC=7
    fi
fi
if [[ -d $SDL_EXTERNALS_PATH ]] ; then
    export PATH=$SDL_EXTERNALS_PATH/../Shared/java/apache-maven-3.0.4/bin:$PATH
fi

uselocalgccmac() {
    if [[ -d /local/gcc/bin ]] ; then
        export PATH=/local/gcc/bin:$PATH
        export LD_LIBRARY_PATH=/local/gcc/lib64:$LD_LIBRARY_PATH
    else
        ccachepre=ccache-
        ccachepre=
        ccsuffix=-6
        export CC=${ccachepre}gcc$ccsuffix
        export CXX=${ccachepre}g++$ccsuffix
    fi
}
#alib=~/c/xmt-externals/Apple/libraries/boost_1_58_0/lib/
finduniq() {
    find . -exec basename {} \; | sort | uniq -c
}
kenplzarg="-S 80% -T /tmp/kenplz"
cp2ken() {
    local base=${1%.gz}
    base=${base%.arpa}
    local out=${2:-$base.ken}
    local q=${3:-4}
    local b=${4:-4}
    local a=${5:-255}
    build_kenlm_binary $kenplzarg -q $q -b $b -a $a trie $1 $out
}
cp2kenqba() {
    local base=${1%.gz}
    base=${base%.arpa}
    local q=${2:-4}
    local b=${3:-4}
    local a=${4:-255}
    local out=${5:-$base.$q.$b.$a.ken}
    if ! [[ -f $out ]] || [[ $force ]] ; then
        build_kenlm_binary $kenplzarg -q $q -b $b -a $a trie $1 $out 1>&2
    fi
    echo $out
}
cp2kenprobing() {
    local base=${1%.gz}
    base=${base%.arpa}
    local out=${2:-$base.probing.ken}
    if ! [[ -f $out ]] || [[ $force ]] ; then
        build_kenlm_binary -p ${3:-1.5} probing $1 $out 1>&2
    fi
    echo $out
}
benchkens() {
    local model=$1
    local corpus=$2
    showvars_required model corpus
    local c=`mktemp /tmp/corpus.XXXXXXXX`
    catz $corpus > $c
    (
        set -e
        for q in 4; do
            for b in 4; do
                for a in 0 4 8 12 14 16 18 20 22; do
                    km=`cp2kenqba $model $q $b $a`
                    benchken1 $km $c
                done
            done
        done
        km=`cp2kenprobing $model`
        benchken1 $km $c |  matchn '(^.*) corpus(.......)' '^\d:.*?s' 'is (\S+)'
    ) | tee times.benchken
}
echosize() {
    ls -1sh $model "$@" | tr '\n' '\t'
}
benchken1() {
    local model=$1
    local corpus=${2:-corpus}
    require_files $model $corpus
    (
        set -e
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/graehl/pub/lib
        kb=kenlm_benchmark
        cid=$corpus.id
        catz $corpus | $kb vocab $model > $cid
        cat $model $cid > /dev/null
        (echosize model; /usr/bin/time -f '%Es - %Mkb peak' -- $kb query $model < $cid 2>&1 ) | tee time.$model
        rm $cid
    ) 2>&1
}
match1() {
    perl -e '$r=shift;$r=qr/($r)/;while(<>) { next unless /$r/o; print $2 ? $2 : $1, "\n"; last }' "$@"
}
matchn() {
    perl -we 'use strict;
my @r = map {qr/($_)/} @ARGV;
my @c = map {undef} @ARGV;
while(defined($_=<STDIN>)) {
chomp;
for my $i (0..$#r) {
    my $x = $r[$i];
    print STDERR "$_ /$x/\n" if $ENV{DEBUG};
    my @m;
    if (!defined($c[$i])) {
       my @m = ( /$x/ );
       my $N = scalar(@m);
       $c[$i] = ($N > 1 ? join(" ", @m[1..$#m]) : $m[0]) if $N;
    }
}
}
print join("\t", @c),"\n"' "$@"
}
compressed() {
    local d=$1
    d=${d%.tar.bz2}
    d=${d%.tar.gz}
    d=${d%.tgz}
    d=${d%.tar.xz}
    [[ $d != $1 ]]
}
kenplz() {
    local in=$1
    local order=${2:-5}
    local out=${3:-$in.arpa}
    local args="-o $order $kenplzarg"
    if compressed $in ; then
        cat_compressed | kenplz $args > $out
    else
        kenplz $args > $out < $in
    fi
    build_kenlm_binary -q 4 -b 4 -a 22 trie $out $in.ken
}
loadmethods() {
    lm=${1:-/local/graehl/time/MosesModel/model/europarl.ken}
    rules=/local/graehl/time/MosesModel/model/rules.db
    xmtbin=${xmt:-/home/graehl/x/Release/xmt/xmt}
    tmpout=/tmp
    mkdir -p $tmpout
    if [[ $dropcaches ]] ; then
        echo "dropping caches before each #1"
    else
        $xmtbin -D
        cat $lm $rules > /dev/null
    fi
    (for method in lazy populate-or-read read; do
         cmd="$xmtbin --input=/home/graehl/tmp/rep.txt --pipeline=1best --config=/local/graehl/time/MosesModel/config/XMTConfig.yml --log-config=/home/graehl/warn.xml --phrase-decoder-1best.heuristic-arc-pruning-num-best=20 --phrase-decoder-1best.heuristic-arc-pruning=1 --phrase-decoder-1best.heuristic-arc-pruning=lm-score-first-word-only --phrase-decoder-1best.stack-limit=40 --nonresident-grammar.num-rules=30 --nonresident-grammar.rule-filter=topn-lm --srilm.file-path=$lm --srilm.load-method=$method -o $tmpout/out.$method"
         if [[ $method = lazy ]] ; then
             echo $cmd
         fi
         for i in `seq 1 3`; do
             echo -n "#$i $method: "
             if [[ $dropcaches ]]; then
                 dropcaches
             fi
             /usr/bin/time -f '%Es %Uuser %Ssystem - %Mkb peak' -- $cmd 2>&1; done
     done ) | tee $tmpout/time.methods
}
add_ldpath $BOOST_LIBDIR
SDL_HADOOP_ROOT=$SDL_EXTERNALS_PATH/libraries/hadoop-0.20.2-cdh3u3
if [[ -d $SDL_HADOOP_ROOT ]] ; then
    SDL_HADOOP_JAVA=$SDL_HADOOP_ROOT/jdk1.8.0_60
    export JAVA_HOME=$SDL_HADOOP_JAVA
    export SDL_HADOOP_JAVA
    export PATH=$JAVA_HOME/bin:$PATH
fi
CT=${CT:-`echo ~/c/ct/main`}
crontest1() {
    (name=${1:-mert-update3}; export LW_SHERPADIR=rename; export LW_RUNONEMACHINE=1; export CT=$CT/..; ( cd $CT; cd main; nohup perl Shared/Test/bin/CronTest.pl $CT -name "$name" -steps tests -tests kraken/kraken/CompoundSplit kraken/kraken/CountFeats kraken/kraken/Merge kraken/kraken/Preproc kraken/kraken/Xiphias kraken/kraken/XTrain-EJ kraken/kraken/XTrain kraken/kraken/XTrain_S2T -parallel 20 ) &>crontest.$name.log ) &
}

CLANGFORMATCCMD=${CLANGFORMATCCMD:-clang-format}
GITCMD=${GITCMD:-git}

iscppfile() {
    local ext=$(echo "$1" | cut -d'.' -f2)
    [[ $ext = cpp ]] || [[ $ext = hpp ]] || [[ $ext = ipp ]] || [[ $ext = h ]] || [[ $ext = c ]] || [[ $ext = cxx ]] || [[ $ext = hxx ]] || [[ $ext = hh ]] || [[ $ext = cc ]]
}
gitfilesize() {
    ${GITCMD:-git} cat-file -s ${newref:-HEAD}:"$1"
}
gitcatnew() {
    ${GITCMD:-git} cat-file blob ${newref:-HEAD}:"$1"
}
checkclangformat() {
    local rdir=/tmp/clang-format
    mkdir -p $rdir
    chmod 755 $rdir
    if iscppfile "$1" ; then
        echo2 -n "checking clang-format for $1 ... "
        repl1=`mktemp /tmp/repl1.XXXXXX`
        repl2=`mktemp /tmp/repl2.XXXXXX`
        rclangformatdir=${rclangformatdir:-/home/graehl/x}
        gitcatnew "$1" | ${CLANGFORMATCCMD:-clang-format} -assume-filename="$rclangformatdir/assume.cpp" -style=file -output-replacements-xml > $repl1
        if [[ -s $repl1 ]] ; then
            cat $repl1 | perl -ne 'print if m{replacement offset} && !m{> *\\?&#10; *</replacement>}' > $repl2
            if [[ -s $repl2 ]] ; then
                replf=$rdir/`basename $1`.replacements
                echo "ERROR: $1 needs clang-format - ssh git02 cat $replf"
                mv $repl1 $replf
                chmod 644 $replf
                rm -f $repl2
                return 1
            fi
        fi
        echo2 OK
        rm -f $repl1 $repl2
    fi
}

checkclangs() {
    (
        local ERR=0
        clangformatsz=`gitfilesize .clang-format`
        checkformat=
        if [[ $clangformatsz -gt 1 ]] ; then
            checkformat=1
        fi
        IFS=$'\n' # make newlines the only separator

        rclangformatdir=`mktemp -d /tmp/clang-format.XXXXXX`
        gitcatnew .clang-format > $rclangformatdir/.clang-format
        for file in $(${GITCMD:-git} diff --stat --name-only --diff-filter=ACMRT ${oldref:-HEAD^1}..${newref:-HEAD});
        do
            size=`gitfilesize "$file"`
            if [[ ! -z $size ]]; then
                if [[ $checkformat ]] ; then
                    checkclangformat "$file" || ERR=1
                fi
            fi
        done
        rm -rf $rclangformatdir
        exit $ERR
    )
}
