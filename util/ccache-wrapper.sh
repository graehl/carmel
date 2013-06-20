name=`basename $0`
if [[ $gccfilter ]] && [[ -x `which gccfilter 2>/dev/null` ]] ; then
  exec gccfilter ${gccfilterargs:- -r -w -n -i} ccache ${name#ccache-} "$@"
else
  exec ccache ${name#ccache-} "$@"
fi
