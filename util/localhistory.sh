# output '0' if no matches (not using -q because PIPEFAIL might not work)
useHistory() {
    \egrep -v '^top$|^pwd$|^ls$|^ll$|^l$|^lt$|^cd |^h |^bg$|^fg$' | wc -l | tr -d ' \n'
}
owner() {
    \ls -ld "${1:-$PWD}" | awk '{print $3}'
}
lasthistoryline() {
    history 1 | HISTTIMEFORMAT= sed 's:^ *[0-9]* *::'
}
localHistory()
{
    if [[ `owner` = ${USER:=$(whoami)} ]] ; then
        local useful=`lasthistoryline | useHistory | tr -d '\n'`
        if [[ $useful != 0 ]] ; then
            # date hostname cmd >> $PWD/.history
            (\date +"%FT%T.${HOST:=$(uname -n)} " | \tr -d '\n' ; lasthistoryline) >>.history 2>/dev/null
        fi
    fi
}
addPromptCommand() {
    if [[ $PROMPT_COMMAND != *$1* ]] ; then
        if [[ $PROMPT_COMMAND ]] ; then
            PROMPT_COMMAND+="$1; "
        else
            PROMPT_COMMAND=$1
        fi
        export PROMPT_COMMAND
    fi
}
h() {
    # look for something in your cwd's .history file
    if [[ -r .history ]]; then
        if ! [[ $1 ]]; then
            \cat .history
        else
            # permit looking for multiple things: h 'foo bar' baz
            local f
            for f in "$@"; do
                \grep -a "$f" .history
            done
        fi
    else
        echo "Warning: .history not accessible" 1>&2
    fi
}

### to enable,
# addPromptCommand localHistory
