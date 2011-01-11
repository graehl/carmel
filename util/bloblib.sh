#####BLOB FUNCTIONS

set_latest_unstable() {
    rm -f latest;rm -f unstable
    ln -s $1 latest; ln -s $2 unstable
}

blob_new_latest() {
( set -e;
    local which
    for which in $*; do
     echo making new $which
     cd $BLOBS
     cd $which
     touch unstable
     newblob
     ls -l $BLOBS/$which
    done
)
}

last_v() {
  local v=$1
  blobtype=${blobtype:-latest}
echo2 using blobtype=$blobtype
  local d=`dirname -- $v`
    local b=`basename --  $v`
  echo2 last_v $v $d
  if [ "$d" = . ]; then
    echo $v
  else
  local gv=`echo $b | egrep -v '^(v[0-9].*|latest|unstable)$'`
    if [ "$gv" ] ; then
     echo $v
    else
      if [ -L $d/$blobtype ] ; then
        echo2 using $d/$blobtype for $v
#        ( cd $d; cd `readlink $d/$blobtype`; pwd  )
        echo $d/`readlink $d/$blobtype`
      else
       ls -d $d/v* 2>/dev/null | perl -e 'while(<>) {($base,$max)=($1,$2) if /(.*v)([\d.]+)$/ && $2 > $max};print "${base}$max\n"'
      fi
    fi
  fi
}

update_blob_text() {
  blobtype=${blobtype:-latest}
echo2 using blobtype=$blobtype on $*
  [ "$*" ] || return 1
  for f in $*; do
   is_text_file $f && perl -i -pe 's#(blobs|BLOBS)(/[a-zA-Z0-9_/.+\-]+/)(unstable|latest|v[\d.]+)\b#$1.$2.(readlink($ENV{BLOBS}.$2."'$blobtype'")||$1.$2.$3)#e;s#/home/hpc-22/dmarcu/nlg/blobs#/home/nlg-01/blobs#g;' $f
 done
}

update_blob_texts() {
  blobtype=${blobtype:-latest}
  blobtype=${1:-$blobtype}
  shift
    echo update_blob_texts using $blobtype
  local f="$*"
#
  [ "$f" ] || f=`find . -type f | fgrep -v make.sh | fgrep -v makeblob.sh`
  update_blob_text $f
}

update_blob_link() {
 local old=$1
  blobtype=${blobtype:-latest}
echo2 using blobtype=$blobtype
 [ -L $old ] || return 1
 local pointsto=`readlink $old`
 local new
#FIXME: handle arbitrarily deep vN.M
 local dir=`dirname -- $pointsto`
 if [ ! -d $pointsto -a ! -L $dir ] ; then
    if [ $dir != . ] ; then
     new=`last_v $dir`/`basename --  $pointsto`
    fi
 else
    new=`last_v $pointsto`
 fi
 echo2 latest $old = $new
 if [ "$new" -a "$new" != "$pointsto" ] ; then
  rm $old
  ln -s $new $old
  echo2 updated link: $old from $pointsto to $new
 fi
}

update_blob_links() {
 blobtype=${blobtype:-latest}
  blobtype=${1:-$blobtype}
echo2 using blobtype=$blobtype
 local f
  for f in `find . -type l`; do
   update_blob_link $f
  done
}

#run from blobdir
blobrunmake() {
    local final
    for makesh in makeblob.sh make.sh ../make.sh ../makeblob.sh ; do
        [ -f $makesh ] && final=$makesh
    done
    echo2 running make: $final
    [ -f $final ] && (source ./$final)
}

#usage run from blobdir
#reblob 1 : updates scripts/links
reblob() {
(
  blobtype=${blobtype:-unstable}
  blobtype=${1:-$blobtype}
    echo using $blobtype

    [ -d $blobtype ] && cd $blobtype || exit
    echo REBLOBBING `pwd` ...
    set -e


    chmod_notlinks u+w .
    blobrunmake
    rm -f BLOB

    echo
    echo ================
    echo updating BLOB symlinks ...
    update_blob_links

    echo
    echo ================
    echo updating scripts and text files to refer to latest BLOB paths ...
    update_blob_texts

    [ -f README ] || echo "See http://twiki.isi.edu/NLP/BlobMap" > README
if [ $blobtype = unstable ] ; then
  echo UNSTABLE
    touch UNSTABLE.DO.NOT.USE
    chmod_notlinks ug+w .
        chmod u+w .
    find . \( -type f -o -type d \) -exec chmod -f ug+w {} \;
else
 echo FINAL
   rm -f EXPERIMENTAL.* UNSTABLE.*
    touch BLOB
    chmod_notlinks a-w .
   find . -type f -exec grep -i 'blobs/.*/unstable' {} \;
fi
set +x
    echo
    echo DONE REBLOBBING $(realpath `pwd`)
    echo
) || echo BLOBBING `pwd` failed.
}

finishblob() {
    blobtype=latest
        [ -d latest ] && cd latest
    chmod_notlinks u+w .
    echo ================
    echo updating BLOB symlinks ...
    update_blob_links

    echo
    echo ================
    echo updating .pl and .sh scripts to refer to latest BLOB paths ...
    update_blob_texts

   rm -f EXPERIMENTAL.* UNSTABLE.*
    touch BLOB
    chmod_notlinks a-w .
   find . -type f -exec grep -i 'blobs/.*/unstable' {} \;
   touch ../unstable
}

#usage: run from blob dir (containg vN)
newblob() {
    local last=`getlast -d v*`
    local new=`echo -E $last | perl -pe 's/(\d+)$/$1+1/e'`
    local blobdir=`pwd`
    if [ "$last" ] && [ "$new" ] && [ ! -d "$new" ] && [ "$new" != "$last" ] ; then
     rm -f unstable
     rm -f latest
     ln -s $last latest
     cp -pr  $last $new
     ln -s $new unstable
     blobtype=latest reblob latest
     blobtype=unstable reblob unstable
    else
     error bad succesor=$new to latest=$last
    fi
}

newlatest() {
    blobtype=latest reblob latest
    blobtype=unstable reblob unstable
}

bupdate() {
    ( set -e;
    forall blob_update "$@"
    )
}
blob_update() {
( set -e;
    local which
    for which in $*; do
     echo making new $which
     cd $BLOBS
     cd $which
     touch unstable
     newlatest
     ls -l $BLOBS/$which
    done
)
}

unprotect()
{
    chmod -R u+w latest
}
protect()
{
    chmod -R -w latest
}
editmake()
{
    (
        cd $BLOBS/$1
    ${VISUAL:emacsclient} unstable/makeblob.sh
    chmod u+w latest/makeblob.sh
    cp unstable/makeblob.sh latest/makeblob.sh
    )
}
initblob()
{
    (
        set -x
        set -e
        dest=.
        [ "$1" ] && dest=$BLOBS/$1
        mkdir -p $dest
        if [ "$2" ] ; then
            cp $2 $dest/make.sh
        fi
        cd $dest
        if ! [ -d v1 ] ; then
            rm -rf first
            mkdir first
            cd first
            blobrunmake
            cd ..
            mv first v1
            cp -pr v1 v2
            ln -s v1 latest
            ln -s v2 unstable
            finishblob
        fi
    )
}
