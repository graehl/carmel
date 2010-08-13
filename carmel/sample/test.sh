#!/bin/bash
set -e
for f in *fs*; do echo $f
../bin/$HOST/carmel $f >/dev/null; done
