Introduction
------------
What is lp_solve and what is it not?
The simple answer is, lp_solve is a Mixed Integer Linear Programming (MILP) solver.

It is a free (see LGPL for the GNU lesser general public license) linear (integer) programming solver
based on the revised simplex method and the Branch-and-bound method for the integers.
It contains full source, examples and manuals.
lp_solve solves pure linear, (mixed) integer/binary, semi-continuous and
special ordered sets (SOS) models.

See the reference guide for more information.


lp_solve 5.1
------------

The engine of this new version has changed fundamentally (thanks to Kjell Eikland).  The result
is more speed and more stability, which means that larger and tougher models can be solved.
The file and module structure of lp_solve is now also a very much-improved platform for further
development.  Check lp_solve.chm, or the HTML help files for a fuller description changes since
version 4.0.  Begin by taking a look at 'Changes compared to version 4' and 'lp_solve usage'
This gives a good starting point.

lp_solve v5 is herewith released.  It is a major enhancement compared to v4 in
terms of functionality, usability, accessibility and documentation.  While the
new core v5 engine has been tested for 6 months, including a beta phase of 3 months,
there are areas of the code that contain placeholders for coming functionality
and can therefore not be expected to be verified.  This includes partial and
multiple pricing and, to a lesser extent, Steepest Edge-based pricing.  In
addition, the development team is aware of issues related to particulare settings
that in some cases can make worst-case performance unsatisfactory.

For example, the default tolerance settings are known to be very thight compared
to many other solvers.  This can in a few very rare cases lead to excessive
running times or that lp_solve determines the model to be infeasible.  We have
chosen to keep the tight settings since in many cases we have found that other
solvers produce solutions even if the underlying solution quality is unacceptable
or strictly considered even infeasible.  We may, however, decide to selectively
loosen tolerances in a v5 subrelease if we feel that this is warranted.

See the reference guide for more information.


BFP's
-----

BFP stands for Basis Factorization Package, which is a unique lp_solve feature.  Considerable
effort has been put in this new feature and we have big expectations for this. BFP is a generic
interface model and users can develop their own implementations based on the provided templates.
We are very interested in providing as many different BFPs as possible to the community.

lp_solve has the v3.2 etaPFI built in as default engine, versioned 1.0,  In addition two other
BFPs are included for both Windows and Linux: bfp_LUSOL.dll, bfp_etaPFI.dll for Windows and
libbfp_LUSOL.so, libbfp_etaPFI.so for Linux.  The standalone bfp_etaPFI is v2.0 and includes
advanced column ordering using the COLAMD library, as well as better pivot management for
stability.  For complex models, however, the LU factorization approach is much better, and
lp_solve now includes LUSOL as one of the most stable engines available anywhere.  LUSOL was
originally developed by Prof. Saunders at Stanford, and it has now been ported to C
and enhanced by Kjell.

A third BFP based on GLPK may be included later, but license issues must first be resolved.

If you compile BFPs yourself, make sure that under Windows, you use __stdcall convention and
use 8 byte alignments.  This is needed for the BFPs to work correctly with the general
distribution of lp_solve and also to make sharing BFPs as uncomplicated as possible.

See the reference guide for more information.


XLI's
-----

XLI stands for eXternal Language Interface, also a unique lp_solve feature. XLI's are stand-alone
libraries used as add-on to lp_solve to make it possible to read and write lp models in a format
not natively supported by lp_solve. Examples are CPLEX lp format, LINDO lp format, MathProg format,
XML format...

See the reference guide for more information.

lpsolve API
-----------

Don't forget that the API has changed compared to previous versions of lpsolve and that you just
can't use the version 5 lpsolve library with your version 4 or older code.  That is also the
reason why the library is now called lpsolve51.dll/lpsolve51.a.  lpsolve51.dll or lpsolve51.a are
only needed when you call the lpsolve library via the API interface from your program.
The lp_solve program is still a stand-alone executable program.

There are examples interfaces for different language like C, VB, C#, VB.NET, Java,
Delphi, and there is now also even a COM object to access the lpsolve library.  This means that
the door is wide-open for using lp_solve in many different situations.  Thus everything that is
available in version 4 is now also available in version 5 and already much more!

See the reference guide for more information.


Conversion between lp modeling formats
--------------------------------------

Note that lp2mps and mps2lp don't exist anymore. However this functionality is now implemented
in lp_solve:

lp2mps can be simulated as following:
lp_solve -parse_only -lp infile -wmps outfile

mps2lp can be simulated as following:
lp_solve -parse_only -mps infile -wlp outfile


via the -rxli option, a model can be read via an XLI library and via the -wxli option, a model
can be written via an XLI library.


How to build the executables yourself.
---------------------------------------

At this time, there are no Makefiles yet. However for the time being, there are batch files/scripts
to build. For the Microsoft compiler under Windows, use cvc6.bat, for the gnu compiler under Windows,
use cgcc.bat and for Unix/Linux, use the ccc shell script (sh ccc).

See the reference guide for more information.


IDE
---

Under Windows, there is now also a very user friendly lpsolve IDE. Check out LPSolveIDE

See the reference guide for more information.


Documentation (reference guide)
-------------------------------

See lp_solve51.chm for a Windows HTML help documentation file.
The html files are also in lp_solve_5.1_doc.tar.gz. Start with index.htm
Also see http://geocities.com/lpsolve/ for a on-line documentation


Change history:
---------------

??/??/?? version incorrect, probably 5.0.0.0
- Some small bugs are solved
- print_lp reported wrong information on ranges on constraints
- A new function print_tableau is added
- The demo program failed at some points. Fixed
- Solving time is improved again. Calculation of sensitivity is only done when asked for.

01/05/04 version incorrect, probably 5.0.0.0
- Some compiler warnings solved
- New API function print_str
- Some functions were not exported in the lpsolve5 dll. Fixed.

02/05/04 version incorrect, probably 5.0.0.0
- Solved some small bugs
- New API routine is_maxim added.
- New API routine is_constr_type added.
- New API routine is_piv_rule added.
- Routine get_reduced_costs renamed to get_dual_solution.
- Routine get_ptr_reduced_costs renamed to get_ptr_dual_solution.
- Return codes of solve API call are changed.
- ANTIDEGEN codes have changed. See set_anti_degen.

08/05/04 version 5.0.4.0
- Routine write_debugdump renamed to print_debugdump
- Routine set_scalemode renamed to set_scaling
- Routine get_scalemode renamed to get_scaling
- New API routine set_add_rowmode added
- New API routine is_add_rowmode added
- Fix of a B&B subperformance issue
- File lp_lp.h renamed to lp_wlp.h
- File lp_lp.c renamed to lp_wlp.c
- File lp_lpt.h renamed to lp_rlpt.h
- File lp_lpt.c renamed to lp_rlpt.c
- File read.h renamed to yacc_read.h
- File read.c renamed to yacc_read.c
- File lex.l renamed to lp_rlp.l
- File lp.y renamed to lp_rlp.y
- File lex.c renamed to lp_rlp.h
- File lp.c renamed to lp_rlp.c

If you build your model via the routine add_constraint, then take a look at the
new routine set_add_rowmode. It will result in a spectacular performance improvement
to build your model.

17/05/04 version 5.0.5.0
- Reading from lp and mps file is considerably made faster. Especially for larger models
  The improvement can be spectacular.
  Also writing lp and mps files are made faster.
- Improvements in B&B routines. Big models take less memory. Can be up to 50%
- Under Windows, the byte alignment is changed to 4 (before it was 1 in version 5).
  This results in some speed improvements. Test gave some 5% improvement.
- is_anti_degen did not return the correct status with ANTIDEGEN_NONE. Fixed.
- is_presolve did not return the correct status with PRESOLVE_NONE. Fixed.
- Under non-windows system, BFP_CALLMODEL doesn't have to be specified anymore

25/05/04 version 5.0.6.0
- lp_solve5.zip now also contains the needed header files to build a C/C++ application.
  The source files that use the lpsolve API functions must now include lp_lib.h
  lpkit.h is obsolete. It is only used internally in the lpsolve library.
  Developers using the lpsolve library must use lp_lib.h
  The examples are changed accordingly.
- set_piv_rule renamed to set_pivoting
- get_piv_rule renamed to get_pivoting
- is_piv_strategy renamed to is_piv_mode
- Constants INFEASIBLE, UNBOUNDED, DEGENERATE, NUMFAILURE have new values
- New solve return value: SUBOPTIMAL
- When there are negative bounds on variables such that these variables must be split in
  a negative and a positive part and solve was called multiple time, then each time the solve
  was done, a new split was done. This could lead to bad performance. Corrected.
- API function get_row is now considerably faster, especially for large models.
- When compiled with VC, some warnings were given about structure alignment. Solved.
- Creation of an mps file (via write_mps) sometimes created numbers with the wrong sign.
  This problem was introduced in 5.0.5.0. Corrected.
- In several source files there were improper /* */ comments. Corrected.
- Several improvements/corrections in the algorithms.
- Some corrections in writing the model to an lp/lpt file.

29/05/04 version 5.0.7.0
- A small error corrected in the lp parser. If there was a constraint with both a lower
  and upper restriction on the same line and with only one variable, then the parser did
  not accept this. For example:
  R1: 1 <= x <= 2;
  This is now corrected.
  Note that normally it is preferred to make of this a bound instead of a constraint:
  1 <= x <= 2;
  This results in a smaller model (less constraints) and faster solution times.
  By adding the label: prefix it is forced as a constraint. This can be for testing purposes
  of in some special circumstances.
- Return values in case of user abort and timeout is now cleaner, and makes it possible to recover
  sub optimal values directly (if the problem was feasible)..
- calculate_duals and sensitivity duals could not be constructed in the presence of free variables
  when these were deleted. The deletion / cleanup of free variables was enforced in the previous beta.
- Added constant PRESOLVE_DUALS so that the program precalculates duals, and always if there are free
  variables. The PRESOLVE_SENSDUALS are only calculated if the user has explicitly asked for it, even
  if there are split variables. The only way to get around this problem is to convert the simplex
  algorithm to a general bounded version, which is a v6 issue.
- Presolve is accelerated significantly for large models with many row and column removals.
  This is done by moving the basis definition logic to after the presolve logic.
- Factorization efficiency statistics were sometimes computed incorrectly with non-etaPFI BFPs.
  This is now fixed without any effect on the user.
- Some minor tuning of bfp_LUSOL has been done.  There will be more coming in this department later.
- Routines set_add_rowmode and is_add_rowmode were not available in the lpsolve5.dll dll. Corrected.
- Constant PRESOLVE_SENSDUALS has new value. Before it was 128. Now it is 256.

04/06/04 version 5.0.8.0
- There was an error in the CPLEX lp parser. When there is a constraint (Subject To section) with only
  one variable (like -C1 >= -99900), the file could not be read. Fixed.
  Note that it is better to have bounds on this kind of constraints. (C1 <= 99900). This gives
  better performance. On the other hand, presolve detects these conditions and auto converts them
  to bounds.
- Under Windows, byte alignment is now set to 8 (previously it was 4). Gives some better performance.
- There were a couple of problems when a restart was done. lp_solve failed, gave numerical unstable
  errors, NUMFAILURE return code.
  Also when the objective function was changed after a solve and solve was done again, lp_solve
  stated that the model was infeasible.
  Also when the objective function or an element in the matrix was changed and a previous non-zero value
  was set to zero, the matrix was blown up with unpredictable effects. Even an unhandled exception error/
  core dump could be the result of this problem.
  These problems should be solved now.
- There was a potential problem in the MPS reader. When column entries were not in ascending order,
  the model could not be read (add_columnex error message).
  Note that lp_solve always generates column entries in ascending order. So the problem only
  occurred when the MPS file was created outside of lp_solve.
- add_columnex did not return FALSE if column entries were not in ascending order.
- Added the option -S7 to the lp_solve program to also print the tableau
- Added a functional index in the lp_solve API reference. API calls are grouped per functionality.
  This can be a great help to find which API routines are needed at which time.
- Added in the manual a section about Basis Factorization Packages (BFPs)

08/06/04 version 5.0.8.1
- There was a memory leak when solve was called multiple times after each other. Fixed.

18/06/04 version 5.0.9.0
- There was a possible memory overrun in some specific cases. This could result in protection
  errors and crashes.
- The default branch-and-bound depth limit is now set to 50 (get_bb_depthlimit)
- There are two new API functions: add_constraintex and set_obj_fnex
- On some systems, the macro CHAR_BIT is already defined and the compiler gave a warning about it. Fixed.
- variable vector is renamed. In some situations this confliced with a macro definition.
- Before, version 5 had to be compiled with __stdcall calling convention. This is no longer
  required. Any calling convention is now ok.
- Big coefficients (>2147483647) in the matrix were handled incorrectly resulting in totally
  wrong results. Corrected.
- column_in_lp now returns an integer indicating the column number where it is found in the lp
  If the column is not in the lp it returns 0.
- get_orig_index and get_lp_index returned wrong values if the functions were used before solve.
  Fixed.
- New routines added: add_constraintex and set_obj_fnex
- New constants PRICE_MULTIPLE and PRICE_AUTOPARTIAL added for set_pivoting/get_pivoting/is_piv_mode
  added new options -pivm and -pivp to lp_solve program to support these new constants.
- New routines read_XLI, write_XLI, has_XLI, is_nativeXLI added to support user written
  model language interfaces. Not yet documented in the manual. Here is a short explanation:
    As you know, lp_solve supports reading and writing of lp, mps and CPLEX lp style model files.
    In addition, lp_solve is interfaced with other systems via LIB code link-ins or the DLL,
    but this has never been an officially supported part of lp_solve.

    To facilitate the development of other model language interfaces, a quite
    simple "eXternal Language Interface" (XLI) has been defined, which allows
    lp_solve to be dynamically configured (at run-time) to use alternative XLIs.
    At present, a MathProg (AMPL subset) interface is in place, and the template
    is provided for other platforms. An XML-based interface is likely to come
    fairly soon also. Obviously, all existing interface methods will remain
    unchanged.

21/07/04 version 5.0.10.0
- This is the first official release of version 5!!!
- The default branch-and-bound depth limit is not 50 as specified in 5.0.9.0, but -50
  This indicates a relative depth limit. See the docs.
- The callback functions were not working properly because they didn't have the __stdcall
  attributes. Fixed. Also modified the VB, VB.NET, CS.NET examples to demonstrate the
  callback features.
- lp_colamdMDO.c and lp_colamdMDO.h renamed to lp_MDO.c and lp_MDO.h
- lp_pricerPSE.c and lp_pricerPSE.h renamed to lp_price.h/lp_pricePSE.h and lp_price.c/lp_pricePSE.c
- New routines read_freemps, read_freeMPS, write_freemps, write_freeMPS for reading/writing
  free MPS format.
- Added options -mfps, -wfmps to lp_solve command line program to handle free MPS files
- There was an error in set_obj_fnex when not in row add mode such that the object function
  was not set correct. Fixed.
- read_XLI, write_XLI now has an extra parameter: options
- Revised default_basis, set_basis to be more robust for wrong provided data.
  set_basis now returns a boolean indicating success of not.
- several internal improvements
- New pricer strategies constants: PRICE_PARTIAL, PRICE_LOOPLEFT, PRICE_LOOPALTERNATE.
- PRICE_ADAPTIVE, PRICE_RANDOMIZE have new values.
- All API functions are now available via the lp structure. This is needed for the XLIs.
- MathProg XLI now uses lp_solves verbose level to print messages.
- MathProg XLI fix for constant term in constraint.
- Fixed possible memory overrun in report function.
- added options -rxli, -rxlidata, -rxliopt to lp_solve command line program to read with XLI library
- added options -wxli, -wxliopt to lp_solve command line program to write with XLI library
- write_lpt, write_LPT now writes to the output set by set_outputstream/set_outputfile when NULL is
  given as filename, stream. Previously output was then written to stdout.
- set_break_at_value (-o option in lp_solve program) didn't work. Fixed.
- The lp format now also allows constant terms in the objective function.
  This value is added with the objective function value.
  This constant was already allowed by the MPS format and is also written when an lp file
  is created (see lp-format in help file).
- Fixed a runtime/protection/core dump error when a timeout occured.
- Unbounded models were not reported as such. Fixed.

01/09/04 version 5.1.0.0
- write_LP, write_MPS, write_freeMPS are now also available via the dll/shared library.
- There was a small problem with negative infinite variables. Corrected.
- The lp and CPLEX lp parsers sometimes returned wrong line numbers when the function was called
  multiple times in the same thread. Fixed.
- The lp and CPLEX lp reader did not accept numbers that end with a dot (.). Example 1. Now it does.
- When there was no objective function and integer variables, then a wrong solution
  or infeasible solution could be given. Corrected.
- When a solve was done again after a previous solve with some added/deleted constraints or columns
  then a wrong solution could be reported. Corrected.
- SOS may not be greater than 2 at this moment because of a design problem which isn't solved yet.
- Presolve sometimes changed the model such that it gave a different solution than without presolve. Fixed.
- Presolve sometimes incorrectly identified models as feasible when they were in fact infeasible. Corrected.
- Matrix code is optimised resulting in some 10% performance increase.
- The internal memory model has been changed to a more conventional format, which can also improve speed
  on some models due to cache effects.  The old memory model can be restored via a compiler setting, if desired.
- There is now a default reporting of the numeric accuracy of the computed results.
  This can help in identifying numerically unstable models.
- set_epsilon acted as set_epspivot instead of setting the integer tolerance. Fixed.
- get_epsilon, set_epsilon renamed to get_epsint, set_epsint.
- Some default tolerances are revised. This makes more models solvable by lp_solve and improves speed.
  (get_epspivot is now 1e-6)
  (get_epsint is now 1e-7)
- When set_outputfile is called to redirect output to a file, then the file was not automatically
  closed when delete_lp was called. Now it is. Note however that when set_outputstream is used
  to redirect to another stream that this stream is not closed when delete_lp is called.
- When there was an empty row in the model (example: <= 5;) then write_lp wrote this row as in the
  example, but when this model was read again by lp_solve it generated an error. Now write_lp
  detects this and uses the first variable in the model with a coefficient of 0 to write the row
  (example: 0 x <= 5;). This is then accepted by lp_solve. Note that such a row cannot just be
  ignored. For example with a >= constraint, the model is infeasible and if the row would be ignored
  it could become feasible.
- The CPLEX lp routines are not more available in lp_solve (write_lpt, read_lpt). However there is
  an XLI that provides the functionality to read and write a CPLEX lp file, so no functionality
  is lost.
- There is now an XLI to read and write the LINDO lp format.
- There is now an XLI to read and write a model in XML format (LPFML schema).
- There is a new routine get_status to get the extended status of a routine.
- The default lp name is now empty instead of "Unnamed"
- When there is in the MPS format a NAME section without a name, then the model name was undefined.
  Sometimes with strange characters. This is fixed. Is now an empty name.
- There are new routines set_row, set_rowex, set_column, set_columnex to set the values of an existing
  row/column. This is more performant than set_mat when multiple values must be set.
- New routine get_nonzeros to return the number of non-zeros in the model.
- New routine is_infinite to check a given value against "infinite"
- A negative lower bound smaller than -1e6 in an MPS file was seen as a -infinite lower bound. Fixed.
- Solution accuracy reporting has been implemented and will be produced by default
  (strangely, most other solvers don't even bother to check the accuracy of the final result ...).
- There was a problem with the DepthFirst B&B option (-Bf in lp_solve, NODE_DEPTHFIRSTMODE
  in set_bb_rule) resulting in returning a wrong solution. Fixed.
- The objective function upper limit of a variable with solution value 0 could in some rare
  situations be wrong. Fixed.
- The objective function limits were in some circumstances not always correct. Fixed
- The objective function limits were incorrect when the objective direction was maximization. Fixed.
- There was a problem with integer restrictions on negative variables. Fixed
- get_splitnegvars and set_splitnegvars routines are removed from the lp_solve library.
- The lp_solve program has a new option -wafter. This is used in combination with -wlp, -wmps, -wfmps
  or -wxli to indicate that the model must be written after solving the model. This is usefull when
  presolve is active such that the changed model by presolve is written instead of the original model.
- The lp_solve program has a new option -nonames. This option has as result that column and row names
  in the model are ignored. The lp_solve names will be used instead. This can be usefull if there are
  names used that are not allowed in some other format.
- The lp_solve program has a new option -noint. This option has as result that integer restricions in
  the read model are ignored.
- get_sensitivity_objex and get_ptr_sensitivity_objex are again implemented as in release 4.

10/09/04 version 5.1.0.1
- A constraint can now also be defined as free (FR). This to temporary disable a constraint.
  see set_constr_type.
- check_constraint didn't work correctly with an infinite RHS value or infinite range. Fixed.
- There was a problem with set_column(ex). Fixed
- There was a problem with set_row(ex). Fixed
- set_column(ex) was not correct documented. It also sets the objective function.
- set_row was not correct documented. Values start at element 1, not 0.
- There was a problem in presolve which could cause infeasibility in a situation where a singleton
  row was fixed at a non-zero value. Fixed.
- When a lower *and* upper bound is set on a variable and one of them is (-)Infinity ((-)1e30)
  then (-)1e30 was printed instead of (-)Inf. Changed.
- Enhanced LUSOL accuracy management and changed tolerance defaults.
- Modified LUSOL tolerance function.
- Minor iteration curtailment.
- Matrix function updates.
- Revised tolerance epspivot. It is now 2.0e-07
- If you compile the lpsolve code yourself, you must now also add myblas.c to the files to compile.
- Several enhancements in the bfp routines
- There is now also a GLPK bfp library.
- Renamed variable vector to myvector to avoid C++ compilation problems.
- API routine add_columnex doesn't require anymore that values are provided in ascending order.
- There is a new B&B mode: NODE_BREADTHFIRSTMODE. The lp_solve program uses the new option -BB for this.

21/09/04 version 5.1.0.2
- Correction in the Mathprog xli reader. Some models were read incorrect. Fixed.
- Improved performance for presolve with large models.
- Correction in myblas.h/myblas.c for duplication declaration of symbols on some platforms.
- Moved some files to different folders:
    lp_BFP.def, lp_BFP.h, lp_BFP1.c, lp_BFP2.c, lp_MDO.c, lp_MDO.h to bfp folder
    lp_etaPFI.h, lp_etaPFI.c to bfp/bfp_etaPFI
    lp_glpkLU.h, lp_glpkLU.c to bfp/bfp_GLPK
    lp_LUSOL.h, lp_LUSOL.c to bfp/bfp_LUSOL
    lp_XLI.def, lp_XLI.h, lp_XLI1.c, lp_XLI2.c to xli
    lp_LPFML.h, lp_LPFML.c to xli/xli_LPFML
    lp_MathProg.h, lp_MathProg.c to xli/xli_MathProg
  This was done to have a better separation of code.
  Note that lp_solve uses as default the bfp_etaPFI Basic Factorization Package and because of this,
  the following directories must be in your include path:
    bfp;bfp/bfp_etaPFI
  Most command line C compilers do this via the option -I:
    -Ibfp -Ibfp/bfp_etaPFI
  Don't forget this when you compile the lpsolve code yourself or you will get errors when the code is
  compiled that some header files can not be found.
- Added compile scripts for Mac OS X (ccc.osx)
- Some small corrections in documentation.

03/10/04 version 5.1.0.3
- Made some small modifications to the code to fix some compiler errors with g++ 3.3.1
- Fixed a line comment error in lp_price.h
- Made add_constraint(ex) more performant with large models
- There was a problem with set_row(ex). Fixed
- Updated the AMPL driver to support version 5.1
  See help file for more information about the AMPL driver. There is a new section about using
  lpsolve with AMPL.
- Updated the documentation on ratio's. It now also describes how to handle a ratio in the objective.
- Added some extra -degen* options to the lp_solve program.

06/10/04 version 5.1.0.4
- There was still a problem with set_row(ex). Fixed
- add_constraint(ex) made some more performant
- presolve could result in a model that was less stable, especially when scaling was used.
  This should now be ok (disabled AggressiveRowPresolve by default).
- When there was an empty column in the model and a presolve on columns was done, then
  the model could get corrupted. Fixed.
- Added an installation script for Unix/Linux (install.sh)
- There was a possible memory overrun in set_basis. Fixed.
- Conditionally added the ll attribute to the MAXINT64 constant definition. Especially
  done for g++

24/10/04 version 5.1.0.5
- There was a problem with set_basis. It sometimes didn't accept the provided base, even when
  it was correct. Fixed.
- Improved presolve performance, especially for larger models.
- Added the warm start feature to the AMPL driver. See documentation.

11/11/04 version 5.1.0.6
- There was a severe memory leak. Fixed.
- set_columnex failed if 1+Nrows elements were provided (one of being objective function value).
- When presolve was done *and* there are variables split because of -Inf lower bound, then
  variable names could be changed to __AntiBodyOf(x)__. Fixed.
- There was a possible problem when set_mat was called after a solve and then a restart was done.
- The Windows dlls can now also be compiled with the gcc compiler via the cgcc.bat batch files.
- When an old BFP/XLI library was provided that is not compatible with the current version of
  lp_solve then it could result in crashes. Fixed.
- set_BFP and set_XLI now give a message why they failed.
- It is now also possible to call set_BFP to change from BFP package after a solve.
  Previously this gave problems.
- There was a problem with the LUSOL and GLPK bfps when a restart was done after adding extra constraints.
  Solved.
- The etaPFI and GLPK bfps are updated for (possible) linker issues.

28/11/04 version 5.1.1.0
- Because a small change had to be done to the lp structure to fix a problem with some compilers,
  the version becomes 5.1.1.0 and if you use bfp's then you also need the new ones. If the old
  bfps are used with this new version, they will not be accepted.
- On some systems, a crash occurs when bfps are used. This should be fixed now.
- There was a possible protection error/core dump when delete_lp is called. Fixed.
- There was a problem with the DepthFirst branch-and-bound mode (-Bf option to lp_solve).
  The returned solution was not integer. Fixed
- There was a possible crash with the WeightReverse branch-and-bound mode (-Bw option to lp_solve).
  Fixed.
- set_columnex sometimes failed (returnvalue 0) while it shouldn't. Fixed.
- New scaling mode option SCALE_DYNUPDATE added. See reference guide for more information.
- bfp_LUSOL read some memory before allocated space. Most of the time this is no problem, but some
  systems and debuggers can give problems or report this. Fixed.
- When a model is scaled and there are columns and/or rows that are entirely empty, it could
  happen that bounds/ranges on variables/constraints were corrupted. Fixed.
- Sometimes a model is reported to be infeasible or unbounded while it isn't. There was some code
  in lp_solve that didn't use tolerances, while it should. Changed.
- Added a new node selection rule for the B&B algorithm: NODE_AUTOORDER
  This is still experimental, but we would like people to test this on their models with integer
  variables to see if it improves performance.
  In the lp_solve driver program this is activated via the -Bo option.
- When variables are named and a model is first solved and then a column is added and a solve is
  done again, it happened that the added column values were not reported. Fixed.

20/12/04 version 5.1.1.1
- set_obj_fnex did not clear the non-specified values as all other ex functions do. Now it does.
- With the lp_solve program, if a timeout was set and it occurs and there was an integer solution
  found, then this solution was not reported. Fixed. Before, the option -timeoutok was needed to
  enable this, now the option isn't needed anymore. It is however still accepted.
- print_tableau didn't flush the stream when output is redirected to file. Fixed.
- There was a possible problem in presolve when variables were deleted. Fixed.
- Activated on several API routines a test on correct provided row/column number.
- A new API routine get_nameindex is added to get the column/row index number for a given
  column/row name.

04/01/05 version 5.1.1.2
- set_obj_fnex sometimes set the provided values on wrong columns. Fixed.
- When get_nameindex was called before a name was set, a protection error or core dump occured. Fixed.
- added two new options in lp_solve driver: -presolverow and -presolvecol to only presolve rows or columns.
- If bb_depthlimit was set to 0 (via set_bb_depthlimit or via lp_solve -depth option) then the depth limit
  was not unlimited as specified in the manual, but integer restrictions were ignored. Fixed.
- The -s and -s0 lp_solve options didn't scale the model at all. Now they are identical as option -s4
  as documented.
- There was a problem with the lp_solve -piv combination options
  (-pivf, -pivm, -piva, -pivr, -pivll, -pivla). They were not taken as such. Fixed.
- Added include <sys/types.h> where malloc.h is included. Some platforms like OS X need this.

xx/xx/xx version 5.1.1.3
- The lp parser could not read lp files with SOS definitions where variables weights were floating
  points. It only accepted integer (or no) weights. Fixed.
- There is no limit anymore in the lp parser on the maximum length of a variable of constraint.
  Before, names could not be longer than 24 characters.
- In some cases, where SOS variables are used, lp_solve reported at the end marginal numerical accuracy
  while this was not the case. Fixed.
- When columns are added after a solve and there are SOS variables in the model, then SOS priorities
  were not correctly adapted. Fixed.
- Modified presolve for better performance.
- When set_obj_fnex was called in row order mode and there was at least one constraint in the model,
  then the objective was set as an extra constraint instead of the objective function. Fixed.
- set_row sometimes didn't set zero value on zero. They were kept. Fixed.
- When negative lower bounds are set on integer variables, lp_solve could report wrong or infeasible
  solution. Fixed.
- A crash could occur when anti-degeneration was turned on. Fixed.
- A test was added in delete_lp, if a NULL lprec pointer is provided that the function exists
  instead of crashing.
- A change was done such that models with a lot of bound flips would now run substantially faster.
- In some rare cases, variable lower and upper bounds were not given correct initial values resulting
  in random bounds on them. Fixed
- lp_solve always called srand() to initialise the randomize library. Now this is only done when
  random numbers are really used.
- Added option -Wl,-Bsymbolic in the ccc shell scripts that build shared libraries on Linux.
  This to make sure that symbols are resolved on the current library and not on other libraries.
- Added instructions in the reference guide how to compile the MATLAB driver under Linux.
- Added two new API routines: read_basis and write_basis. This to read/write the basis from/to an
  MPS bas formatted file.
  The lp_solve command line program has the new options -rbas and -wbas to make this possible.
  See the reference guide for more information about providing a basis and the MPS bas file format.
- Added in the reference guide a section about scaling.
- Added a driver to call lp_solve from Octave.

We are thrilled to hear from you and your experiences with this new version. The good and the bad.
Also we would be pleased to hear about your experiences with the different BFPs on your models.

Please send reactions to:
Peter Notebaert: peno@mailme.org
Kjell Eikland: kjell.eikland@broadpark.no
