d=`dirname $0`
. $d/aliases.sh
pycheckers $($d/findscripts.sh py "$@")
