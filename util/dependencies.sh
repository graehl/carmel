#!/bin/bash
 
# This is the maximum depth to which dependencies are resolved
MAXDEPTH=14
 
# analyze a given file on its
# dependecies using ldd and write
# the results to a given temporary file
#
# Usage: analyze [OUTPUTFILE] [INPUTFILE]
function analyze
{
    local OUT=$1
    local IN=$2
    local NAME=$(basename $IN)
 
    for i in $LIST
    do
        if [ "$i" == "$NAME" ];
        then
            # This file was already parsed
            return
        fi
    done
    # Put the file in the list of all files
    LIST="$LIST $NAME"
 
    DEPTH=$[$DEPTH + 1]
    if [ $DEPTH -ge $MAXDEPTH ];
        then
        echo "MAXDEPTH of $MAXDEPTH reached at file $IN."
        echo "Continuing with next file..."
        return
    fi
 
    echo "Parsing file:              $IN"
 
    $READELF $IN &> $READELFTMPFILE
    ELFRET=$?
 
    if [ $ELFRET != 0 ];
        then
        echo "ERROR: ELF reader returned error code $RET"
        echo "ERROR:"
        cat $TMPFILE
        echo "Aborting..."
        rm $TMPFILE
        rm $READELFTMPFILE
        rm $LDDTMPFILE
        exit 1
    fi
 
    DEPENDENCIES=$(cat $READELFTMPFILE | grep NEEDED | awk '{if (substr($NF,1,1) == "[") print substr($NF, 2, length($NF) - 2); else print $NF}')
 
    for DEP in $DEPENDENCIES;
    do
        if [ -n "$DEP" ];
        then
 
            ldd $IN &> $LDDTMPFILE
            LDDRET=$?
 
            if [ $LDDRET != 0 ];
                then
                echo "ERROR: ldd returned error code $RET"
                echo "ERROR:"
                cat $TMPFILE
                echo "Aborting..."
                rm $TMPFILE
                rm $READELFTMPFILE
                rm $LDDTMPFILE
                exit 1
            fi
 
            DEPPATH=$(grep $DEP $LDDTMPFILE | awk '{print $3}')
            if [ -e "$DEPPATH" ];
            then
                echo -e "  \"$NAME\" -> \"$DEP\";" >> $OUT
                analyze $OUT $DEPPATH
            else
                echo "WARNING: Excluding unreachable library: $DEP"
            fi
        fi
    done
 
    DEPTH=$[$DEPTH - 1]
}
 
########################################
# main                                 #
########################################
 
if [ $# != 2 ];
    then
    echo "Usage:"
    echo "  $0 [filename] [outputimage]"
    echo ""
    echo "This tools analyses a shared library or an executable"
    echo "and generates a dependency graph as an image."
    echo ""
    echo "GraphViz must be installed for this tool to work."
    echo ""
    exit 1
fi
 
DEPTH=0
INPUT=$1
OUTPUT=$2
TMPFILE=$(mktemp -t)
LDDTMPFILE=$(mktemp -t)
READELFTMPFILE=$(mktemp -t)
LIST=""
 
if [ ! -e $INPUT ];
    then
    echo "ERROR: File not found: $INPUT"
    echo "Aborting..."
    exit 2
fi
 
# Use either readelf or dump
# Linux has readelf, Solaris has dump
READELF=$(type readelf 2> /dev/null)
if [ $? != 0 ]; then
  READELF=$(type dump 2> /dev/null)
  if [ $? != 0 ]; then
    echo Unable to find ELF reader
    exit 1
  fi
  READELF="dump -Lv"
else
  READELF="readelf -d"
fi
 
 
 
echo "Analyzing dependencies of: $INPUT"
echo "Creating output as:        $OUTPUT"
echo ""
 
echo "digraph DependencyTree {" > $TMPFILE
echo "  \"$(basename $INPUT)\" [shape=box];" >> $TMPFILE
analyze $TMPFILE "$INPUT"
echo "}" >> $TMPFILE
 
#cat $TMPFILE # output generated dotfile for debugging purposses
dot -Tpng $TMPFILE -o$OUTPUT
 
rm $LDDTMPFILE
rm $TMPFILE
 
exit 0
