#!/bin/bash
g++ $* -I/cache/boost -I.. LazyKbestTrees_test.cpp -o LazyKbestTrees_test && ./LazyKbestTrees_test
