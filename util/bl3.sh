bl3() {
    (
        set -e
        set -x
        obasep=${obase:-bl3}
        for tune in 0 1; do
            i=1
            fs=
            bs=
            if [[ $tune = 1 ]] ; then
                tunere='\S+\s+'
                title="overfit "
                ot=
            else
                tunere=
                title="held-out "
                ot=".heldout"
            fi
            title+="tune BLEU vs epoch"
            for f in "$@" ; do
                pushd $f
                i=$((i+1))
                name=$(val1 $f $i)
                f=$(name1 $f)
                if [[ $name = $i ]] ; then
                    if [ -f name ] ; then
                        name=`cat name`
                        name=$(echo $name)
                    fi
                fi
                #[ -f epoch.scores ] ||
                mira-sum-time
                eot=epoch$ot.bleu
                #FIXME: syntax error
                #perl -ne 'print "$1\t$2\n" if /(\d+)\s+'$tunere'([\d.]+)/' epoch.scores > $eot
                fs+=" $f/$eot"
                popd
                bs+=" $i $name"
            done
            obase=$obasep$ot
            joinleft --npad=1 --padval='""' --nosort $fs > $obase.data
            tailn=30 preview $obase.data
            mv $obase.png $obase.png.bak || true
            showvars_required fs bs
            title=$title data=$obase.data obase=$obase xlbl="MIRA epoch" graph3 $bs
        done
        [[ $noview ]] || firefox $obase{,.heldout}.png
    )
}
