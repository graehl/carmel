#!/bin/bash
BLOBS=${BLOBS:-/home/nlg-01/blobs}
[ -d $BLOBS ] || BLOBS=~/blobs
export BLOBS
d=`dirname $0`
if [ -f $BLOBS/bashlib/unstable/bashlib.sh ] ; then
 . $BLOBS/bashlib/unstable/bashlib.sh
else
 if [ -f $d/bashlib.sh ] ; then
  . $d/bashlib.sh
 fi
fi

set -e

function myusage {

echo2 produces '${outbase}{unit_,}_{lhs,root}_{count,prob}{_it#,}{_add#,}' files to be later
echo2 added to rules files
echo2
echo2 submits parallel jobs with '$qsub' by default, with logs in '$logdir'
echo2
echo2 expects input: '${inbase}${forests}, ${inbase}${lhsnorm}, and ${inbase}${rootnorm}'
echo2
echo2 Set environment variables to override params.
echo2
echo2 note: outviterbi 1 means output viterbi derivations.
echo2
echo2 see http://twiki.isi.edu/NLP/ForestEMButton for documentation
echo2
echo2 an addfield.sh script will be created for attaching attributes by name, e.g. 
echo2 unit_count_it1 unit_lhs_count_it2 root_count_it2 root_count root_prob_add20 lhs_prob_it1_add20
echo2 note: unit_count_it1 is a special case where lhs or root "doesn't" matter
echo2
echo2 since addk smoothing can be done later by addfield.sh, and dirichlet prior is probably not so useful, leave them at default=0
echo2
}

getrealprog

logdir=${logdir:-./logs}
inbase=${inbase:-extract.}
forests=${forests:-deriv}
lhsnorm=${lhsnorm:-lhs_norm}
rootnorm=${rootnorm:-root_norm}
addksmoothing=${addksmoothing:-0}
dirichletprior=${dirichletprior:-0}
maxiter=${maxiter:-30}
tmpdir=${tmpdir:-/tmp/}
outbase=${outbase:-emtrained/}
checkpointfreq=${checkpointfreq:-1}
qsub=${qsub:-$d/qsh -jobsdir=$logdir -mem=3000mb}
forestemblob=${forestemblob:-$BLOBS/forest-em/unstable}
forestem=${forestem:-$forestemblob/forest-em$bin_suffix}
forestemargs=${forestemargs:---forest-tick-period 10000 --max-forest-nodes 2m --max-normgroup-size 45m}
outviterbi=${outviterbi:-1}

function showparams {
 echo2
 echo2 THESE CAN BE SET AS ENVIRONMENT VARIABLES: 
 showvars inbase outbase forests lhsnorm rootnorm maxiter tmpdir addksmoothing dirichletprior checkpointfreq qsub forestem forestemargs outviterbi logdir forestemblob bin_suffix
 echo2
}

function checkoutbase {
 outbaseorig=outbase
 outbase=${outbase%/}
 if [ $outbase != $outbaseorig ] ; then
  mkdir -p $outbase
 fi
 if [ -d $outbase ] ; then
  outbase="$outbase/"
 else
  outbasedir=`dirname $outbase`
  mkdir -p $outbasedir
  touch $outbase && rm $outbase || die make the parent directories for $outbase yourself and try again.
 fi
}

if [ "$*" ] ; then
 myusage
 showparams
 exit
fi
mkdir -p $logdir

checkoutbase
showparams
showparams > $logdir/forest-em-button.params
[ -d $tmpdir ] || mkdir -p $tmpdir || die failed to make $tmpdir

inabs=`abspath $inbase`

for init in unit_ ""; do
 if [ "$init" = unit_ ] ; then
  initarg="--initial-1-params"
#  maxiter_fornow=1
 maxiter_fornow=$maxiter
 else
  initarg=''
  maxiter_fornow=$maxiter
 fi
 for norm in lhs root; do
  eval normfile=\$${norm}norm
  absnorm=${inabs}${normfile}
  normarg="--normgroups-file $absnorm"
  runoutput=${outbase}${init}${norm}
  if [ $outviterbi = 1 ] ; then
   viterbiarg="--checkpoint-viterbi-per-examples 1 --outviterbi-file ${runoutput}_viterbi"
  else
   viterbiarg=''
  fi
  [ -f ${inabs}$forests ] && [ -f $absnorm ] || die forests/normgroups ${inabs}$forests and ${inabs}$normfile not found
    logfile=${runoutput}.log
  echo2
  echo2 init $init initarg $initarg norm $norm normarg $normarg runoutput $runoutput log $logfile
  $qsub $forestem --forests-file ${inabs}${forests} $normarg --max-iter $maxiter_fornow --prior-counts-per $dirichletprior --add-k-smoothing $addksmoothing --outparam-file ${runoutput}_prob --outcounts-file ${runoutput}_count $initarg $viterbiarg --watch-period $checkpointfreq --checkpoint-parameters --checkpoint-prefix $runoutput --tempfile-prefix $tmpdir $forestemargs --log-file $logfile
 done
done
echo2
outscript=${outbase}addfield.sh
addfieldpl=$d/addfield.pl

 for norm in lhs root; do
  eval normfile=\$${norm}norm
  absnorm=${inabs}${normfile}
  addfieldnorms="$addfieldnorms -${norm}-norm $absnorm"
  normarg="--normgroups-file $absnorm"
 done
outabs=`abspath $outbase`
cat <<EOF > $outscript
#!/bin/bash
$addfieldpl -altforestem $forestem $addfieldnorms -basename $outabs \$*
EOF
chmod a+x $outscript
#cat $outscript
echo2 use \"cat rules \| $outscript -f unit_lhs_prob_it1 \> rules.with.unit_lhs_prob_it1\" to create a rules file with unit_lhs_prob_it1, -c if you want it compressed
echo2
echo2 check on job status with $d/cj -j $logdir -n 4
echo2
exit
