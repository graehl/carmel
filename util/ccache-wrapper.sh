name=`basename $0`
exec ccache ${name#ccache-} "$@"
