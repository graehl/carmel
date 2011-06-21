#!/bin/bash
UNAMEA=${UNAMEA:-`/bin/uname -a`}
case "$UNAMEA" in
    *x86_64*)	ON64=1 ;;
esac

HOSTNAME=${HOSTNAME:-`hostname`}
HOSTNAME=${HOSTNAME%.isi.edu}
HOSTNAME=${HOSTNAME%.usc.edu}

function add_ldpath
{
if [ "$1" ] ; then
#  export LD_LIBRARY_PATH="$1:$LD_LIBRARY_PATH"
#  export LIBRARY_PATH="$1:$LIBRARY_PATH"
  export LDFLAGS="$LDFLAGS -L$1 -Wl,-rpath,$1"
fi
}

function add_pypath
{
    local f
    for f in "$@"; do
        if [[ -d $f ]] ; then
            if ! echo $PYTHONPATH | grep -q $f ; then
                export PYTHONPATH+=":$f"
            fi
        fi
    done
}
function prepend_path
{
 if [ "$1" ] ; then
 local prefix=`echo $1`
 local sublib=$prefix/lib
 local subpy=$sublib/python
 local subbin=$prefix/bin
 local subman=$prefix/man
 local subinc=$prefix/include
if [ "$HOST" != maybe ] ; then
 mkdir -p $prefix
 mkdir -p $sublib
 mkdir -p $subbin
 mkdir -p $subman
 mkdir -p $subinc
fi
 export PATH="$subbin:$PATH"
 export MAN_PATH="$subman:$MAN_PATH"
 export C_INCLUDE_PATH="$subinc:$C_INCLUDE_PATH"
 export CPPFLAGS="-I$subinc $CPPFLAGS"
 add_ldpath $sublib
 add_pypath $subpy $subpy/bzrlib ${subpy}2.6/site-packages
 if [ "$ON64" ] ; then
  local sublib64=$prefix/lib64
  mkdir -p $sublib64
  add_ldpath $sublib64
 fi
 fi
}

prepend_paths() {
dirs="$*"

[ "$nohostbase" ] || dirs="~graehl/isd/$HOSTNAME $dirs"

for f in $dirs; do
#echo prepending path base: $f
prepend_path $f
done
}

prepend_paths $*
