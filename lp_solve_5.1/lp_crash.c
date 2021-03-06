
/*
   ----------------------------------------------------------------------------------
   Crash management routines in lp_solve v5.0+
   ----------------------------------------------------------------------------------
    Author:        Kjell Eikland
    Contact:       kjell.eikland@broadpark.no
    License terms: LGPL.

    Requires:      lp_lib.h, lp_utils.h, lp_matrix.h

    Release notes:
    v1.0.0  1 April   2004      First version.
    v1.1.0  20 July 2004        Reworked with flexible matrix storage model.

   ----------------------------------------------------------------------------------
*/

#include <string.h>

#include "commonlib.h"
#include "lp_lib.h"
#include "lp_utils.h"
#include "lp_report.h"
#include "lp_matrix.h"
#include "lp_crash.h"

#ifdef FORTIFY
# include "lp_fortify.h"
#endif


MYBOOL crash_basis(lprec *lp)
{
  int     i;
  MATrec  *mat = lp->matA;

  /* Initialize basis indicators */
  if(lp->basis_valid)
    lp->var_basic[0] = FALSE;
  else
    default_basis(lp);

/*
  for(i = 0; i <= lp->sum; i++)
    lp->is_basic[i] = FALSE;
  for(i = 1; i <= lp->rows; i++)
    lp->is_basic[lp->var_basic[i]] = TRUE;
*/

  if((lp->crashmode == CRASH_MOSTFEASIBLE) && mat_validate(mat)) {
    /* The logic here follows Maros */
    LLrec   *rowLL = NULL, *colLL = NULL;
    int     ii, rx, cx, ix, nz;
    REAL    wx, tx, *rowMAX = NULL, *colMAX = NULL;
    int     *rowNZ = NULL, *colNZ = NULL, *rowWT = NULL, *colWT = NULL;
    REAL    *value;
    int     *rownr, *colnr;

    report(lp, NORMAL, "crash_basis: Basis crashing selected\n");

    /* Tally row and column non-zero counts */
    allocINT(lp,  &rowNZ, lp->rows+1,     TRUE);
    allocINT(lp,  &colNZ, lp->columns+1,  TRUE);
    allocREAL(lp, &rowMAX, lp->rows+1,    FALSE);
    allocREAL(lp, &colMAX, lp->columns+1, FALSE);
    nz = mat_nonzeros(mat);
    rownr = &COL_MAT_ROWNR(0);
    colnr = &COL_MAT_COLNR(0);
    value = &COL_MAT_VALUE(0);
    for(i = 0; i < nz;
        i++, rownr += matRowColStep, colnr += matRowColStep, value += matValueStep) {
      rx = *rownr;
      cx = *colnr;
      wx = fabs(*value);
      rowNZ[rx]++;
      colNZ[cx]++;
      if(i == 0) {
        rowMAX[rx] = wx;
        colMAX[cx] = wx;
        colMAX[0]  = wx;
      }
      else {
        rowMAX[rx] = my_max(rowMAX[rx], wx);
        colMAX[cx] = my_max(colMAX[cx], wx);
        colMAX[0]  = my_max(colMAX[0],  wx);
      }
    }
    /* Reduce counts for small magnitude to preserve stability */
    rownr = &COL_MAT_ROWNR(0);
    colnr = &COL_MAT_COLNR(0);
    value = &COL_MAT_VALUE(0);
    for(i = 0; i < nz;
        i++, rownr += matRowColStep, colnr += matRowColStep, value += matValueStep) {
      rx = *rownr;
      cx = *colnr;
      wx = fabs(*value);
#ifdef CRASH_SIMPLESCALE
      if(wx < CRASH_THRESHOLD * colMAX[0]) {
        rowNZ[rx]--;
        colNZ[cx]--;
      }
#else
      if(wx < CRASH_THRESHOLD * rowMAX[rx])
        rowNZ[rx]--;
      if(wx < CRASH_THRESHOLD * colMAX[cx])
        colNZ[cx]--;
#endif
    }

    /* Set up priority tables */
    allocINT(lp, &rowWT, lp->rows+1, TRUE);
    createLink(lp->rows,    &rowLL, NULL);
    for(i = 1; i <= lp->rows; i++) {
      if(get_constr_type(lp, i)==EQ)
        ii = 3;
      else if(lp->upbo[i] < lp->infinite)
        ii = 2;
      else if(fabs(lp->rhs[i]) < lp->infinite)
        ii = 1;
      else
        ii = 0;
      rowWT[i] = ii;
      if(ii > 0)
        appendLink(rowLL, i);
    }
    allocINT(lp, &colWT, lp->columns+1, TRUE);
    createLink(lp->columns, &colLL, NULL);
    for(i = 1; i <= lp->columns; i++) {
      ix = lp->rows+i;
      if(is_free(lp, i))
        ii = 3;
      else if(lp->upbo[ix] >= lp->infinite)
        ii = 2;
      else if(fabs(lp->upbo[ix]-lp->lowbo[ix]) > lp->epsmachine)
        ii = 1;
      else
        ii = 0;
      colWT[i] = ii;
      if(ii > 0)
        appendLink(colLL, i);
    }

    /* Loop over all basis variables */
    for(i = 1; i <= lp->rows; i++) {

      /* Select row */
      rx = 0;
      wx = -lp->infinite;
      for(ii = firstActiveLink(rowLL); ii > 0; ii = nextActiveLink(rowLL, ii)) {
        tx = rowWT[ii] - CRASH_SPACER*rowNZ[ii];
        if(tx > wx) {
          rx = ii;
          wx = tx;
        }
      }
      if(rx == 0)
        break;
      removeLink(rowLL, rx);

      /* Select column */
      cx = 0;
      wx = -lp->infinite;
      for(ii = mat->row_end[rx-1]; ii < mat->row_end[rx]; ii++) {

        /* Update NZ column counts for row selected above */
        tx = fabs(ROW_MAT_VALUE(ii));
        ix = ROW_MAT_COLNR(ii);
#ifdef CRASH_SIMPLESCALE
        if(tx >= CRASH_THRESHOLD * colMAX[0])
#else
        if(tx >= CRASH_THRESHOLD * colMAX[ix])
#endif
          colNZ[ix]--;
        if(!isActiveLink(colLL, ix) || (tx < CRASH_THRESHOLD * rowMAX[rx]))
          continue;

        /* Now do the test for best pivot */
        tx = my_sign(get_OF_raw(lp, lp->rows+ix)) - my_sign(ROW_MAT_VALUE(ii));
        tx = colWT[ix] + CRASH_WEIGHT*tx - CRASH_SPACER*colNZ[ix];
        if(tx > wx) {
          cx = ix;
          wx = tx;
        }
      }
      if(cx == 0)
        break;
      removeLink(colLL, cx);

      /* Update row NZ counts */
      ii = mat->col_end[cx-1];
      rownr = &COL_MAT_ROWNR(ii);
      value = &COL_MAT_VALUE(ii);
      for(; ii < mat->col_end[cx];
          ii++, rownr += matRowColStep, value += matValueStep) {
        wx = fabs(*value);
        ix = *rownr;
#ifdef CRASH_SIMPLESCALE
        if(wx >= CRASH_THRESHOLD * colMAX[0])
#else
        if(wx >= CRASH_THRESHOLD * rowMAX[ix])
#endif
          rowNZ[ix]--;
      }

      /* Set new basis variable */
      setBasisVar(lp, rx, lp->rows+cx);
    }

    /* Clean up */
    FREE(rowNZ);
    FREE(colNZ);
    FREE(rowMAX);
    FREE(colMAX);
    FREE(rowWT);
    FREE(colWT);
    freeLink(&rowLL);
    freeLink(&colLL);
  }
  return( TRUE );
}


MYBOOL guess_basis(lprec *lp, REAL *guessvector, int *basisvector)
{
  MYBOOL status = FALSE;
  REAL   *values, *violation, *value, error, upB, loB, sortorder = 1.0;
  int    i, n, *rownr, *colnr;
  MATrec *mat = lp->matA;

  if(!mat_validate(lp->matA))
    return( status );

  /* Create helper arrays */
  allocREAL(lp, &values, lp->sum+1, TRUE);
  allocREAL(lp, &violation, lp->sum+1, TRUE);

  /* Compute values of slack variables for given guess vector */
  i = 0;
  n = get_nonzeros(lp);
  rownr = &COL_MAT_ROWNR(i);
  colnr = &COL_MAT_COLNR(i);
  value = &COL_MAT_VALUE(i);
  for(; i < n; i++, rownr += matRowColStep, colnr += matRowColStep, value += matValueStep)
    values[*rownr] += unscaled_mat(lp, my_chsign(is_chsign(lp, *rownr), *value), *rownr, *colnr) *
                      guessvector[*colnr];
  MEMMOVE(values+lp->rows+1, guessvector+1, lp->columns);

  /* Initialize constraint bound violation measures */
  for(i = 1; i <= lp->rows; i++) {
    upB = get_rh_upper(lp, i);
    loB = get_rh_lower(lp, i);
    error = values[i] - upB;
    if(error > lp->epsprimal)
      violation[i] = sortorder*error;
    else {
      error = loB - values[i];
      if(error > lp->epsprimal)
        violation[i] = sortorder*error;
      else if(is_infinite(lp, loB) && is_infinite(lp, upB))
        ;
      else if(is_infinite(lp, upB))
        violation[i] = sortorder*(loB - values[i]);
      else if(is_infinite(lp, loB))
        violation[i] = sortorder*(values[i] - upB);
      else
        violation[i] = - sortorder*MAX(upB - values[i], values[i] - loB);
    }
    basisvector[i] = i;
  }

  /* Initialize user variable bound violation measures */
  for(i = 1; i <= lp->columns; i++) {
    n = lp->rows+i;
    upB = get_upbo(lp, i);
    loB = get_lowbo(lp, i);
    error = guessvector[i] - upB;
    if(error > lp->epsprimal)
      violation[n] = sortorder*error;
    else {
      error = loB - values[n];
      if(error > lp->epsprimal)
        violation[n] = sortorder*error;
      else if(is_infinite(lp, loB) && is_infinite(lp, upB))
        ;
      else if(is_infinite(lp, upB))
        violation[n] = sortorder*(loB - values[n]);
      else if(is_infinite(lp, loB))
        violation[n] = sortorder*(values[n] - upB);
      else
        violation[n] = - sortorder*MAX(upB - values[n], values[n] - loB);
    }
    basisvector[n] = n;
  }

  /* Sort decending by violation; this means that variables with
     the largest violations will be designated as basic */
  sortByREAL(basisvector, violation, lp->sum, 1, FALSE);

  /* Adjust the non-basic indeces for the (proximal) bound state */
  error = lp->epsprimal;
  for(i = lp->rows+1, rownr = basisvector+i; i <= lp->sum; i++, rownr++) {
    if(*rownr <= lp->rows) {
      if(values[*rownr] <= get_rh_lower(lp, *rownr)+error)
        *rownr = -(*rownr);
    }
    else
      if(values[i] <= get_lowbo(lp, (*rownr)-lp->rows)+error)
        *rownr = -(*rownr);
  }

  /* Clean up and return status */
  status = (MYBOOL) (violation[1] == 0);
  FREE(values);
  FREE(violation);


  return( status );
}
