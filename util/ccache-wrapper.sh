name=`basename $0`
ccbasename=${name#ccache-}
ccpre=/local/gcc
cc=$ccpre/bin/$ccbasename
if ! [[ -x $cc ]] ; then
    ccpre=/usr/local
    cc=$ccpre/bin/$ccbasename
fi
addld() {
    LD_LIBRARY_PATH="$1:$LD_LIBRARY_PATH"
    LD_LIBRARY_PATH=${LD_LIBRARY_PATH%:}
    export LD_LIBRARY_PATH
    LD_RUN_PATH="$1:$LD_RUN_PATH"
    LD_RUN_PATH=${LD_RUN_PATH%:}
    export LD_RUN_PATH
}
if [[ -x $cc ]] ; then
    addld $ccpre/lib
    addld $ccpre/lib64
    export PATH=$ccpre:$PATH
else
    cc=$ccbasename
fi
if [[ $gccfilter ]] && [[ -x `which gccfilter 2>/dev/null` ]] ; then
  exec gccfilter ${gccfilterargs:- -r -w -n -i} ccache $cc "$@"
else
  exec ccache $cc "$@"
fi
