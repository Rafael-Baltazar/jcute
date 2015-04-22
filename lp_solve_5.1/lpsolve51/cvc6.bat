@echo off

REM This batch file compiles the lpsolve libraries with the Microsoft Visual C/C++ compiler under Windows

set src=../bfp/lp_MDO.c ../commonlib.c ../myblas.c ../colamd/colamd.c ../lp_rlp.c ../lp_crash.c ../bfp/bfp_etaPFI/lp_etaPFI.c ../lp_Hash.c ../lp_lib.c ../lp_wlp.c ../lp_matrix.c ../lp_mipbb.c ../lp_MPS.c ../lp_presolve.c ../lp_price.c ../lp_pricePSE.c ../lp_report.c ../lp_scale.c ../lp_simplex.c ../lp_SOS.c ../lp_utils.c ../yacc_read.c

set c=cl

rc lpsolve.rc
%c% -I.. -I../bfp -I../bfp/bfp_etaPFI -I../colamd /LD /MD /O2 /Zp8 /Gz -D_WINDLL -D_USRDLL -DWIN32 -DYY_NEVER_INTERACTIVE -DPARSER_LP %src% lpsolve.res ..\lp_solve.def -o lpsolve51.dll
rem /link /LINK50COMPAT

if exist a.obj del a.obj
%c% -I.. -I../bfp -I../bfp/bfp_etaPFI -I../colamd /MT /O2 /Zp8 /Gd /c -DWIN32 -DYY_NEVER_INTERACTIVE -DPARSER_LP %src%
lib *.obj /OUT:liblpsolve51.lib

if exist a.obj del a.obj
%c% -I.. -I../bfp -I../bfp/bfp_etaPFI -I../colamd /MTd /O2 /Zp8 /Gd /c -DWIN32 -DYY_NEVER_INTERACTIVE -DPARSER_LP %src%
lib *.obj /OUT:liblpsolve51d.lib

if exist *.obj del *.obj
