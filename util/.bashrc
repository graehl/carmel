HOME=`echo ~`
#set completion-prefix-display-length 2
#set show-all-if-ambiguous on
#set show-all-if-unmodified on
#set completion-map-case on

LOCAL_PYTHON_PATH=$HOME/lib/python
mkdir -p $LOCAL_PYTHON_PATH
export PYTHON_PATH=$LOCAL_PYTHON_PATH:$PYTHON_PATH
GRAEHL=`echo ~graehl`
UTIL=$GRAEHL/u

if false && [ "$TERM" = dumb ] ; then
    PS1='$ '
    exit
fi
export SHELL=/bin/bash

JVMFLAGS="-Xss10m -Xms192m -Xmn128m -Xmx3560m -server -Xbatch -XX:MaxPermSize=256m -XX:+UseConcMarkSweepGC -XX:+AggressiveOpts "
DEV=$HOME/dev
export PATH=/usr/bin:$PATH:
HOSTNAME=`hostname | /usr/bin/tr -d '\r\n'`

umask 22

interactive=
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
export LC_ALL=
#export JAVA_HOME=


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
    PATH=${cygp:-}/usr/local/bin:/usr/bin:$DEFAULT_PATH
    PATH=~/bin:$PATH
    #~/script:$isd/bin:
    #mkdir -p ~/script
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
#export LD_LIBRARY_PATH=$FIRST_PREFIX/lib:$FIRST_PREFIX/lib64
function add_path
{
    local prefix=$1
    export FIRST_PREFIX=`echo $prefix`
    prepend_path $prefix
}

function finalize_paths {
#[ "$ON64" ] && PATH=/usr/bin:$PATH
#[ `/usr/bin/whoami` = root ] && PATH=/sbin:/usr/sbin:/usr/local/sbin:$PATH

#[ "$ONCYGWIN" ] && PATH=/usr/local/ssh-bin:$PATH
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

if [ "$ON64" ] ; then
    ARCH64BASE=${ARCHBASE}64
    HOST32BASE=${HOSTBASE}-i686
    PREFIXES="$ARCHBASE $ARCH64BASE $HOSTBASE"
else
    ARCH64BASE=""
    HOST32BASE=""
    PREFIXES="$ARCHBASE $ARCH64BASE $HOST32BASE $HOSTBASE"
fi
PREFIXES=

if [[ -d /usr/x11/bin ]] ; then
  PREFIXES+=" /usr/x11/bin"
fi

function set_extra_paths {
    set_paths $PREFIXES $*
}

set_extra_paths

function exportflags {
    if [[ ${static:-} ]] ; then
        staticargs="-static"
        LDFLAGS="$LDFLAGS $staticargs "
    fi
    export CFLAGS="$CFLAGS $*"
    export CXXFLAGS="$CFLAGS $*"
    export LDFLAGS
    export BOOST_SUFFIX=-mt
    export BOOST_SRCDIR=~/src/boost
    export HOSTBASE ARCH64BASE ARCHBASE HOST32BASE
}

exportflags

#i686
OPTFLAGS="-march=native -mtune=native -ffast-math -Wunsafe-loop-optimizations -funsafe-loop-optimizations -funsafe-math-optimizations"

function archive_history
{
    # HISTORYOLD=${HISTFILE}.archive
    CURTIME=`date`
    CURTTY=`tty`
    if [[ ! ${HISTDUMPED:-} ]]; then
        echo "#-${HOSTNAME}-- ${CURBASHDATE} - ${CURTIME} ($CURTTY) ----" >> $HISTORYOLD
        local n=$(($HISTCMD-${CURBASHSTART-0}))
        if [[ $n -gt 0 ]] ; then
            history $n | sed -e 's/^[ ]*[0-9][0-9]* [ ]*//g' >> $HISTORYOLD
        fi
        export HISTDUMPED=1
    fi
    chmod go-r $HISTORYOLD
}
function historycmd {
    history $* | cut -b8-
}
CURBASHDATE=`date`

if [[ ${interactive:-} ]] ; then
    export CURBASHSTART=`grep -v "^[ \t]*$" $HISTFILE | wc -l | awk '{print $1}'`

# for bashlib.sh exec_safe
    teeout=1

# from http://www.onerussian.com/Linux/bash_history.phtml
    HISTORYOLD=~/.archive.bash.history
#export CURBASHSTART=`grep -v "^[ \t]*$" $HISTFILE | wc -l | awk '{print $1}'` CURBASHDATE=`date`
    shopt -s cmdhist histappend
    function exit
    {
        archive_history
        builtin exit
    }
fi

export PYTHONSTARTUP=$UTIL/inpy

set_eterm_dir() {
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

. $UTIL/aliases.sh
. $GRAEHL/local.sh
ulimitsafe 262144 s

#if [[ $INSIDE_EMACS ]] ; then export PS1='|PrOmPt|\w|\w $ '; fi
if [[ $INSIDE_EMACS ]] ; then
 export PS1='\w $ '
fi

if false && [[ -f ~/.git-completion.bash ]] ; then
 . ~/.git-completion.bash
fi

if [[ -f $UTIL/localhistory.sh ]] ; then
    . $UTIL/localhistory.sh
    addPromptCommand localHistory
fi
