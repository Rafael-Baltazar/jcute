@echo off

REM This batch file compiles the demo program with the gnu gcc compiler under DOS/Windows

REM There are two ways to do that: use the lpsolve code directly or use that static library.

rem link lpsolve code with application
set src=../bfp/lp_MDO.c ../commonlib.c ../myblas.c ../colamd/colamd.c ../fortify.c ../lp_rlp.c ../lp_crash.c ../bfp/bfp_etaPFI/lp_etaPFI.c ../lp_Hash.c ../lp_lib.c ../lp_wlp.c ../lp_matrix.c ../lp_mipbb.c ../lp_MPS.c ../lp_presolve.c ../lp_price.c ../lp_pricePSE.c ../lp_report.c ../lp_scale.c ../lp_simplex.c ../lp_SOS.c ../lp_utils.c ../yacc_read.c

rem statically link lpsolve library
rem set src=../lpsolve51/liblpsolve51.a

set c=gcc

%c% -I.. -I../bfp -I../bfp/bfp_etaPFI -I../colamd -O3 -DYY_NEVER_INTERACTIVE -DPARSER_LP demo.c %src% -o demo.exe
