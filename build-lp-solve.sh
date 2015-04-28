#!/bin/bash
JCUTE_HOME=/vagrant
# Build lp_solve
cd lp_solve_5.1/lpsolve51
dos2unix ccc
sh ccc
cp liblpsolve51.so $JCUTE_HOME
cd $JCUTE_HOME

# Build lp_solve Java Wrapper
cp build-lp-solve-java-lib-linux.sh lp_solve_5.1_java/lib/linux/build
cd lp_solve_5.1_java/lib/linux
dos2unix build
sh build
cp liblpsolve51j.so $JCUTE_HOME
cd $JCUTE_HOME
