set -x
g++ $* -O0 -ffast-math -I.. weight_underflow.cpp -I../../boost -o weight_underflow && ./weight_underflow
#g++ -DSINGLE_PRECISION -I.. weight_underflow.cpp -I../../boost -o weight_underflow_single

