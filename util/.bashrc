HOME=$(echo ~)
#set completion-prefix-display-length 2
#set show-all-if-ambiguous on
#set show-all-if-unmodified on
#set completion-map-case on

export PYTHON_PATH=$HOME/lib/python:$PYTHON_PATH
shdir=$HOME/t/graehl/util
. $shdir/bashlib.sh
if false && [ "$TERM" = dumb ] ; then
    PS1='$ '
    exit
fi
export SHELL=/bin/bash
export LOCAL_WORKDIR=/lfs/bauhaus/graehl/workflow
export WORKDIR=/home/nlg-03/graehl/workflow
HPCHOME=/home/rcf-12/graehl
ISIHOME=/nfs/topaz/graehl
ONHPC=""
[ ! -d $ISIHOME ] && [ -d $HPCHOME ] && ONHPC=1
HPCUSER=$USER
ISIUSER=$USER

JVMFLAGS="-Xss10m -Xms192m -Xmn128m  -Xmx3560m -server -Xbatch -XX:MaxPermSize=256m -XX:+UseConcMarkSweepGC -XX:+AggressiveOpts "
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
 *i*)	 interactive=1 ;;
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

# function java {
#     javacp "$@"
# }
# export HOME=/cygdrive/z
# export TEMP=c:/cygwin/tmp
# export CYGWIN=smbntsec
# set -x
#export EMACSPACKAGEPATH=/usr/share/xemacs
#alias xemacs="EMACSPACKAGEPATH=/usr/share/xemacs good.xemacs"
#export EMACSPACKAGEPATH="c:/Program Files/XEmacs-21.5.21-jon/xemacs-packages"
 if [ $USERDOMAIN = ISI1 -o $USERDOMAIN = MAYBE ] ; then
  HOST=maybe
#  ONTRUE=
 fi
else
export REAL_JAVA=`which java`
fi
HOST=${HOST%.isi.edu}
HOST=${HOST%.usc.edu}
if [ $HOST = hpc-login1 ] ; then
 HOST=hpc
fi
export HOST

# if [ $HOST = TRUE ] && xhost +nlg0.isi.edu > /dev/null 2>&1
case $HOST in
 hpc*)   ONHPC=1 ;;
 cage*)  ONCAGE=1 ;;
 erdos*) ONERDOS=1 ;;
 zergling*) ONZERG=1 ;;
esac

if [ "$ONERDOS" ] || [ "$ONCAGE" ] ; then
#export JAVA_HOME=/usr/java/jre1.5.0_06
    export JAVA_HOME=/usr/lib/jvm/jre-1.6.0
fi

#if [ "$ONCAGE" ] ; then
#export JAVA_HOME=/usr/java/jdk1.5.0_04/
#fi
export SHOME=/home/nlg-02/graehl
export PHOME=/home/nlg-02/pust
#[ "$ONCAGE" ] && export JAVA_HOME=/nfs/isd/sdeneefe/software/jdk1.5.0/

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

#if [ "$HOST" = hpc ] ; then
# bash3=$BLOBS/bash3/bin/bash
# if [ "${BASH_VERSION#2.}" != "$BASH_VERSION" ] ; then
#  [ -x $bash3 ] && exec $bash3 --login
# fi
#fi



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
    *x86_64*)	ON64=1 ;;
esac
export ON64

if [ ! "$ONCYGWIN" ]; then
   if [ ! "$ONHPC" ]; then
    ulimit -f unlimited
    ulimit -t unlimited
    ulimit -d unlimited
    #ulimit -s 65536
   fi
fi

isd=$HOME/isd
dev=$HOME/dev
SSHDEV=dev
DEV=$dev
#export BOOST_ROOT=$isd/boost

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
#         PS1='\t(\!)$HOST:\w\$ '
PS1="\w$gitps\$ "
          ;;
  esac

PS2='> '
#fi
PAGER=less
EDITOR=emacsclient
VISUAL=$EDITOR
if false ; then
    case $TERM in
         rxvt*|*term)
            set -o functrace
#            trap 'echo -ne "\e]0;$BASH_COMMAND\007"' DEBUG
#            export PS1="\e]0;$TERM\007$PS1"
            trap 'echo -ne "\[\033]0;$HOST:\w $BASH_COMMAND\007"' DEBUG
            export PS1="\[\033]0;$HOST:\w $TERM\007$PS1"
         ;;
    esac
fi
export PS1 PS2 PAGER  EDITOR VISUAL
export CVS_RSH=ssh
export RSYNC_SSH=ssh
export FTP_PASSIVE=1

#export TZ=PDT8PST

shpchost='hpc-login1.usc.edu';
#      case "$HOST" in
#         hpc*) shpchost='nlg0.isi.edu' ;;
#        esac
if [ "$ONHPC" ] ; then
 shpchost='cage.isi.edu'
fi
export CVSROOT=":ext:graehl@cage.isi.edu:$ISIHOME/isd/cvs"

export BLOBS=~/blobs
unset LC_ALL || true
unset LANG || true
#unset JAVA_HOME || true
unset SUPPORTED || true
unset LANGVAR || true

#export JAVA_HOME=""


SBMT_SVNREPO='https://nlg0.isi.edu/svn/sbmt'
SBMT_BASE=$HOME/sbmt
SBMT_TRUNK=$SBMT_BASE/trunk


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
 PATH=/usr/bin:$cygp/usr/local/bin:$DEFAULT_PATH
 PATH=$isd/bin:~/bin:$PATH:/local/bin:/usr/X11R6/bin:$SBMT_TRUNK
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
. $shdir/add_paths.sh

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
if  [ "$ONHPC" ] ; then
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
#        exit 42
        ;;
esac
PERL=$HPCPERL/bin/perl

PATH=$HPCGCC/bin:$PATH
PATH=/home/nlg-02/graehl/mini_dev/$SBMTARCH/bin:/home/nlg-02/graehl/mini_13.0/$SBMTARCH/bin:$PATH
PATH=$HPCPERL/bin:$PATH
add_ldpath $GHOME/lib$S64
add_ldpath $HPCGCC/lib$S64 || true
fi
CABAL=~/.cabal
PATH=$CABAL/bin:$PATH
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
            export HOSTBASE ARCH64BASE ARCHBASE  HOST32BASE SBMT_TRUNK
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
 export CXXFLAGS="$CFLAGS"
}

qsippn=2
#512M stack!
ulimitsafe 524288 s

set_build


if [ "$interactive" ] ; then
export teeout=1
# from http://www.onerussian.com/Linux/bash_history.phtml
HISTORYOLD=~/.archive.bash.history
function archive_history
{
#    HISTORYOLD=${HISTFILE}.archive
    CURTIME=`date`
    CURTTY=`tty`
    if  [ x$HISTDUMPPED = x ]; then
      echo "#-${HOSTNAME}-- ${CURBASHDATE} - ${CURTIME} ($CURTTY) ----" >>   $HISTORYOLD
      history $(($HISTCMD-${CURBASHSTART-0})) | sed -e 's/^[ ]*[0-9][0-9]* [ ]*//g'  >> $HISTORYOLD
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
#export CURBASHSTART=`grep -v "^[ \t]*$" $HISTFILE | wc -l | awk '{print $1}'`  CURBASHDATE=`date`
shopt -s cmdhist histappend

fi
#interactive
if [ "$ONTRUE" ] ; then
 false && echo \'xhosts\' to give X access
else
 false && truex
fi

tocage='cage.isi.edu:/users/graehl'



export PYTHONSTARTUP=$shdir/inpy

export SCALA_HOME=${SCALA_HOME:-~/isd/linux}

function pontus {
   ssh -L 3391:darkstar:3389 peggy@pontus.languageweaver.com -p 4640
}

export GOROOT=$HOME/go
export GOARCH=amd64
export GOOS=linux

export MONO_USE_LLVM=1
[ "$ONHPC" ] && export PATH=/home/nlg-01/chiangd/pkg/emacs-23.2/bin:$PATH

if [ "$HOST" = cage ] ; then
    export PERL5LIB=/nfs/topaz/graehl/isd/cage/lib/perl5/5.10.0:/nfs/topaz/graehl/isd/cage/lib/perl5/site_perl/5.10.0:/nfs/topaz/graehl/isd/cage/lib64/perl5/5.10.0/x86_64-linux-thread-multi:/nfs/topaz/graehl/isd/cage/lib64/perl5/site_perl/5.10.0/x86_64-linux-thread-multi:${FIRST_PERL5LIB:=$PERL5LIB}
fi
true
export PATH=$PATH:~/isd/maven/bin

GRAEHL=~graehl
. ~/local.sh
UTIL=$GRAEHL/t/graehl/util
. $UTIL/aliases.sh
#. $UTIL/z.sh
export LD_LIBRARY_PATH=$FIRST_PREFIX/lib:$FIRST_PREFIX/lib64

if ! bash --version | grep -q 2.05 ; then
. ~/bin/autojump.bash
fi

export CLOJURE_EXT=~/.clojure
