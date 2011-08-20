d=`dirname $0`
#-perm -u+x
$d/util/subst.pl "$@" -v -t $d/util/use-pypy.subst  -i -e `find . -size -1000k ! -name '*~' ! -name '*svn*'`
