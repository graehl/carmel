. ~graehl/isd/hints/bashlib.sh
export PATH=~graehl/t/graehl/util:$PATH
in=${1:-10.eng-parse}
pre=${pre:-{$in%.eng-parse}.}
showvars_required in pre
export local=1
savemap=tmp.count.map iomr-hadoop $in ${pre}counted ./pcfg-map ./count.py
savemap=tmp.sums.map iomr-hadoop ${pre}counted ${pre}lhs-sums ./lhs-sums-map ./count.py
./lhs-sums-map ${pre}counted | mapsort | ./count.py > ${pre}lhs-sums
./cat-pcfg-for-divide ${pre}lhs-sums ${pre}counted
