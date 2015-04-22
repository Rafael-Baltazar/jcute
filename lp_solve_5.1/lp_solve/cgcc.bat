@echo off

REM This batch file compiles the lp_solve driver program with the GNU gcc compiler under Windows


set src=../bfp/lp_MDO.c ../commonlib.c ../colamd/colamd.c ../myblas.c ../lp_rlp.c ../lp_crash.c ../bfp/bfp_etaPFI/lp_etaPFI.c ../lp_Hash.c ../lp_lib.c ../lp_wlp.c ../lp_matrix.c ../lp_mipbb.c ../lp_MPS.c ../lp_presolve.c ../lp_price.c ../lp_pricePSE.c ../lp_report.c ../lp_scale.c ../lp_simplex.c lp_solve.c ../lp_SOS.c ../lp_utils.c ../yacc_read.c

set c=gcc

%c% -I.. -I../bfp -I../bfp/bfp_etaPFI -I../colamd -O3 -DBFP_CALLMODEL=__stdcall -DYY_NEVER_INTERACTIVE -DPARSER_LP %src% -o lp_solve.exe
