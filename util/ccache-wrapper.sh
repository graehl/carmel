name=`basename $0`
ccbasename=${name#ccache-}
ccpre=/local/gcc
cc=$ccpre/bin/$ccbasename
addld() {
    LD_LIBRARY_PATH+=":$1"
    LD_LIBRARY_PATH=${LD_LIBRARY_PATH#:}
    export LD_LIBRARY_PATH
    LD_RUN_PATH+=":$1"
    LD_RUN_PATH=${LD_RUN_PATH#:}
    export LD_RUN_PATH
}
if [[ -x $cc ]] ; then
    addld $ccpre/lib
    addld $ccpre/lib64
else
    cc=$ccbasename
fi
if [[ $gccfilter ]] && [[ -x `which gccfilter 2>/dev/null` ]] ; then
    set -x
  exec gccfilter ${gccfilterargs:- -r -w -n -i} ccache $cc "$@"
else
    set -x
  exec ccache $cc "$@"
fi
