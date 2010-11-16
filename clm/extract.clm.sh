#!/bin/bash
. ~graehl/isd/hints/bashlib.sh
. ~graehl/t/utilities/make.lm.sh

makebig=${makebig:-1}
makelw=${makelw:-1}
phrasal=${phrasal:-$d/phrasal-clm-events}
yield=${yield:$d/e-parse-yield.pl}
stripef=${stripef:-$d/stripEF.pl}
[ -x $stripef ] || stripef=cat

extract=${extract:-/auto/nlg-02/graehl/xrs-extract/bin/extract}
#`which extract`
extract=`realpath $extract`

numclass=${numclass:-1}
[ "$numclass" = 1 ] && enumclass=1
[ "$numclass" = 1 ] && fnumclass=1
showvars enumclass fnumclass binary

sleeptime=0

case  `hostname | /usr/bin/tr -d '\r\n'` in
    hpc*) sleeptime=1
esac

function delay {
    sleep $sleeptime
}

function one {
    perl -pe 's/$/ 1/' "$@"
}
function fsos {
    #loose: works on finished srilm w/ or w/o F prefix, thus subject to error if foreign word looks like a logprob and there's no backoff
    perl -i -pe 's#(\s+)<(/?)s>(F?)(\s*-?\d*\.?\d*)$#$1<${2}foreign-sentence>$2$3#o' "$@"
}
function bocounts {
    perl -ne 'chomp;@a=split;for (0..$#a) { $a[$_] =~ s/\d/\@/g if $_==$#a && $ENV{fnumclass} || $_<$#a && $ENV{enumclass}};$a[$#a]=~s/^\<(\/)?s\>F$/<${1}foreign-sentence>F/o;for (0..$#a) { print join(" ",@a[$_..$#a]),"\n" }' "$@"
}
function filt {
    egrep '^[0-9]' -- "$@" | cut  -f4- | bocounts | one
}
function Evocab {
    perl -ne '$e{$1}=1 while /(\S+E)($| )/g;END{print "$_\n" for (keys %e)}' "$@"
}

function clm_from_counts {
    local count=${1:?'Ea Eb Fc x' e.g. x=1 time, clm ngram counts.  E... are all nonevents (context), F... is predicted.  env N=3 means trigram}
    shift
    local ngram=${N:-3}
    local sri=${1:-$count.$ngram.srilm}
    shift
    local Ev=$count.Ev
    #`mktemp`
    local unkargs="-unk"
    local ngoargs="-order $ngram"
    local noprune="-minprune $((ngram+1))"
    local smoothargs="-wbdiscount"
#kn discount fails when contexts are not events.
#    set -x
    if ! newer_than=$count skip_files 2 $sri ; then
        catz $count | Evocab > $Ev
        ngram-count $ngoargs $unkargs $smoothargs $noprune -sort -read $count -nonevents $Ev -lm $sri $*
        show_outputs $Ev $sri
        if [ "$stripEF" ] ; then
            delay
            mv -f $sri $sri.EF
            delay
            $stripef < $sri.EF > $sri && bzip2 -f $sri.EF && show_outputs $sri
        fi
        delay
    fi
#    set +x
#    rm $Ev
}
###

function main {
#TODO: pipe preproc straight to ngram-count w/o intermediate file?
    set -x
    grf=${grf:-giraffe}
    ix=${ix:-training}
    ox=${ox:-x}
    chunksz=${chunksz:-100000}
    if [ "$binary" ] ; then
        chunksum="binary (phrasal) "
    else
        nl=${head:-`nlines $ix.e-parse`}
        if [ $chunksz -gt $nl ] ; then
            chunksz=$nl
        fi
        nc=$(((nl+chunksz-1)/chunksz))
        chunksum="$((nc)) chunks of $chunksz ea. for $nl lines. "
    fi
    N=${N:-3}
    bign=${bign:-0}
    banner "$chunksum$N-gram i=$ix o=$ox bign=$bign"
    set -e
    lfiles=""
    rfiles=""
    rm -f $ox.c*.{left,right}

    if ! skip_files 1 $ox.left.bz2 $ox.right.bz2 ; then
        if [ "$binary" ]; then
            $phrasal -w $ox.left -W $ox.right -N $N -r $ix
            for d in left right; do
                bocounts $ox.$d | one | bzip2 -c > $ox.$d.bz2 && rm $ox.$d
            done
        else
            for i in `seq 1 $nc`; do
                el=$((chunksz*i))
                sl=$((el-chunksz+1))
#    showvars_required nc chunksz el sl
                oxi=$ox.c$i
    #empirically (100sent) verififed to not change uniqued locations over minimal: -G - (wsd), $bign>0, -T
                echo $extract "$@" -s $sl -e $el -w $oxi.left -W $oxi.right -N $N -r $ix -z -x /dev/null  -g 1 -l 1000:$bign -m 5000 -O -i -X
            # -U 0 would disable ambiguous unaligned word attachment but we want to use the same setting as in real pipeline extraction
            done | $grf - > log.extract.`filename_from $ox`.giraffe 2>&1

            header DONE WITH GHKM
            delay

            for d in left right; do
                (
                    dpz=$ox.$d.bz2
                    dfiles=$ox.c*.$d
                    showvars_required dfiles
                    sort $dfiles | uniq | filt | bzip2 -c > $dpz
                    tbz=$ox.$d.ghkm.tar.bz2
                    rm -f $tbz
                    tar -cjf $tbz $dfiles && rm $dfiles
                )
            done

        fi
    fi

    for d in left right; do
        dpz=$ox.$d.bz2
        ulm=$ox.$N.srilm.$d
        stripEF=1 clm_from_counts $dpz $ulm
    done
    for d in left right; do
    #redundant loop, but get srilm out no matter what fails in conversion
        ulm=$ox.$N.srilm.$d
        if [ "$makelw" = 1 ] ; then
            outl=`LWfromSRI $ulm`
            newer_than=$ulm skip_files 3 $outl || lwlm_from_srilm $ulm $outl
        fi
        if [ "$makebig" = 1 ] ; then
            outb=`bigfromSRI $ulm`
            newer_than=$ulm skip_files 4 $outb || ngram=$N biglm_from_srilm $ulm $outb
        fi
    done
#wait
}

[ "$nomain" ] || main "$@"; exit
