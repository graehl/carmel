addlicense() {
    tmpfile=$(mktemp ${tmpdir:-/tmp}/license.XXXXXX)
    for f in "$@"; do
        if grep -q "WARRANT" $f; then
            echo "$f had a WARRANT string - licensed already?"
            head -10 $f
            echo ...
            echo
        else
            set -x
            cat $license $f > $tmpfile && mv $tmpfile $f
            set +x
        fi
    done
}
LICENSE_DIR=${LICENSE_DIR:-`dirname $0`}
findc() {
    find . -name '*.hpp' -o -name '*.cpp' -o -name '*.ipp' -o -name '*.cc' -o -name '*.hh' -o -name '*.c' -o -name '*.h'
}
addlicenses() {
    local license=${1:-$LICENSE_DIR/license.txt}
    if [[ -f $license ]] ; then
        addlicense `findc`
    else
        echo "usage: cd src; addlisencec ../license.txt"
    fi
}
