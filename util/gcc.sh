gccver=4.9.2
srcdir=/local/graehl/src
gccwithv=gcc-$gccver
gccsrc=$srcdir/$gccwithv
gccbuild=$srcdir/gcc-build
gccprefix=/local/gcc
gccget() {
    mkdir -p $srcdir
    cd $srcdir
    [[ -f $gccwithv.tar.bz2 ]] || wget http://ftp.gnu.org/gnu/gcc/gcc-$gccver/$gccwithv.tar.bz2
    gccclean
}
gccclean() {
    cd $srcdir
    rm -rf $gccwithv
    bzcat $gccwithv.tar.bz2 | tar xf -
    gccprereq
}
gccprereq() {
    cd $gccsrc
    contrib/download_prerequisites
}
gccbuild() {
    (
    cd $gccsrc
    set -e
    mkdir $gccbuild
        cd $gccbuild
        ../gcc-$gccver/configure \
            --prefix=$gccprefix \
            --libdir=$gccprefix/lib \
            --enable-static \
            --enable-shared \
            --enable-threads=posix \
            --enable-__cxa_atexit \
            --enable-clocale=gnu \
            --disable-multilib \
            --with-system-zlib \
            --disable-checking \
            --enable-languages=c,c++
        make "$@"
    )
}
gccinstall() {
    cd $gccbuild
    sudo make install
}
gccall() {
    (set -e
    gccget
    gccbuild "$@"
    gccinstall
    )
}
uselocalgcc() {
    export PATH=/local/gcc/bin:$PATH
    export LD_LIBRARY_PATH=/local/gcc/lib64:$LD_LIBRARY_PATH
}
