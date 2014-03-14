# output '0' if no matches (not using -q because PIPEFAIL might not work)
useHistory() {
    egrep -v '^top$|^pwd$|^ls$|^ll$|^l$|^lt$|^dir$|^cd |^h$|^gh$|^h |^bg$|^fg$|^qsm$|^quser$|^cStat|^note |^mutt|^std ' | wc -l | tr -d ' \n'
}
owner() {
    ls -ld "${1:-$PWD}" | awk '{print $3}'
}
lasthistoryline() {
    history 1 | sed 's:^ *[0-9]* *::g'
}
localHistory()
{
    if [[ `owner` = "$USER" ]] ; then
        local useful=`lasthistoryline | useHistory | tr -d '\n'`
        if [[ $useful != 0 ]] ; then
            # date hostname cmd >> $PWD/.history
            ((date +%F.%H-%M-%S.; echo "$HOST ") | tr -d '\n' ; lasthistoryline) >>.history 2>/dev/null
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
# convenience cmd for searching: h blah => anything matching blah in current local history
alias h='cat .history | grep --binary-file=text'

### to enable,
# addPromptCommand localHistory
