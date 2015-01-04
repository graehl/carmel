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
densesparse="sparse"
theirbaseline() {
    perl -ne 'if (/\[(.*-baseline)\]/) { print $1; exit }' "$@"
}
initweights() {
    set -e
    local s=$1/config/weights.file.final
    local d=$2/config/weights.file.final
    require_files $s $d
    cp $s init.sparse.yml
    cp $d init.dense.yml
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
    l0s="0 4e-6 1e-5 4e-5 4e-4 1e-4"
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
    mine=$lp/my.apex
}
mytune() {
    set -e
    [[ $baseline ]] || baselinetune "$@"
    (nocommands $theirs; mycommands) > $mine
    cat $mine
    echo $mine
}
