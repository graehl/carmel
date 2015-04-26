gccprefix=${gccprefix:-/local/gcc}
if [[ $NOLOCALGCC = 1 ]] ; then
    gccprefix=
fi
if [[ -d $gccprefix ]] ; then
    export PATH=$gccprefix/bin:$PATH
    LD_RUN_PATH+=":$gccprefix/lib64"
    export LD_RUN_PATH=${LD_RUN_PATH#:}
    LD_LIBRARY_PATH+=":$gccprefix/lib64"
    export LD_LIBRARY_PATH=${LD_RUN_PATH#:}
fi
