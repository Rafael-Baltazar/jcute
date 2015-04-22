#ifndef HEADER_lp_presolve
#define HEADER_lp_presolve

#include "lp_types.h"

/* -------------------------------------------------------------------------------------------- */
/* Defines for various presolve options                                                         */
/* -------------------------------------------------------------------------------------------- */
#define LegacyPresolveCompact       /* Define this to use the slow old presolve compacting type */

#define MAX_PSLINDEPLOOPS       2      /* Upper limit to detecting linear dependend constraints */

#define DoPresolveRounding
/*#define DoPresolveRelativeTest*/     

#define RowPresolveLevel  0
#if RowPresolveLevel >= 1
  #define UseFullConstraintInfo                  /* Use bound sums for non-singular constraints */
#elif RowPresolveLevel >= 2
  #define AggressiveRowPresolve                      /* Extra row presolve (beware of accuracy) */
#endif


#define PRESOLVE_EPSVALUE  lp->epsprimal
/*#define PRESOLVE_EPSROUND  lp->epsprimal*/

#define PRESOLVE_EPSROUND  lp->epsint 


typedef struct _presolverec
{
  int *rowlength;
  int *collength;
  int *plucount;
  int *negcount;
  int *pluneg;
  REAL *plulower;
  REAL *neglower;
  REAL *pluupper;
  REAL *negupper;
  LLrec *rowmap;
  LLrec *colmap;
  MYBOOL forceupdate;
} presolverec;

#ifdef __cplusplus
extern "C" {
#endif

/* Put function headers here */

STATIC MYBOOL presolve_createUndo(lprec *lp);
STATIC MYBOOL inc_presolve_space(lprec *lp, int delta, MYBOOL isrows);
STATIC MYBOOL presolve_setOrig(lprec *lp, int orig_rows, int orig_cols);
STATIC MYBOOL presolve_fillUndo(lprec *lp, int orig_rows, int orig_cols, MYBOOL setOrig);
STATIC MYBOOL presolve_freeUndo(lprec *lp);

STATIC int presolve(lprec *lp);
STATIC MYBOOL postsolve(lprec *lp, int status);

#ifdef __cplusplus
 }
#endif

#endif /* HEADER_lp_presolve */
