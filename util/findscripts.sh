interp=$1
shift
dir=${2:-.}
for f in $(find $dir -type f -size -1000k ! -name '*~' ! -name '*svn*') ; do
    if head -1 $f | grep '^#!/' | fgrep -q "$interp" ; then
        echo $f
    fi
done
