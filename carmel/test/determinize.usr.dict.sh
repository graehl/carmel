#!/bin/bash
set -x
input=${input:-/usr/share/dict/words}
if [ ! "$skipdict" ] ; then
../make-dictionary.pl -r $input > dict.words.char.fsa.random
carmel -jn dict.words.char.fsa.random > dict.words.char.fsa
cp dict.words.char.fsa looped.dict.words.char.fsa
echo '(END 0 " ")' >> looped.dict.words.char.fsa
fi
carmel --minimize --minimize-determinize $* dict.words.char.fsa -F det.dict.fsa
carmel --minimize --minimize-determinize $* looped.dict.words.char.fsa -F det.looped.dict.fsa
carmel -kO 20 det.dict.fsa
carmel -kO 20 dict.words.char.fsa
carmel -kO 20 det.looped.dict.fsa
carmel -kO 20 looped.dict.words.char.fsa
