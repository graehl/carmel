ghost=c-graehl
nchanged() {
    grep 'file changed' "$@"
    grep -c 'file changed' "$@"
    grep -c 'files changed' "$@"
}
justbleu() {
    perl -ne 'print "$1-ref %BLEU: $2 (len=$3)\n" if (/BLEUr(\d+)n\S+ (\S+) .*lengthRatio: (\S+)/)' "$@"
}
shortenbleu() {
    perl -pe '$_ = "$1-ref %BLEU: $2 (len=$3)\n" if (/BLEUr(\d+)n\S+ (\S+) .*lengthRatio: (\S+)/)' "$@"
}
attributexml1() {
    perl -e '$a=shift;while(<>) { if (/$a=['"'"'"]?([^'"'"'" >]+)/) { print "$1\n"; exit} }' "$@"
}
srctrgxml() {
    src=`attributexml1 foreign "$@"`
    trg=`attributexml1 native "$@"`
    [[ $src ]] && echo src=$src trg=$trg quality=${quality:-1}
}
bleuscore() {
    echo "bleu4: $*"
    perl /c04_data/coretraining/test/manager/main/kraken/xeval/App/bin/bleu.pl --metric=bleu -n 4 -hyp "$@"
}
setoutdir() {
    outdir=${outdir:-`echo ~/projects/unkweight`}
    mkdir -p $outdir
}
tee12() {
    local out=$1
    shift
    if [[ $force ]] || ! [[ -s $out ]] ; then
        "$@" 2>&1 | tee $out
    else
        cat < $out
    fi
}
savestdout() {
    local out=$1
    shift
    if [[ $force ]] || ! [[ -s $out ]] ; then
        "$@" > $out
    fi
}
testbleu() {
    (set -e
     echo cd $f
     cd $f
     f=`pwd`
     xmt=${xmt:-/home/graehl/pub/cxmt/xmt.sh}
     quality=${quality:-1}
     loglvl=${loglvl:-warn}
     logconfig=${logconfig:-/home/graehl/log/$loglvl.xml}
     NPROC=12
     metaxml=`find . -name decoder-conf.xml`
     require_file "$metaxml"
     srctrgxml $metaxml
     setoutdir
     basef=q${quality}_`basename $f`
     out=$outdir/$basef.hyp
     refs=
     for s in '' .0 .1 .2 .3 .4 .5 .6 .7 .8; do
         if [[ -s $f/meta/test$s.$trg ]] ; then
             cat $f/meta/test$s.$trg > $out$s.ref
             refs+=" $out$s.ref"
         fi
     done
     name="$basef"
     force=
     time savestdout $out $xmt -c $f/config/XMTConfig.yml -p decode_q$quality -i $f/meta/test.$src --detokenizer.output-type=string  --log-config $logconfig -n $NPROC
     force=1 tee12 $out.bleu4 bleuscore $out $refs | (echo -n "$name: "; justbleu)
     time savestdout $out.unk100 $xmt -c $f/config/XMTConfig.yml -p decode_q$quality -i $f/meta/test.$src --detokenizer.output-type=string --log-config $logconfig --grammar_q$quality.feature-weights.unk-weight=100 -n $NPROC
     force=1 tee12 $out.unk100.bleu4 bleuscore $out.unk100 $refs  | (echo -n "unk100.$name: "; justbleu)
     diff $out $out.unk100 > $out.unk100.diff
     /usr/bin/diffstat < $out.unk100.diff
     )
}
testbleus() {
    for f in "$@"; do
        echo ------------------- $f
        (
            set -e
        testbleu $f
        echo
        echo $f
        ) || true
    done
}
latest_dir() {
    for f in "$@"; do
    f=${f%_0}
    for i in `seq 9 0`; do
        if [[ -d ${f}_$i ]] ; then
            echo ${f}_$i
            break
        fi
    done
    echo $f
    done
}
qbleus() {
    (
    setoutdir
    cd /build/data
    echo quality ${quality:=1}
    force=1 tee12 $outdir/q$quality.bleus.txt testbleus `latest_dir $1*SAS*_5_4_x_1 | sort | uniq`
    )
}

unkweights() {
    for f in */config/XMTConfig.yml; do g=`dirname $f`;g=`dirname $g`; echo $g; grep unk-weight: $f; done
}
macmdb() {
(
export TERM=dumb
set -x
set -e
ldapmdb=src/openldap/libraries/liblmdb
mdb=c/mdb/libraries/liblmdb/
lmdb=c/lmdb
cp $HOME/$mdb/mdb_from_db.1 $HOME/$mdb/mdb_from_db.c $HOME/$ldapmdb
#cp $lmdb/CMakeLists.txt $mdb
cd ~
ln -sf ~/$mdb/*.c ~/$lmdb/mdb
cd ~/c/mdbbuild
set -x
cmake -G 'Unix Makefiles' ../lmdb
make -s -j2
for f in ~/$mdb/*.c; do
    rm ~/$lmdb/mdb/`basename $f`
    cp $f ~/$lmdb/mdb/
done
cp *mdb* ~/bin
ll=$XMT_EXTERNALS_PATH/libraries/lmdb
lib=$ll/lib
bin=$ll/bin
mkdir -p $lib
mkdir -p $bin
cp *.dylib *.a $lib
cp mdb_* $bin
cp -f mdb_* ~/bin/
cd $lib
git add *.dylib *.a
cd $bin
git add mdb_*
)
}
upmdb() {
(
#set -x
set -e
macmdb
HOME=$(echo ~)
c=$(echo ~/c)
mdb=c/mdb/libraries/liblmdb/
cd $HOME/$mdb
git commit --allow-empty -a -C HEAD --amend
#git pull --rebase
#(echo "";gitshowcommit) > ORIGIN.git.commit
libver=lmdb
smdb=c/xmt-externals-source/$libver
cp ORIGIN.git.commit *.1 *.c *.h $HOME/$smdb/mdb/
cd $HOME/$smdb/mdb
#git pull --rebase
git add *.c *.h ORIGIN.*

xlib=$XMT_EXTERNALS_PATH/../FC12/libraries
slib=$XMT_EXTERNALS_PATH/../Shared/cpp
ll=$xlib/lmdb/lib
bl=$xlib/lmdb/bin
mkdir -p $ll
mkdir -p $bl
chost=${mdbhost:-c-jmay}
cb=c/build-$libver
rm -f $HOME/$smdb/*.o
rm -f $HOME/$smdb/*.a
scp -r $HOME/$smdb $chost:c/
banner building remotely on $chost
c-s "mkdir -p $cb;cd $cb; xmtcmake -G 'Unix Makefiles' ../$libver && make -j5 && cp mdb_* ~/bin"
set -x
scp $chost:$cb/\*.so  $ll
scp $chost:$cb/\*.a  $ll
scp $chost:$cb/mdb_\*  $bl
set +x
incl=$slib/libraries/lmdb/include
cp $HOME/$smdb/mdb/lmdb.h $incl
cd $ll
#git check-ignore -v *.so *.a || true
git add *.so *.a
cd $bl
git add mdb_*
git add $incl/*.h || true
if [[ $mend ]] ; then
    mend
fi
)
}
dumpdb() {
    (
        set -e
        db=$1
        shift
        mdb_from_db -l $db; mdb_from_db -T "$@" $db > $db.txt
        preview $db.txt
        mdb_from_db -l $db
    )
}

ruleversion() {
    (
        set -e
        d=${2:-`dirname $1`}
        b=`basename $1`
        db=$d/$b.unzip.db
        (gunzip -c "$@" || cat "$@") > $db
        (mdb_from_db -l $db; echo VERSION; mdb_from_db -T -s Version $db; echo HEADER; mdb_from_db -T -s Header $db  || echo No Header; echo RULES; mdb_from_db -T -s RulesDB $db) > $d/$b.txt
        echo
        echo  $d/$b.txt
        head -20 $d/$b.txt
    )
}
bmdb() {
    (
        cd ~/c/mdb/libraries/liblmdb/
        export TERM=dumb
        XMT_SHARED_EXTERNALS_PATH=${XMT_EXTERNALS_PATH}/../Shared/cpp
        dbroot=$XMT_EXTERNALS_PATH/libraries/db-5.3.15
        make mdb_from_db CC=${CC:-gcc} OPT="-O3" XCFLAGS="-I$dbroot/include" LDFLAGS+="-L$dbroot/lib" LDLIBS="-ldb"
        #gdb --fullname --args ~/c/mdb/libraries/liblmdb/mdb_from_db -T /tmp/foo.db
        if [[ "$*" ]] ; then
            /Users/graehl/c/mdb/libraries/liblmdb/mdb_from_db "$@"
        fi
        #mdb_dump -n -a /tmp/foo.mdb
    )
}

rclangformathost=${rclangformathost:-c-graehl}
rclangformatdir=${rclangformatdir:-/home/graehl/x}
rclangformat() {
    (set -o pipefail
     cat "$@" | ssh $rclangformathost "/usr/local/bin/clang-format -style=file -assume-filename=$rclangformatdir/assume.cpp $rclangformatargs"
     )
}
# in-place
irclangformat() {
    if ! [[ $"$@" ]] || [[ $* = - ]] ; then
        rclangformat
    else
        for f in "$@"; do
            local g=$f.clang-format
            if rclangformat $f > $g; then
                diff -u $f $g
                mv $g $f
                echo $f clang-format OK
            fi
        done
    fi
}

experiments() {
    for f in ${*:-`pwd`}; do
        experimentf $f/*/my.experiment.yml $f/my.experiment.yml
    done
}
tunes() {
    for f in ${*:-`pwd`}; do
        [[ -d $f/tune_slot ]] && f=$f/tune_slot
        [[ -d $f/tune_work ]] && f=$f/tune_work
        ag -G "iter_${iter}"'.*\.err.*\.'$parti ' peak |Segmentation |Converged|stddev=|elapsed|score\+0|intersections, |steps from origin' $f
    done
}
tunesw() {
    for f in ${*:-`pwd`}; do
        [[ -d $f/tune_slot ]] && f=$f/tune_slot
        [[ -d $f/tune_work ]] && f=$f/tune_work
        ag -i -G "iter_${iter}"'.*\.err.*\.'$parti 'egmentation |exit code|failed|warning|error' $f | grep -v 'used 0 for missing' | grep -v 'No weight defined'
    done
}
expclean() {
    for f in ${*:-`pwd`}; do
        expcleanf $f/*/my.experiment.yml $f/my.experiment.yml
    done
}
expcleanf() {
    for f in "$@"; do
        if [[ -f $f ]] ; then
            local dir=`dirname $f`
            rm -rf $dir/tune_slot/tune_work
        fi
    done
}
experimentf() {
    for f in "$@"; do
        if [[ -f $f ]] ; then
            local dir=`dirname $f`
            perl -e 'while(<>) { print "$2\t" if /([a-zA-Z_-]+)bleu: '"'?-?([0-9.]+)/ }" $f
            perl -e '$n = 0; $z = 0;while(<>) { chomp;
                      ++$n if '"/: '?-?[0-9.]/"'; ++$z if '"/: '?-?0+\.?0*'?$/"';
                     } print "$n\t$z\t"' $dir/config/weights.file.final
            echo $dir
        fi
    done
}
cmertdir=c/ct-archive/archive/3rdParty/mert
pubdir=/home/graehl/pub
gmerts() {
    (
    nobuild=1 cwithdir $cmertdir
   for DEBUG in 1 ''; do
        dname=
        [[ $DEBUG = 1 ]] && dname=.debug
        for SAN in thread address ''; do
            sanname=
            [[ $SAN ]] && sanname=.$SAN
            name=$pubdir/mert$dname$sanname
            ssh $ghost ". ~/.e;usegcc;cd $cmertdir; rm -f *.o; make CC=gcc DEBUG=$DEBUG SAN=$SAN && cp mert $name && ln -sf withlib.sh $name.sh"
    done
    done
    )
}
smert() {
    save12 ~/tmp/cmert cwithmertrun -x -f 0 /home/graehl/projects/sparse/weights /home/graehl/projects/sparse/${nbest:-named.2k} "$@"
    #nbest.2k
}
cmert() {
    #    makearg="XCFLAGS=-DMERT_SPARSE=${MERT_SPARSE:-1}"
    makearg=
    suf=
    arg=-x
    if [[ $named ]] ; then
        arg=-X
        suf=.named
        #init=initial.txt.19.sparse
    fi
    initarg=/home/graehl/projects/tune/tune_work/iter_0/${init:-initial.txt.19}
    if [[ $noinit ]] ; then
        initarg="-R 1"
    fi
    ${out12:-save12} ~/tmp/cmert.$SPARSE cwithmertrun  $arg $initarg -g 'SLTL_SL:::TrainingData'  /home/graehl/projects/tune/tune_work/iter_0/output.nbest/corpus.nbest$suf "$@"
    if [[ $l0bonus ]] ; then
        c-s "cp -a /home/graehl/c/ct-archive/archive/3rdParty/mert/mert /home/graehl/pub/mert-l0=${l0bonus}; ls ~/pub/mert*"
    fi
    if [[ $CC ]] ; then
        c-s "set -x; cp -a /home/graehl/c/ct-archive/archive/3rdParty/mert/mert /home/graehl/pub/mert-$CC"
    fi
}
l0s="0" # 1e-2 4e-2 1e-3 4e-3 4e-4 1e-5 4e-5 4e-6 1e-4 1e-6"
cmerts() {
    for f in $l0s; do
        out12=out12 SAN= l0bonus=$f CC=ccache-gcc CXX=ccache-g++ DEBUG= ASSERT= cmert
        if [[ $f = 0 ]] ; then
            c-s 'cp pub/mert-gcc pub/mert2; stripx pub/mert2'
        fi
    done
}
benchmert() {
for j in 6 8 10 12 14 16 18 32 64; do
echo2 ===== j=$j
for CC in gcc; do
echo2
echo2 CC=$CC j=$j
~/pub/mert-$CC -x -f 0 /home/graehl/projects/tune/tune_work/iter_0/initial.txt.19 /home/graehl/projects/tune/tune_work/iter_0/output.nbest/corpus.nbest -v 0 -0 1e-4 -j $j >/dev/null
done
done
}
stevemert() {
    cwithmertrun -f 13 /home/graehl/bugs/mert2/initial.txt.3 /home/graehl/bugs/mert2/corpus.nbest "$@"
}
cwithmertrun() {
    (
                set -x
        cwithdir $cmertdir "time $pre /home/graehl/c/ct-archive/archive/3rdParty/mert/mert $*"
    )
}
cwithmertrunct() {
    (
        c-s "time ~/bin/mertct $*"
        cwithmertrun "$@"
    )
}
cwithdir() {
    (set -e;
     local d=$1
     shift
     rm -f ~/$d/*.o mert
     cd ~
     local dst=$d
     local rdir=$d
     if [[ -d $d ]] ; then
         dst=`dirname $d`
     else
         rdir=`dirname $d`
     fi
     if [[ $l0bonus ]] ; then
     if [[ -f $d/mert.c ]]; then
         perl -i -pe 's/double l0_bonus = [^;]*;/double l0_bonus = '"$l0bonus"';/' $d/mert.c
         grep 'l0_bonus =' $d/mert.c
     fi
     fi
     c-s mkdir -p $dst
     set -x
     dstf=$dst/`basename $d`
     scp $d/*.c $d/*.h $d/*.cc $d/Makefile $chost:$dstf/
     if [[ $scan ]] ; then
         scanpre="scan-build -k "
     fi
     if [[ $SPARSE ]] ; then
         sparsearg="SPARSE=$SPARSE"
     else
         sparsearg=
     fi
     [[ $nobuild ]] || c-s "cd $rdir; set -x; $scanpre make $target $sparsearg DEBUG=$DEBUG ASSERT=$ASSERT CC=${CC:-ccache-gcc} SAN=$SAN $makearg && $*"
    )
}
densesparse="sparse"
theirbaseline() {
    perl -ne 'if (/\[(.*-baseline)\]/) { print $1; exit }' "$@"
}
initweights() {
    set -e
    local d=$1/config/weights.file.final
    local s=$2/config/weights.file.final
    require_files $s $d
    [ -f init.sparse.yml ] || cp $s init.sparse.yml
    [ -f init.dense.yml ] || cp $d init.dense.yml
    wc -l init.sparse.yml init.dense.yml
}
baselinetune() {
    lntune "$@"
    baseline=`theirbaseline $theirs`
    echo $baseline $theirs $mine
}
nocommands() {
    perl -e 'while(<>) { $sec = $1 if /^\[(.*)\]/; print ($sec eq "commands" ? "# $_" : $_) }' "$@"
}
mycommands() {
    set -e
    [[ $baseline ]] || baselinetune "$@"
    if ! [[ $tunestart ]] ; then
        if [[ $lp = eng-swe ]] ; then tunestart=/c01_data/mdreyer/xmt-test/eng-swe/20141031-baseline-retune-pro1-i10/tune_slot/tune_work/iter_9/weights.yaml
        else
            tunestart=/c01_data/mdreyer/xmt-test/pol-eng/data/weights-init.yml
        fi
    fi
    cp $tunestart $mine.weights-init.yml
    xmt=${xmt:-/c01_data/mdreyer/software/coretraining/main/LW/CME/xmt-rpath}
    showvars_required baseline theirs mine tunestart xmt
    require_file $xmt
    xmt=`abspath $xmt`
    require_file $theirs $tunestart
    for sd in $densesparse; do
        origweights=$lp/init.$sd.yml
        require_file $origweights
        origweights=`abspath $origweights`
        for l0 in $l0s; do
            echo $l0 > STDERR
            mert=$(echo ~graehl/pub/mert-l0=$l0)
            require_file $mert
            name=$sd-l0_$l0
            cat<<EOF
[$name]
task=xtrain
inherit=$baseline
mert-bin=/home/graehl/pub/mert-l0=$l0
prep_slot=[$baseline]/prep_slot
wa_slot=[$baseline]/wa_slot
cap_slot=[$baseline]/cap_slot
rules_slot=[$baseline]/rules_slot
db_slot=[$baseline]/db_slot
lm_slot=[$baseline]/lm_slot
tuning-max-pro-iter=0
tuning-max-mert-iter=30
tuning-orig-weights-file=$origweights
xmt-bin=$xmt
decoding-memory-request-gb=28
num-decoder-threads=2
tuning-num-pieces=10

EOF
        done
    done
    echo
    echo [commands]
    for sd in $densesparse; do
        for l0 in $l0s; do
            name=$sd-l0_$l0
            echo $name
        done
    done
}
lntune() {
    set -e
    src=$1
    trg=${2:-lntune ita eng [basedir]}
    lp=$1-$2
    base=${3:-/c01_data/mdreyer/xmt-test}
    origin=$base/$lp
    fromapex=$origin/kraken.$lp.apex
    require_dir $base/$lp
    require_file $fromapex
    [[ -d $lp ]] || cp -nRs $base/$lp .
    require_dir $lp
    theirs=$lp/their.apex
    cp $fromapex $theirs
    mine=$lp/my.$densesparse.apex
}
mytune() {
    set -e
    [[ $baseline ]] || baselinetune "$@"
    (nocommands $theirs; mycommands) > $mine
    cat $mine
    echo $mine
}

showtune() {
    local report=tunereport.`filename_from "@@"`
(
    local tunedir
    for tunedir in "$@"; do
        #$tunedir/tune_slot/tune_work/report.txt
        echo -n "$tunedir - eval:"
        justbleu  $tunedir/evalresults/test-results.txt
    done
    for tunedir in "$@"; do
        echo -n "$tunedir - eval: "
        avgperseg $tunedir/eval_slot/trans_work/translate_xmt/test/condor/translate*.err.*
        echo -n '            - tune:'
        avgperseg $tunedir/tune_slot/condor/tune/iter_0/condor_decode_job/tune*.err.*
    done
) | tee $tunereport
preview $tunereport
}
hgsrcs() {
    perl -ne 'print "#include <sdl/Hypergraph/src/$1.cpp>\n" if /sdl_hgTransform\((\S*)\)/' $xmtx/sdl/Hypergraph/CMakeLists.txt
}
forhgsrcs() {
    perl -ne 'print "  x($1) \\\n" if /sdl_hgTransform\((\S*)\)/' $xmtx/sdl/Hypergraph/CMakeLists.txt
}
