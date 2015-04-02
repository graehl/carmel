# Carmel finite-state toolkit - Jonathan Graehl

 (carmel includes EM and gibbs-sampled (pseudo-Bayesian) training)

 (see `carmel/LICENSE` - free for research/non-commercial)

 (see `carmel/README` and `carmel/carmel-tutorial`).

# pre-built binaries

TODO

# building

```
ln -sf . graehl;cd carmel; INSTALL_PREFIX=/usr/local make -j 4 install
```

(prerequisites: C++ compiler and [Boost](http://boost.org), which you
probably already have on your linux system; for Mac, you can get them
from [Homebrew](http://brew.sh/). Windows: you can use Microsoft's
tools or cygwin or mingw.

## other `make` options

If your system doesn't support static linking, `make NOSTATIC=1`

If you're trying to modify or troubleshoot the build, take a look at
`shared/graehl.mk` as well as `carmel/Makefile`; you shouldn't need to
manually run `make depend`.

# Subdirectories

* `carmel`: finite state transducer toolkit with EM and gibbs-sampled
  (pseudo-Bayesian) training

* `forest-em`: derivation forests EM and gibbs (dirichlet prior bayesian) training

* `shared`: utility C++/Make libraries used by carmel and forest-em

* `gextract`: some python bayesian syntax MT rule inference

* `sblm`: some simple pcfg (e.g. penn treebank parses, but preferably binarized)

* `clm`: some class-based LM feature? I forget.

* `cipher`: some word-class discovery and unsupervised decoding of simple
probabilistic substitution cipher (uses carmel, but look to the tutorial in
carmel/ first)

* `util`: misc shell/perl scripts
