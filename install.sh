#!/bin/bash
dest=${1:?'first-arg: destination prefix - installs headers in prefix/include'}
sdest=$dest/include/graehl/shared
echo installing graehl/shared into $sdest
mkdir -p $sdest
cd shared
for f in *.h *.hpp *.hh *.cc *.cpp *.C; do
 cp $f $sdest
done
