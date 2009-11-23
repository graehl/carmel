set -x
$carmel --train-cascade -aHJmM -1 cipher2 plain.bi.wfsa subst.wfst --priors=1e5,1e-2 --fem-norm=norm --fem-forest=forest --fem-param=param --normby=NC --fem-alpha=alpha
