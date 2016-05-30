name=`basename $0`
d=`dirname $0`
if [[ $d != . ]] ; then
  export PATH=`dirname $0`:$PATH
fi
ccbasename=${name#ccache-}
CCACHE_DIR=${CCACHE_DIR:-/local/graehl/ccache}
mkdir -p $CCACHE_DIR || CCACHE_DIR=
if [[ -d $CCACHE_DIR ]] ; then
    export CCACHE_DIR=$CCACHE_DIR
fi
exec ccache $ccbasename "$@"
