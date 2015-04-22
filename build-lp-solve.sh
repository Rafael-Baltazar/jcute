#!/bin/bash
# Build lp_solve
cd lp_solve_5.1/lpsolve51
dos2unix ccc
sh ccc
cp liblpsolve51.so ..
cd ..

# Build lp_solve Java Wrapper
cp build-lp-solve-java-lib-linux.sh lp_solve_5.1_java/lib/linux/build
cd lp_solve_5.1_java/lib/linux
dos2unix build
sh build
cp liblpsolve51j.so ..
cd ..
