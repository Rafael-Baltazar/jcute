
# -------------------------------------------------------------------
# This is a build file for the lp_solve Java wrapper stub library
# on linux platforms.
#
# Requirements and preconditions:
#
# - gcc and g++ compiler installed (I used gcc Version 3.3.1)
# - Sun JDK 1.4 installed
# - lp_solve linux archive (lp_solve5.tar.gz) unpacked
#
# Change the paths below this line and you should be ready to go!
# -------------------------------------------------------------------

LPSOLVE_DIR=/vagrant/lp_solve_5.1/
JDK_DIR=/usr/lib/jvm/jdk1.7.0_65/

SRC_DIR=../../src/c
INCL="-I $JDK_DIR/include -I $JDK_DIR/include/linux -I $LPSOLVE_DIR -I $SRC_DIR"

g++ -fpic $INCL -c $SRC_DIR/lpsolve5j.cpp
g++ -shared -Wl,-soname,liblpsolve51j.so -o liblpsolve51j.so lpsolve5j.o -lc -llpsolve51 -L/vagrant/
