HOME=$(echo ~)
#set completion-prefix-display-length 2
#set show-all-if-ambiguous on
#set show-all-if-unmodified on
#set completion-map-case on

LOCAL_PYTHON_PATH=$HOME/lib/python
mkdir -p $LOCAL_PYTHON_PATH
export PYTHON_PATH=$LOCAL_PYTHON_PATH:$PYTHON_PATH
GRAEHL=~graehl
UTIL=$GRAEHL/u

if false && [ "$TERM" = dumb ] ; then
    PS1='$ '
    exit
fi
export SHELL=/bin/bash
export LOCAL_WORKDIR=/lfs/bauhaus/graehl/workflow
export WORKDIR=/home/nlg-03/graehl/workflow

JVMFLAGS="-Xss10m -Xms192m -Xmn128m -Xmx3560m -server -Xbatch -XX:MaxPermSize=256m -XX:+UseConcMarkSweepGC -XX:+AggressiveOpts "
SCALAFLAGS_BASE="-target:jvm-1.5 -Ywarn-dead-code -deprecation"
SCALAFLAGS_DBG="$SCALAFLAGS_BASE -g:notailcalls"
SCALAFLAGS_OPT="$SCALAFLAGS_BASE -Xdisable-assertions -g:line -Yinline -optimise -Yclosure-elim -Ydead-code -Ydetach -Yno-generic-signatures"
DEV=$HOME/dev
export PATH=/usr/bin:$PATH
#if [ $HOST = TRUE ] ; then DEV=/cache; fi
HOSTNAME=`hostname | /usr/bin/tr -d '\r\n'`
#set -x

umask 22

case "$-" in
    *i*) interactive=1 ;;
esac

if [ $TERM = "cmd" ]; then
    stty -onlcr ocrnl icrnl
fi
#OS=cygwin
OS=`uname`
export USER=`whoami`
HOST=`echo "$HOSTNAME"`
if [ ${OS#CYGWIN} != "$OS" ] ; then
#export CYGWIN=smbntsec
    ONCYGWIN=1
    export USER=graehl
    [ "$JAVA_HOME" ] && export JAVA_HOME=$(cygpath $JAVA_HOME)
    export REAL_JAVA=`/usr/bin/which java.exe`
    export REAL_JAVA=`which java`
fi
HOST=${HOST%.isi.edu}
HOST=${HOST%.usc.edu}
HOST=${HOST%.languageweaver.com}
export HOST

# if [ $HOST = TRUE ] && xhost +nlg0.isi.edu > /dev/null 2>&1
case $HOST in
    hpc*) ONHPC=1 ;;
    cage*) ONCAGE=1 ;;
    erdos*) ONERDOS=1 ;;
    zergling*) ONZERG=1 ;;
esac


if [ ! "$ONCYGWIN" ] ; then
    [ "$JAVA_HOME" ] && export PATH="$JAVA_HOME/bin":$PATH
fi

function hpchome {
    if [ "$ONHPC" ] ; then
        perl -i -pe 's|\Q'"$ISIHOME"'\E|'"$HPCHOME"'|g' $*
    else
        perl -i -pe 's|\Q'"$HPCHOME"'\E|'"$ISIHOME"'|g' $*
    fi
}

if [ -z "$ARCH" ] ; then
    ARCH=cygwin
fi
if [ $OS = Linux ] ; then
    ARCH=linux
fi
if [ $OS = SunOS ] ; then
    ARCH=solaris
fi
if [ $OS = Darwin ] ; then
    ARCH=macosx
fi
export ARCH
UNAMEA=`uname -a`
case "$UNAMEA" in
    *x86_64*) ON64=1 ;;
esac
export ON64

        #ulimit -f unlimited
        #ulimit -t unlimited
        #ulimit -d unlimited
    #ulimit -s 65536

isd=$HOME/isd

export BOOST=$isd/boost
export BOOST_BUILD=$BOOST/tools/build/v2

function set_title
{
    echo -ne "\[\033]0;$*\007"
}

gitps='(__git_ps1 "(%s)")'
gitps=
case $TERM in
    xterm*)
#\t \! \w \u \h (\!)
#\`historycmd 1\`
        PS1="\[\033]0;$HOST:\w \007\]\w$gitps\$ "
        ;;
    rxvt*)
        PS1="\[\033]0;$HOST:\w \007\]\w$gitps\$ "
        ;;
    *)
# PS1='\t(\!)$HOST:\w\$ '
        PS1="\w$gitps\$ "
        ;;
esac

PS2='> '
#fi
if [[ $INSIDE_EMACS ]] ; then
PAGER=
else
PAGER=less
fi
EDITOR=emacs
VISUAL=$EDITOR
if false ; then
    case $TERM in
        rxvt*|*term)
            set -o functrace
# trap 'echo -ne "\e]0;$BASH_COMMAND\007"' DEBUG
# export PS1="\e]0;$TERM\007$PS1"
            trap 'echo -ne "\[\033]0;$HOST:\w $BASH_COMMAND\007"' DEBUG
            export PS1="\[\033]0;$HOST:\w $TERM\007$PS1"
            ;;
    esac
fi
export PS1 PS2 PAGER EDITOR VISUAL
export CVS_RSH=ssh
export RSYNC_SSH=ssh
export FTP_PASSIVE=1

#export TZ=PDT8PST


export BLOBS=~/blobs
unset LC_ALL || true
unset LC_CTYPE || true
unset LC_NUMERIC || true
unset LANG || true
#unset JAVA_HOME || true
unset SUPPORTED || true
unset LANGVAR || true

export LANG=en_US.UTF-8
enutf8=en_US.UTF-8
export LC_ALL=$lc
#export JAVA_HOME=""


SBMT_SVNREPO='https://nlg0.isi.edu/svn/sbmt'
SBMT_BASE=$HOME/sbmt
SBMT_TRUNK=$HOME/t


function save_default_paths {
    if [ ! "$DEFAULTS_SAVED" ] ; then
        DEFAULTS_SAVED=1
        DEFAULT_PATH=$PATH
        DEFAULT_MAN_PATH=$MAN_PATH
        DEFAULT_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
        DEFAULT_LIBRARY_PATH=$LIBRARY_PATH
        DEFAULT_C_INCLUDE_PATH=$C_INCLUDE_PATH
    fi
}
save_default_paths

function default_paths {
    MAN_PATH=/usr/local/man:$MAN_PATH
    local cygp
    [ "$ONCYGWIN" ] && cygp=/usr/lib:
    PATH=$cygp/usr/local/bin:/usr/bin:$DEFAULT_PATH
    mkdir -p ~/script
    PATH=$isd/bin:~/bin:~/script:$PATH
    if [[ $OS != Darwin ]] ; then
        PATH=$PATH:/local/bin
    fi
    PATH+=/usr/X11R6/bin
#:$SBMT_TRUNK
    LDFLAGS="-L/usr/local/lib "
# --enable-lto
# LDFLAGS=" -L/usr/local/lib -L/usr/lib"
    CPPFLAGS=""
    LD_LIBRARY_PATH="$DEFAULT_LD_LIBRARY_PATH"
    LIBRARY_PATH="$DEFAULT_LIBRARY_PATH"
    C_INCLUDE_PATH="$DEFAULT_C_INCLUDE_PATH:/usr/include"
# CPPINC=""
#/opt/sfw/bin:/opt/SUNWspro/bin:/usr/openwin/bin:/usr/dt/bin:/local/xwin/bin:/opt/sfw/gnome/bin:/opt/sfw/kde/bin:/usr/bin:/usr/ccs/bin:/usr/ucb
}
default_paths


nohostbase=1
. $UTIL/add_paths.sh

S64=""
[ "$ON64" ] && S64="64"

FIRST_PREFIX=""
function add_path
{
    local prefix=$1
    export FIRST_PREFIX=`echo $prefix`
    prepend_path $prefix
}

HPCBLOBS=/home/nlg-01/blobs
HPCGCC=/usr/usc/gnu/gcc/gcc4-default


HPCPERL32=$HPCBLOBS/perl/v5.8.8
HPCPERL64=/home/nlg-03/voeckler/perl/x86_64

SBMTARCH=i386
[ "$ON64" ] && SBMTARCH=x86_64

function finalize_paths {
#[ "$ON64" ] && PATH=/usr/bin:$PATH
#[ `/usr/bin/whoami` = root ] && PATH=/sbin:/usr/sbin:/usr/local/sbin:$PATH

#[ "$ONCYGWIN" ] && PATH=/usr/local/ssh-bin:$PATH
    if [ "$ONHPC" ] ; then
        case "`uname -m`" in
            i[3456]86)
                HPCPERL=$HPCPERL32
                PERLV=5.8.8
                export PERL5LIB=~voeckler/lib/perl:$HPCPERL/lib/$PERLV:$HPCPERL/lib/site_perl/$PERLV:$HPCPERL/lib/site_perl
                ;;
            x86_64)
                HPCPERL=$HPCPERL64
                unset PERL5LIB
                ;;
            *)
                echo "WARNING: Unsupported architecture `uname -m`" 1>&2
# exit 42
                ;;
        esac
        PERL=$HPCPERL/bin/perl

        PATH=$HPCGCC/bin:$PATH
#/home/nlg-02/graehl/mini_dev/$SBMTARCH/bin:/home/nlg-02/graehl/mini_13.0/$SBMTARCH/bin:
        PATH=$PATH
        PATH=$HPCPERL/bin:$PATH
        add_ldpath $GHOME/lib$S64
        add_ldpath $HPCGCC/lib$S64 || true
    fi
    CABAL=~/.cabal
    if [ -d $CABAL/bin ] ; then
        PATH=$CABAL/bin:$PATH
    fi
    export PATH
}

function set_paths {
    default_paths
    for p in $*; do
        add_path $p
    done
    finalize_paths
}
ARCHBASE=$isd/$ARCH
HOSTBASE=$isd/$HOST
case $HOST in
    a??)
        HOSTBASE=$isd/a
        ;;
    x??)
        HOSTBASE=$isd/x
        ;;
esac

if [ "$ON64" ] ; then
    [ "$ONHPC" ] && HOSTBASE=$isd/hpc-opteron
    ARCH64BASE=${ARCHBASE}64
    HOST32BASE=${HOSTBASE}-i686
    PREFIXES="$ARCHBASE $ARCH64BASE $HOSTBASE"
else
    [ "$ONHPC" ] && HOSTBASE=$isd/hpc
    ARCH64BASE=""
    HOST32BASE=""
    PREFIXES="$ARCHBASE $ARCH64BASE $HOST32BASE $HOSTBASE"
fi


function set_extra_paths {
    set_paths $PREFIXES $*
}

set_extra_paths

function exportflags {
    if [ "$static" ] ; then
        staticargs="-static"
    fi
    export CFLAGS="$CFLAGS $*"
    export CXXFLAGS="$CFLAGS $*"
    export LDFLAGS="$LDFLAGS $staticargs "
    export BOOST_SUFFIX=
    [ "$HOST" = "strontium" ] && export BOOST_SUFFIX=gcc41-mt
    [ "$HOST" = "grieg" ] && export BOOST_SUFFIX=gcc41-mt
    [ "$HOST" = "cage" ] && export BOOST_SUFFIX=
    [ "$HOST" = "maybe" ] && export BOOST_SUFFIX=gcc34-mt
    export BOOST_SRCDIR=~/isd/boost
    export HOSTBASE ARCH64BASE ARCHBASE HOST32BASE SBMT_TRUNK
}

exportflags

#CMPH_INCLUDE=-I$SBMT_TRUNK/biglm/cmph/src


#i686
function set_build {
    SBMT_INCLUDE=$SBMT_TRUNK/sbmt_decoder/include
    RR_INCLUDE=$SBMT_TRUNK/RuleReader/include
    GRAEHL_INCLUDE=$SBMT_TRUNK
    BOOST_INCLUDE=$BOOST
    local target=$1
    shift
    local suffix=""
    if [ "$target" ] ; then
        suffix=-`filename_from $target`
        PREFIX=$isd/$HOST$suffix
        if [ "$target" = i686 ] ; then
            ARCHPREFIX=$ARCHBASE
        fi
        CONFIG="--target $target"
    else
        PREFIX=""
        CONFIG=""
        MARGS=""
    fi
    BUILDDIR=$HOST$suffix
    set_extra_paths $ARCHPREFIX $PREFIX
    SRILMDIR=$HOME/src/srilm
    CONFIG="$CONFIG --prefix=$FIRST_PREFIX"
    local bsufarg
    [ "$BOOST_SUFFIX" ] && bsufarg="--with-boost-suffix=$BOOST_SUFFIX"
    CONFIGBOOST="$CONFIG $bsufarg"
    CONFIGSRILM="$CONFIG --with-srilm=$SRILMDIR"
    if [ "$static" ] ; then
        export LDFLAGS="-static $LDFLAGS"
    fi
#-I$BOOST_INCLUDE
    export OPTFLAGS="-march=native -mtune=native -ffast-math -Wunsafe-loop-optimizations -funsafe-loop-optimizations -funsafe-math-optimizations"
#-I$SBMT_INCLUDE -I$RR_INCLUDE -I$GRAEHL_INCLUDE
    export GCPPFLAGS="-I$SBMT_INCLUDE -I$RR_INCLUDE -I$GRAEHL_INCLUDE $CPPFLAGS"
    export CPPFLAGS="$CPPFLAGS"
    if [ "$HOST" = zergling ] ; then
        export CFLAGS="$CPPFLAGS -O0 -ggdb -Wall $*"
    else
        export CFLAGS="$CPPFLAGS -O3 -ggdb -Wall -Wno-parentheses $*"
    fi
#endif
#-Wno-deprecated
#-fvisibility-inlines-hidden
#-Wno-write-strings
    export CXXFLAGS="$CFLAGS "
 #-Weffc++
 # not compatible w/ boost really
}

qsippn=2

set_build


if [ "$interactive" ] ; then
    export teeout=1
# from http://www.onerussian.com/Linux/bash_history.phtml
    HISTORYOLD=~/.archive.bash.history
    function archive_history
    {
# HISTORYOLD=${HISTFILE}.archive
        CURTIME=`date`
        CURTTY=`tty`
        if [ x$HISTDUMPPED = x ]; then
            echo "#-${HOSTNAME}-- ${CURBASHDATE} - ${CURTIME} ($CURTTY) ----" >> $HISTORYOLD
            history $(($HISTCMD-${CURBASHSTART-0})) | sed -e 's/^[ ]*[0-9][0-9]* [ ]*//g' >> $HISTORYOLD
            export HISTDUMPPED=1
        fi
        chmod go-r $HISTORYOLD
    }
    function historycmd {
        history $* | cut -b8-
    }
    function exit
    {
        archive_history
        builtin exit
    }
    alias x="exit"
#export CURBASHSTART=`grep -v "^[ \t]*$" $HISTFILE | wc -l | awk '{print $1}'` CURBASHDATE=`date`
    shopt -s cmdhist histappend

fi

export PYTHONSTARTUP=$UTIL/inpy

export SCALA_HOME=${SCALA_HOME:-~/isd/linux}

export GOROOT=$HOME/go
export GOARCH=amd64
export GOOS=linux

export MONO_USE_LLVM=1

export LD_LIBRARY_PATH=$FIRST_PREFIX/lib:$FIRST_PREFIX/lib64


set-eterm-dir() {
    echo -e "\033AnSiTu" "$LOGNAME" # $LOGNAME is more portable than using whoami.
    echo -e "\033AnSiTc" "$(pwd)"
    if [ $(uname) = "SunOS" ]; then
            # The -f option does something else on SunOS and is not needed anyway.
        hostname_options="";
    else
        hostname_options="-f";
    fi
    echo -e "\033AnSiTh" "$(hostname $hostname_options)" # Using the -f option can cause problems on some OSes.
    history -a # Write history to disk.
}

if false && [ "$TERM" = "eterm-color" ]; then
    PROMPT_COMMAND="$PROMPT_COMMAND ; set-eterm-dir"
fi

export CLOJURE_EXT=~/.clojure
export HYPERGRAPH_DBG=1

. $UTIL/aliases.sh
. ~/local.sh
ulimitsafe 262144 s

#if [[ $INSIDE_EMACS ]] ; then export PS1='|PrOmPt|\w|\w $ '; fi
if [[ $INSIDE_EMACS ]] ; then
 export PS1='\w $ '
fi
if [[ -f ~/.git-completion.bash ]] ; then
 . ~/.git-completion.bash
fi

PATH=$PATH:$HOME/.rvm/bin # Add RVM to PATH for scripting
