#!/bin/bash
B=${B:-../bin/$ARCH/carmel}
which $B
set -x
if [ ! "$skipdict" ] ; then
../make-dictionary.pl /usr/share/dict/words > dict.words.char.fsa
cp dict.words.char.fsa looped.dict.words.char.fsa
echo '(END 0 " ")' >> looped.dict.words.char.fsa
fi
carmel --minimize --minimize-determinize dict.words.char.fsa -F det.dict.fsa
carmel --minimize --minimize-determinize looped.dict.words.char.fsa -F det.looped.dict.fsa
carmel -kO 100 det.looped.dict.fsa
