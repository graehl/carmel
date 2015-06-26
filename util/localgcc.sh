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
gccprefix=${gccprefix:-/local/gcc}
appendld() {
    if [[ $lwarch = Apple ]] ; then
        DYLD_FALLBACK_LIBRARY_PATH+=":$1"
        export DYLD_FALLBACK_LIBRARY_PATH=${DYLD_FALLBACK_LIBRARY_PATH#:}
    else
        LD_RUN_PATH+=":$1"
        export LD_RUN_PATH=${LD_RUN_PATH#:}
        LD_LIBRARY_PATH+=":$1"
        export LD_LIBRARY_PATH=${LD_RUN_PATH#:}
    fi
}
if [[ $NOLOCALGCC = 1 ]] ; then
    gccprefix=
fi
if [[ -d $gccprefix ]] ; then
    export PATH=$gccprefix/bin:$PATH
    appendld "$gccprefix/lib64"
fi
export CXX=ccache-g++
export CC=ccache-gcc
