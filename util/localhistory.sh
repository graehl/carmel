# output '0' if no matches (not using -q because PIPEFAIL might not work)
useHistory() {
    \egrep -v '^top$|^pwd$|^ls$|^ll$|^l$|^lt$|^cd |^h |^bg$|^fg$'
}
owner() {
    \ls -ld "${1:-$PWD}" | \awk '{print $3}'
}
lastHistoryLine() {
    history 1 | HISTTIMEFORMAT= \sed 's:^ *[0-9]* *::'
}
localHistory()
{
    if [[ `owner` = ${USER:=$(whoami)} ]] ; then
        local line=$(lastHistoryLine)
        if [[ $(echo "$line" | useHistory) ]]; then
            # date hostname cmd >> $PWD/.history
            echo $(date +'%FT%T').${HOST:=$(uname -n)} "$line" >> .history 2>> /dev/null
        fi
    fi
}
addPromptCommand() {
    # convenience command to enable adding of the prompt
    if [[ $PROMPT_COMMAND != *$1* ]]; then
        if [[ $PROMPT_COMMAND ]]; then
            # exists with content
            if [[ $PROMPT_COMMAND =~ \;[\ \ ]*$ ]]; then
                # already ends in semicolon and space or tab
                PROMPT_COMMAND+="$1"
            else
                # does not end in semicolon
                PROMPT_COMMAND+=" ; $1"
            fi
        else
            PROMPT_COMMAND="$1"
        fi
        export PROMPT_COMMAND
    fi
}
function h() {
    # look for something in your cwd's .history file
    if [[ -r .history ]]; then
        if ! [[ $1 ]]; then
            \cat .history
        else
            # permit looking for multiple things: h 'foo bar' baz
            local f
            for f in "$@"; do
                grep -a "$f" .history # allow aliased grep --color etc
            done
        fi
    else
        echo "Warning: .history not accessible" 1>&2
    fi
}

### to enable,
# addPromptCommand localHistory
