#!/bin/bash
B=${B:-../bin/cage/carmel.debug}
../src/sri2fsa.pl < tiny.sri > tiny.fsa;$B tiny.fsa -YZB > tiny.dot;dot -O -Tpdf tiny.dot;dot -O -Gdpi=150 -Tpng tiny.dot
