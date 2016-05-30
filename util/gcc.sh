# cd /local/graehl/src/gcc-build;sudo mv /local/gcc{,-4.9}; sudo make install
#cd /local;sudo mv gcc{,-4.9}; sudo tar xzf /home/graehl/gcc.tar.gz
#ftp://gcc.gnu.org/pub/gcc/snapshots/5.2.0-RC-20150707
gccver=6.1.0
srcdir=/local/graehl/src
mkdir -p $srcdir
gccwithv=gcc-$gccver
gccsrc=$srcdir/$gccwithv
gccbuild=$srcdir/gcc-build
gccprefix=/local/gcc
gccyum() {
    sudo yum install gcc-c++ zlib-devel
}
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
    rm -rf $gccbuild
}
gccprereq() {
    cd $gccsrc
    contrib/download_prerequisites
}
gccbuild() {
    (
        cd $gccsrc
        set -e
        if [[ -d $gccbuild ]] ; then
            cd $gccbuild
        else
            mkdir $gccbuild
            cd $gccbuild
            ../gcc-$gccver/configure \
                --prefix=$gccprefix \
                --libdir=$gccprefix/lib \
                --enable-static \
                --enable-shared \
                --enable-threads=posix \
                --enable-__cxa_atexit \
                --disable-multilib \
                --with-system-zlib \
                --disable-checking \
                --with-default-libstdcxx-abi=gcc4-compatible \
                --enable-languages=c,c++,fortran
            #            --enable-clocale=gnu
        fi
        make "$@"
    )
}
gccinstall() {
    cd $gccprefix/..
    [[ -d gcc ]] && sudo mv gcc gcc-pre-$gccver
    cd $gccbuild
    sudo make install
}
gccgetbuild() {
    (set -e
     gccget
     gccbuild -j4 "$@"
     cd $gccbuild
    )
}
gccall() {
    (set -e
     gccget
     gccbuild -j4 "$@"
     gccinstall
    )
}
uselocalgcc() {
    export PATH=/local/gcc/bin:$PATH
    export LD_LIBRARY_PATH=/local/gcc/lib64:$LD_LIBRARY_PATH
}
vgall() {
    (set -e
     cd /local/graehl/src
     tarxzf ~/src/valgrind-3.10.1.tar.bz2 2>&1 >/dev/null
     cd valgrind-3.10.1/
     ./configure --prefix=/usr/local && make -j8 && sudo make install
    )
}
gdbinstall() {
    mkdir -pv /usr/share/gdb/auto-load/usr/lib &&
        mv -v /usr/lib/*gdb.py /usr/share/gdb/auto-load/usr/lib &&
        chown -v -R root:root \
              /usr/lib/gcc/*linux-gnu/$gccver/include{,-fixed}
}
