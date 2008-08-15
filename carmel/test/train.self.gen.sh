#!/bin/bash
set -x
carmel=${carmel:-carmel}
$carmel -g 1 train.a.w > corpus.a.1 
$carmel -g 100 train.a.w > corpus.a.100
$carmel -S corpus.a.100 train.a.u >/dev/null 
$carmel -t corpus.a.100 train.a.u | tee trained.a.w
echo original:
$carmel train.a.w
