@echo off

REM This batch file compiles the demo program with the Microsoft Visual C/C++ compiler under Windows

set src=../bfp/lp_MDO.c ../commonlib.c ../myblas.c ../colamd/colamd.c ../fortify.c ../lp_rlp.c ../lp_crash.c ../bfp/bfp_etaPFI/lp_etaPFI.c ../lp_Hash.c ../lp_lib.c ../lp_wlp.c ../lp_matrix.c ../lp_mipbb.c ../lp_MPS.c ../lp_presolve.c ../lp_price.c ../lp_pricePSE.c ../lp_report.c ../lp_scale.c ../lp_simplex.c demo.c ../lp_SOS.c ../lp_utils.c ../yacc_read.c
set c=cl

%c% -I.. -I../bfp -I../bfp/bfp_etaPFI -I../colamd /O2 /Zp8 /Gd -DWIN32 -DYY_NEVER_INTERACTIVE -DPARSER_LP %src% -o demo.exe

if exist *.obj del *.obj
