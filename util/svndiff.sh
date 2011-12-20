#!/bin/bash
# Configure your favorite diff program here.
DIFF="diff"

# Subversion provides the paths we need as the sixth and seventh
# parameters.
LEFT=${6}
RIGHT=${7}

nleft=${3}
nright=${5}

#. ~/u/bashlib.sh
#showvars_required nleft LEFT nright RIGHT
# Call the diff command (change the following line to make sense for
# your merge program).
$DIFF  -w -u -b --label="$nleft" $LEFT --label "$nright" $RIGHT

# Return an errorcode of 0 if no differences were detected, 1 if some were.
# Any other errorcode will be treated as fatal.
