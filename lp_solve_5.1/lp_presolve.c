
/* -------------------------------------------------------------------------
   Presolve routines for lp_solve v5.0+
   -------------------------------------------------------------------------
    Author:        Kjell Eikland
    Contact:       kjell.eikland@broadpark.no
    License terms: LGPL.

    Requires:      lp_lib.h, lp_presolve, lp_crash.h, lp_scale.h

    Release notes:
    v5.0.0  1 January 2004      Significantly expanded and repackaged
                                presolve routines.
    v5.0.1  1 April   2004      Added reference to new crash module
    v5.1.0  20 August 2004      Reworked infeasibility detection.
                                Added encapsulation of presolve undo logic.
    v5.1.1  10 September 2004   Added variable bound tightening based on
                                full-constraint information, as well as
                                variable fixing by duality.
    v5.2.0  1 January 2005      Fixes to bound fixing handling.
                                Added fast batch compression after presolve.
                                Restructured calls by adding presolve wrapper.

   ------------------------------------------------------------------------- */

#include <string.h>
#include "commonlib.h"
#include "lp_lib.h"
#include "lp_presolve.h"
#include "lp_crash.h"
#include "lp_scale.h"
#include "lp_report.h"

#ifdef FORTIFY
# include "lp_fortify.h"
#endif


STATIC MYBOOL presolve_createUndo(lprec *lp)
{
  if(lp->presolve_undo != NULL)
    presolve_freeUndo(lp);
  lp->presolve_undo = (presolveundorec *) calloc(1, sizeof(presolveundorec));
  if(lp->presolve_undo == NULL)
    return( FALSE );
  return( TRUE );
}
STATIC MYBOOL inc_presolve_space(lprec *lp, int delta, MYBOOL isrows)
{
  int i, ii, oldrowcolalloc, rowcolsum, oldrowalloc;
  presolveundorec *psundo = lp->presolve_undo;

  if(psundo == NULL) {
    presolve_createUndo(lp);
    psundo = lp->presolve_undo;
  }

  /* Set constants */
  oldrowalloc = lp->rows_alloc-delta;
#if 1
  oldrowcolalloc = lp->sum_alloc-delta;
#else
  oldrowcolalloc = lp->sum_alloc;
  lp->sum_alloc += delta;
#endif
  rowcolsum = lp->sum_alloc + 1;

  /* Reallocate lp memory */
  allocREAL(lp, &psundo->fixed_rhs,   lp->rows_alloc+1, AUTOMATIC);
  allocINT(lp,  &psundo->var_to_orig, rowcolsum, AUTOMATIC);
  allocINT(lp,  &psundo->orig_to_var, rowcolsum, AUTOMATIC);

  /* Fill in default values, where appropriate */
  for(i = oldrowcolalloc+1, ii = oldrowalloc+1; i < rowcolsum; i++, ii++) {
    psundo->var_to_orig[i] = 0;
    psundo->orig_to_var[i] = 0;
    if(isrows)
      psundo->fixed_rhs[ii] = 0;
  }

  return(TRUE);
}
STATIC MYBOOL presolve_setOrig(lprec *lp, int orig_rows, int orig_cols)
{
  presolveundorec *psundo = lp->presolve_undo;

  if(psundo == NULL)
    return( FALSE );
  psundo->orig_rows = orig_rows;
  psundo->orig_columns = orig_cols;
  if(lp->wasPresolved)
    presolve_fillUndo(lp, orig_rows, orig_cols, FALSE);
  return( TRUE );
}
STATIC MYBOOL presolve_fillUndo(lprec *lp, int orig_rows, int orig_cols, MYBOOL setOrig)
{
  int i;
  presolveundorec *psundo = lp->presolve_undo;

  for(i = 0; i <= orig_rows; i++) {
    psundo->var_to_orig[i] = i;
    psundo->orig_to_var[i] = i;
    psundo->fixed_rhs[i]   = 0;
  }
  for(i = 1; i <= orig_cols; i++) {
    psundo->var_to_orig[orig_rows + i] = i;
    psundo->orig_to_var[orig_rows + i] = i;
  }
  if(setOrig)
    presolve_setOrig(lp, orig_rows, orig_cols);

  return( TRUE );
}
STATIC MYBOOL presolve_freeUndo(lprec *lp)
{
  presolveundorec *psundo = lp->presolve_undo;

  if(psundo == NULL)
    return( FALSE );
  FREE(psundo->orig_to_var);
  FREE(psundo->var_to_orig);
  FREE(psundo->fixed_rhs);
  FREE(lp->presolve_undo);
  return( TRUE );
}


/* ----------------------------------------------------------------------------- */
/* Presolve routines for tightening the model                                    */
/* ----------------------------------------------------------------------------- */

STATIC REAL presolve_round(lprec *lp, REAL value, MYBOOL isGE)
{
  REAL epsvalue = PRESOLVE_EPSROUND;

#ifdef DoPresolveRounding
  value += my_chsign(isGE, epsvalue/SCALEDINTFIXRANGE);
  value = restoreINT(value, epsvalue);
#endif
  return( value );
}

STATIC REAL presolve_precision(lprec *lp, REAL value)
{
  REAL epsvalue = PRESOLVE_EPSROUND;

#ifdef DoPresolveRounding
  value = restoreINT(value, epsvalue);
#endif
  return( value );
}

STATIC MYBOOL presolve_mustupdate(lprec *lp, int colnr)
{
#if 0
  return( is_infinite(lp, get_lowbo(lp, colnr)) ||
          is_infinite(lp, get_upbo(lp, colnr)) );
#else
  return( is_infinite(lp, lp->orig_lowbo[lp->rows+colnr]) ||
          is_infinite(lp, lp->orig_upbo[lp->rows+colnr]) );
#endif
}

STATIC REAL presolve_sumplumin(lprec *lp, int item, REAL *plu, REAL *neg)
{
  if(fabs(plu[item]) >= lp->infinite)
    return( plu[item] );
  else if(fabs(neg[item]) >= lp->infinite)
    return( neg[item] );
  else
    return( plu[item]+neg[item] );
}

STATIC char *get_constr_str(lprec *lp, int con_type)
{
  switch(con_type) {
    case FR: return("FR");
    case LE: return("LE");
    case GE: return("GE");
    case EQ: return("EQ");
    default: return("Error");
  }
}

STATIC MYBOOL presolve_rowfeasible(lprec *lp, int rownr, presolverec *psdata, MYBOOL userowmap)
{
  MYBOOL status = TRUE;
  int    contype;
  REAL   LHS, RHS, value;

  /* Optionally loop across all active rows in the provided map (debugging) */
  if(userowmap)
    rownr = firstActiveLink(psdata->rowmap);

  /* Now do once for ingoing rownr or loop across rowmap */
  while((status == TRUE) && (rownr != 0)) {

    /* Check the lower bound */
    value = presolve_sumplumin(lp, rownr, psdata->pluupper, psdata->negupper);
    LHS = get_rh_lower(lp, rownr);
    if(value < LHS-DEF_EPSSOLUTION) {
      contype = get_constr_type(lp, rownr);
      report(lp, NORMAL, "presolve: Lower bound infeasibility in %s row %d\n",
                          get_constr_str(lp, contype), rownr);
      status = FALSE;
    }

    /* Check the upper bound */
    value = presolve_sumplumin(lp, rownr, psdata->plulower, psdata->neglower);
    RHS = get_rh_upper(lp, rownr);
    if(value > RHS+DEF_EPSSOLUTION) {
      contype = get_constr_type(lp, rownr);
      report(lp, NORMAL, "presolve: Upper bound infeasibility in %s row %d\n",
                          get_constr_str(lp, contype), rownr);
      status = FALSE;
    }
    if(userowmap)
      rownr = nextActiveLink(psdata->rowmap, rownr);
    else
      rownr = 0;
  }
  return( status );
}

STATIC int presolve_validate(lprec *lp, LLrec *rowmap, LLrec *colmap)
{
  int i, j, errc = 0;

  /* Validate constraint bounds */
  for(i = 1; i < lp->rows; i++) {
    if((rowmap != NULL) && !isActiveLink(rowmap, i))
      continue;
    /* Check if we have a negative range */
    if(lp->orig_upbo[i] < 0) {
      errc++;
      report(lp, SEVERE, "presolve_validate: Detected negative range %g for row %d\n",
                         lp->orig_upbo[i], i);
    }
  }
  /* Validate variables */
  for(j = 1; j < lp->columns; j++) {
    if((colmap != NULL) && !isActiveLink(colmap, j))
      continue;
    i = lp->rows+j;
    /* Check if we have infeasible  bounds */
    if(lp->orig_lowbo[i] > lp->orig_upbo[i]) {
      errc++;
      report(lp, SEVERE, "presolve_validate: Detected UB < LB for column %d\n",
                         j);
    }
  }
  /* Return total number of errors */
  return( errc );
}

STATIC int presolve_collength(MATrec *mat, int column, LLrec *rowmap, int *collength)
{
  if(collength != NULL)
    column = collength[column];
  else if(rowmap == NULL) {
    column = mat_collength(mat, column);
  }
  else {
    int ib = mat->col_end[column-1],
        ie = mat->col_end[column];
    int *rownr;
    column = 0;
    rownr = &COL_MAT_ROWNR(ib);
    for(; ib < ie; ib++, rownr += matRowColStep) {
      if((*rownr == 0) ||
         isActiveLink(rowmap, *rownr))
        column++;
    }
  }
  return( column );
}

STATIC int presolve_rowlength(MATrec *mat, int row, LLrec *colmap, int *rowlength)
{
  if(rowlength != NULL)
    row = rowlength[row];
  else if(colmap == NULL)
    row = mat_rowlength(mat, row);
  else {
    int ib = 0, ie = mat->row_end[row];
    if(row > 0)
      ib = mat->row_end[row-1],
    row = 0;
    for(; ib < ie; ib++) {
      if(isActiveLink(colmap, ROW_MAT_COLNR(ib)))
        row++;
    }
  }
  return( row );
}

STATIC int presolve_nextcol(MATrec *mat, int rownr, int prevcol, LLrec *colmap)
/* Find the first active (non-eliminated) nonzero column in rownr after prevcol */
{
  int j, jj, jb = 0, je = mat->row_end[rownr];

  if(rownr > 0)
    jb = mat->row_end[rownr-1];
  for(; jb < je; jb++) {
#if 1
    /* Narrow search window */
    jj = (jb + je) / 2;
    if(jj > jb) {
      j = ROW_MAT_COLNR(jj);
      if(j <= prevcol) {
        jb = jj;
        continue;
      }
    }
#endif
    /* Test on the left hand side of the window */
    j = ROW_MAT_COLNR(jb);
    if((j > prevcol) && isActiveLink(colmap, j))
       return( jb );
  }
  return( mat->row_end[rownr] );
}
STATIC int presolve_nextrow(MATrec *mat, int colnr, int prevrow, LLrec *rowmap)
/* Find the first active (non-eliminated) nonzero row in colnr after prevrow */
{
  int *rownr, i, ii, ib = mat->col_end[colnr-1], ie = mat->col_end[colnr];

  rownr = &COL_MAT_ROWNR(ib);
  for(; ib < ie; ib++, rownr += matRowColStep) {
#if 1
    /* Narrow search window */
    ii = (ib + ie) / 2;
    if(ii > ib) {
      i = COL_MAT_ROWNR(ii);
      if(i <= prevrow) {
        rownr += (ii - ib)*matRowColStep;
        ib = ii;
        continue;
      }
    }
#endif
    /* Test on the left hand side of the window */
    if((*rownr > prevrow) && isActiveLink(rowmap, *rownr))
       return( ib );
  }
  return( mat->col_end[colnr] );
}

STATIC void presolve_rowupdate(lprec *lp, int rownr, int *collength, MYBOOL remove)
{
  if(collength != NULL) {
    if(remove) {
      int    ix, ie;
      MATrec *mat = lp->matA;

      ix = mat->row_end[rownr-1];
      ie = mat->row_end[rownr];
      for(; ix < ie; ix++)
        collength[ROW_MAT_COLNR(ix)]--;
    }
  }
}

/* Function to find if a variable can be fixed based on considering the dual */
STATIC MYBOOL presolve_coldualfix(lprec *lp, int colnr, LLrec *rowmap, REAL *fixValue)
{
  MYBOOL  hasOF, isMI, isDualFREE = TRUE;
  int     i, ix, ie, *rownr, signOF;
  REAL    *value, loX, upX;
  MATrec  *mat = lp->matA;

  /* First check basic variable range */
  loX = get_lowbo(lp, colnr);
  upX = get_upbo(lp, colnr);
  if(((loX < 0) && (upX > 0)) || (fabs(upX-loX) < lp->epsvalue))
    return( FALSE );
  isMI = (MYBOOL) (upX <= 0);

  /* Retrieve OF (standard form assuming maximization) */
  ix = mat->col_end[colnr - 1];
  ie = mat->col_end[colnr];
  rownr = &COL_MAT_ROWNR(ix);
  value = &COL_MAT_VALUE(ix);
  hasOF = (MYBOOL) (*rownr == 0);
  if(hasOF) {
    signOF = my_sign(*value);
    ix++;
    rownr += matRowColStep;
    value += matValueStep;
  }
  else
    signOF = 0;

  /* Loop over all constraints involving active variable (standard form with LE constraints)*/
  for(; (ix < ie) && isDualFREE;
      ix++, rownr += matRowColStep, value += matValueStep) {
    i = *rownr;
    if(!isActiveLink(rowmap, i))
      continue;
    isDualFREE = is_infinite(lp, get_rh_range(lp, i));
    if(isDualFREE) {
      if(signOF == 0)  /* Test on the basis of identical signs in the constraints */
        signOF = my_sign(*value);
      else             /* Test on the basis of constraint sign equal to OF sign */
        isDualFREE = (MYBOOL) (signOF == my_sign(*value));
    }
  }

  /* Set fixing value if we were successful */
  if(isDualFREE) {
    if(signOF > 0) {
      if(is_infinite(lp, loX))
        isDualFREE = FALSE;
      else {
        if(is_int(lp, colnr))
          *fixValue = ceil(loX-PRESOLVE_EPSVALUE);
        else
          *fixValue = loX;
      }
    }
    else {
      if(is_infinite(lp, upX))
        isDualFREE = FALSE;
      else {
        if(is_int(lp, colnr))
          *fixValue = floor(upX+PRESOLVE_EPSVALUE);
        else
          *fixValue = upX;
      }
    }
  }

  return( isDualFREE );
}

STATIC MYBOOL presolve_singletonbounds(lprec *lp, int rownr, int colnr, REAL *lobound, REAL *upbound, REAL *aval)
{
  REAL   coeff_a, epsvalue = PRESOLVE_EPSVALUE;
  MYBOOL isneg;

  /* Compute row singleton variable range */
  if(is_constr_type(lp, rownr, EQ) && (fabs(*lobound) < epsvalue))
    *lobound = *upbound = 0;
  else {
    if(aval == NULL)
      coeff_a = get_mat(lp, rownr, colnr);
    else
      coeff_a = *aval;
    isneg = (MYBOOL) (coeff_a < 0);
    if(*lobound > -lp->infinite)
      *lobound /= coeff_a;
    else if(isneg)
      *lobound = -(*lobound);
    if(*upbound < lp->infinite)
      *upbound /= coeff_a;
    else if(isneg)
      *upbound = -(*upbound);
    if(isneg)
      swapREAL(lobound, upbound);
  }

  /* Check against bound - handle SC variables specially */
  if(is_semicont(lp, colnr)) {
    coeff_a = get_lowbo(lp, colnr);
    if(coeff_a > 0) {
      *lobound = MAX(*lobound, 0.0);
      *upbound = MIN(*upbound, get_upbo(lp, colnr));
    }
    else {
      coeff_a = get_upbo(lp, colnr);
      if(coeff_a > 0) {
        *lobound = MAX(*lobound, get_lowbo(lp, colnr));
        *upbound = MIN(*upbound, 0.0);
      }
    }
  }
  else {
    *lobound = MAX(*lobound, get_lowbo(lp, colnr));
    *upbound = MIN(*upbound, get_upbo(lp, colnr));
    /* Attempt numeric error management / correction */
#if 0
    if(!is_infinite(lp, *lobound) && !is_infinite(lp, *upbound)) {
      coeff_a = ((*lobound) + (*upbound))*0.5;
      if((*upbound   < coeff_a) &&
         (my_reldiff(*upbound, coeff_a) > -epsvalue)) {
        my_roundzero(coeff_a, epsvalue);
        *lobound = coeff_a;
        *upbound = coeff_a;
      }
    }
#endif
  }

  /* Return with consistency status */
#ifdef DoPresolveRelativeTest
  return( (MYBOOL) (my_reldiff(*upbound, *lobound) >= - epsvalue) );
#else
  return( (MYBOOL) (*upbound >= *lobound - epsvalue) );
#endif
}

STATIC MYBOOL presolve_altsingletonvalid(lprec *lp, int rownr, int colnr, REAL reflotest, REAL refuptest)
{
  REAL coeff_bl, coeff_bu, epsvalue = PRESOLVE_EPSVALUE;

  coeff_bl = get_rh_lower(lp, rownr);
  coeff_bu = get_rh_upper(lp, rownr);

  /* Check base data validity */
#ifdef DoPresolveRelativeTest
  if((my_reldiff(refuptest, reflotest) < -epsvalue) ||
#else
  if((reflotest > refuptest + epsvalue) ||
#endif
     !presolve_singletonbounds(lp, rownr, colnr, &coeff_bl, &coeff_bu, NULL))
    return( FALSE );

  /* Base data is Ok, now check against against each other */
  if((reflotest > coeff_bu + epsvalue) ||
     (refuptest < coeff_bl - epsvalue))
    return( FALSE );
  else
    return( TRUE );
}

STATIC MYBOOL presolve_multibounds(lprec *lp, int rownr, int colnr,
                                   REAL *lobound, REAL *upbound, REAL *aval, presolverec *psdata)
{
  MYBOOL status = FALSE;
  REAL   coeff_a, LHS, RHS, netX, Xupper, Xlower;

  /* Get variable bounds for netting */
  LHS = *lobound;
  RHS = *upbound;
  Xlower = get_lowbo(lp, colnr);
  Xupper = get_upbo(lp, colnr);

  /* Identify opportunity for bound tightening */
  if(aval == NULL)
    coeff_a = get_mat(lp, rownr, colnr);
  else
    coeff_a = *aval;

  netX = presolve_sumplumin(lp, rownr, psdata->pluupper, psdata->negupper);
  if(!is_infinite(lp, LHS) && !is_infinite(lp, netX)) {
    if(coeff_a > 0) {
      LHS -= netX-coeff_a*Xupper;
      LHS /= coeff_a;
      if(LHS > Xlower) {
        Xlower = LHS;
        status = TRUE;
      }
    }
    else {
      LHS -= netX-coeff_a*Xlower;
      LHS /= coeff_a;
      if(LHS < Xupper) {
        Xupper = LHS;
        status = AUTOMATIC;
      }
    }
  }

  netX = presolve_sumplumin(lp, rownr, psdata->plulower, psdata->neglower);
  if(!is_infinite(lp, RHS) && !is_infinite(lp, netX)) {
    if(coeff_a < 0) {
      if(!is_infinite(lp, Xupper)) {
        RHS -= netX-coeff_a*Xupper;
        RHS /= coeff_a;
        if(RHS > Xlower) {
          Xlower = RHS;
          status |= TRUE;
        }
      }
    }
    else if(!is_infinite(lp, Xlower)) {
      RHS -= netX-coeff_a*Xlower;
      RHS /= coeff_a;
      if(RHS < Xupper) {
        Xupper = RHS;
        status |= AUTOMATIC;
      }
    }
  }

  *lobound = Xlower;
  *upbound = Xupper;
  return(status);
}

STATIC MYBOOL presolve_coltighten(lprec *lp, presolverec *psdata, int colnr, REAL LOnew, REAL UPnew, int *count)
{
  int     elmnr, elmend, k, oldcount = 0, newcount = 0;
  REAL    LOold, UPold, Value, margin = PRESOLVE_EPSVALUE;
  MATrec  *mat = lp->matA;
  REAL    *value;
  int     *rownr;

  /* Check if there is anything to do */
  LOold = get_lowbo(lp, colnr);
  UPold = get_upbo(lp, colnr);
#ifdef Paranoia
  if((LOold > LOnew) || (UPold < UPnew)) {
    report(lp, SEVERE, "presolve_coltighten: Inconsistent new bounds requested for column %d\n", colnr);
    return( FALSE );
  }
#endif
  if(count != NULL)
    newcount = *count;
  oldcount = newcount;

  /* Look for opportunity to tighten upper variable bound */
  if((UPnew < lp->infinite) && (UPnew+margin < UPold)) {
    if(is_int(lp, colnr)) {
      if(lp->columns_scaled && is_integerscaling(lp) && (ceil(UPnew) - UPnew > margin))
        UPnew = floor(UPnew) + margin;
      else
        UPnew = floor(UPnew);
    }
    if(UPold < lp->infinite) {
      elmnr = mat->col_end[colnr-1];
      elmend = mat->col_end[colnr];
      rownr = &COL_MAT_ROWNR(elmnr);
      value = &COL_MAT_VALUE(elmnr);
      for(; elmnr < elmend;
          elmnr++, rownr += matRowColStep, value += matValueStep) {
        k = *rownr;
        if((k > 0) && !isActiveLink(psdata->rowmap, k))
          continue;
        Value = unscaled_mat(lp, *value, k, colnr);
        Value = my_chsign(is_chsign(lp, k), Value);
        if((Value > 0) && (psdata->pluupper[k] < lp->infinite))
          psdata->pluupper[k] += (UPnew-UPold)*Value;
        else if((Value < 0) && (psdata->negupper[k] < lp->infinite))
          psdata->negupper[k] += (LOnew-LOold)*Value;
      }
    }
    else
      psdata->forceupdate = TRUE;
    if(UPnew < UPold) {
      UPold = UPnew;
      newcount++;
    }
  }

  /* Look for opportunity to tighten lower variable bound */
  if((LOnew > -lp->infinite) && (LOnew-margin > LOold)) {
    if(is_int(lp, colnr)) {
      if(lp->columns_scaled && is_integerscaling(lp) && (LOold - floor(LOold) > margin))
        LOnew = ceil(LOnew)-margin;
      else
        LOnew = ceil(LOnew);
    }
    if(LOold > -lp->infinite) {
      elmnr = mat->col_end[colnr-1];
      elmend = mat->col_end[colnr];
      rownr = &COL_MAT_ROWNR(elmnr);
      value = &COL_MAT_VALUE(elmnr);
      for(; elmnr < elmend;
          elmnr++, rownr += matRowColStep, value += matValueStep) {
        k = *rownr;
        if((k > 0) && !isActiveLink(psdata->rowmap, k))
          continue;
        Value = unscaled_mat(lp, *value, k, colnr);
        Value = my_chsign(is_chsign(lp, k), Value);
        if((Value > 0) && (psdata->plulower[k] > -lp->infinite))
          psdata->plulower[k] += (LOnew-LOold)*Value;
        else if((Value < 0) && (psdata->neglower[k] > -lp->infinite))
          psdata->neglower[k] += (UPnew-UPold)*Value;
      }
    }
    else
      psdata->forceupdate = TRUE;
    if(LOnew > LOold) {
      LOold = LOnew;
      newcount++;
    }
  }

  /* Now set the new variable bounds, if they are tighter */
  if(newcount > oldcount) {
#if 0     /* Experimental version */
    UPnew = presolve_round(lp, UPnew-margin, FALSE);
    LOnew = presolve_round(lp, LOnew+margin, TRUE);
#else     /* Safe version */
    UPnew = presolve_precision(lp, UPnew);
    LOnew = presolve_precision(lp, LOnew);
#endif
    if(LOnew > UPnew) {
      if(LOnew-UPnew < margin) {
        LOnew = UPnew;
      }
      else {
        report(lp, IMPORTANT, "presolve_coltighten: Found LB %g > UB %g for column %d\n",
                              LOnew, UPnew, colnr);
        return( FALSE );
      }
    }
    if(lp->spx_trace || (lp->verbose > DETAILED))
      report(lp, NORMAL, "presolve_coltighten: Replaced bounds on column %d to [%g ... %g]\n",
                         colnr, LOnew, UPnew);
    set_bounds(lp, colnr, LOnew, UPnew);
  }
  if(count != NULL)
    *count = newcount;

  return( TRUE );
}

STATIC int presolve_rowtighten(lprec *lp, presolverec *psdata, int rownr, int *tally)
{
  int    jx, jjx, ix, idxn = 0, *idxbound = NULL, status = RUNNING;
  REAL   *newbound = NULL, RHlo = get_rh_lower(lp, rownr), RHup = get_rh_upper(lp, rownr),
         VARlo, VARup;
  MATrec *mat = lp->matA;

  jx = presolve_rowlength(lp->matA, rownr, psdata->colmap, psdata->rowlength);
  allocREAL(lp, &newbound, 2*jx, TRUE);
  allocINT (lp, &idxbound, 2*jx, TRUE);

  /* Identify bound tighening for each active variable in the constraint */
  for(jx = presolve_nextcol(mat, rownr, 0, psdata->colmap); jx < mat->row_end[rownr];
      jx = presolve_nextcol(mat, rownr, jjx, psdata->colmap)) {
    jjx = ROW_MAT_COLNR(jx);

    VARlo = RHlo;
    VARup = RHup;
    ix = presolve_multibounds(lp, rownr,jjx, &VARlo, &VARup, NULL, psdata);
    if(ix & TRUE) {
      idxbound[idxn] = -jjx;
      newbound[idxn] = VARlo;
      idxn++;
    }
    if(ix & AUTOMATIC) {
      idxbound[idxn] = jjx;
      newbound[idxn] = VARup;
      idxn++;
    }
  }
  /* Loop over the bounds identified for tightening and perform update */
  ix = 0;
  while(ix < idxn) {
    jjx = idxbound[ix];
    jx = abs(jjx);
    VARlo = get_lowbo(lp, jx);
    VARup = get_upbo(lp, jx);
    while((ix < idxn) && (jx == abs(jjx))) {
      if(jjx < 0)
        VARlo = newbound[ix];
      else
        VARup = newbound[ix];
      ix++;
      jjx = idxbound[ix];
    }
    if(!presolve_coltighten(lp, psdata, jx, VARlo, VARup, tally)) {
      status = INFEASIBLE;
      report(lp, NORMAL, "presolve_rowtighten: Found variable bound infeasibility for column %d\n",
                          jx);
      break;
    }
  }
  FREE(newbound);
  FREE(idxbound);
  return(status);
}

STATIC int presolve_colsingleton(lprec *lp, presolverec *psdata, int i, int j, int *count)
{
  REAL  RHlow, RHup, LObound, UPbound, Value, margin;

  margin = PRESOLVE_EPSVALUE;

#ifdef Paranoia
  if(!isActiveLink(psdata->colmap, j))
    report(lp, SEVERE, "presolve_colsingleton: Nothing to do, column %d was eliminated earlier\n",
                       j);
#endif

  Value = get_mat(lp,i,j);
  if(Value == 0)
    return( RUNNING );

  /* Initialize and identify semicontinuous variable */
  LObound = get_lowbo(lp, j);
  UPbound = get_upbo(lp, j);
  if(is_semicont(lp, j) && (UPbound > LObound)) {
    if(LObound > 0)
      LObound = 0;
    else if(UPbound < 0)
      UPbound = 0;
  }

  /* Get singleton variable bounds */
  RHlow = get_rh_lower(lp, i);
  RHup  = get_rh_upper(lp, i);
  if(!presolve_singletonbounds(lp, i,j, &RHlow, &RHup, &Value))
    return( INFEASIBLE );

  if(presolve_coltighten(lp, psdata, j, RHlow, RHup, count))
    return( RUNNING );
  else
    return( INFEASIBLE );
}

STATIC MYBOOL presolve_colfix(lprec *lp, presolverec *psdata, int colnr, REAL newvalue, MYBOOL remove, int *tally)
{
  int     i, ix, ie;
  MYBOOL  isneg, lofinite, upfinite, doupdate = FALSE;
  REAL    lobound, upbound, lovalue, upvalue,
          Value, fixvalue, fixprod, mult, epsvalue = PRESOLVE_EPSVALUE;
  MATrec  *mat = lp->matA;
  REAL    *value;
  int     *rownr;

  /* Set "fixed" value in case we are deleting a variable */
  upbound = get_upbo(lp, colnr);
  lobound = get_lowbo(lp, colnr);
  if(remove) {
    if(upbound-lobound < epsvalue) {
      if((newvalue > lobound) && (newvalue < upbound))
        fixvalue = newvalue;
      else
        fixvalue = lobound; /* If scaling done before then : lp->orig_lowbo[lp->rows+colnr] */
    }
    else {
      if(is_infinite(lp, newvalue) && (get_mat(lp, 0, colnr) == 0))
        fixvalue = ((lobound <= 0) && (upbound >= 0) ? 0 : MIN(upbound, lobound));
      else
        fixvalue = newvalue;
    }
#if 1 /* Fast normal version */
    set_bounds(lp, colnr, fixvalue, fixvalue);
#else /* Slower version that can be used for debugging/control purposes */
    presolve_coltighten(lp, psdata, colnr, fixvalue, fixvalue, NULL);
    lobound = fixvalue;
    upbound = fixvalue;
#endif
    lp->full_solution[lp->presolve_undo->orig_rows +
                      lp->presolve_undo->var_to_orig[lp->rows + colnr]] = fixvalue;
    mult = -1;
  }
  else
    mult = 1;

  /* Adjust semi-continuous variable bounds to zero-base */
  if(is_semicont(lp, colnr) && (upbound > lobound)) {
    if(lobound > 0)
      lobound = 0;
    else if(upbound < 0)
      upbound = 0;
  }

  /* Loop over rows to update statistics */
  ix = mat->col_end[colnr - 1];
  ie = mat->col_end[colnr];
  rownr = &COL_MAT_ROWNR(ix);
  value = &COL_MAT_VALUE(ix);
  for(; ix < ie;
      ix++, rownr += matRowColStep, value += matValueStep) {

   /* Retrieve row data and adjust RHS if we are deleting a variable */
    i = *rownr;
    Value = *value;

    if(remove && (fixvalue != 0)) {
      fixprod = Value*fixvalue;
      lp->orig_rhs[i] -= fixprod;
      my_roundzero(lp->orig_rhs[i], epsvalue);
      lp->presolve_undo->fixed_rhs[i] += fixprod;
    }

   /* Prepare for further processing */
    Value = unscaled_mat(lp, Value, i, colnr);
    Value = my_chsign(is_chsign(lp, i), Value);
    isneg = (MYBOOL) (Value < 0);

   /* Reduce row variable counts if we are removing the variable */
    if((i > 0) && !isActiveLink(psdata->rowmap, i))
      continue;
    if(remove) {
      if(psdata->rowlength != NULL)
        psdata->rowlength[i]--;
      if(isneg)
        psdata->negcount[i]--;
      else
        psdata->plucount[i]--;
      if((lobound < 0) && (upbound >= 0))
        psdata->pluneg[i]--;
    }

   /* Compute associated constraint contribution values */
    upfinite = (MYBOOL) (upbound < lp->infinite);
    lofinite = (MYBOOL) (lobound > -lp->infinite);
    upvalue = my_if(upfinite, Value*upbound, my_chsign(isneg, lp->infinite));
    lovalue = my_if(lofinite, Value*lobound, my_chsign(isneg, -lp->infinite));

   /* Cumulate effective upper row bound (only bother with non-finite bound) */
    if(isneg) {
      if((psdata->negupper[i] < lp->infinite) && lofinite) {
        psdata->negupper[i] += mult*lovalue;
        psdata->negupper[i] = presolve_round(lp, psdata->negupper[i], FALSE);
      }
      else if(remove && !lofinite)
        doupdate = TRUE;
      else
        psdata->negupper[i] = lp->infinite;
    }
    else {
      if((psdata->pluupper[i] < lp->infinite) && upfinite) {
        psdata->pluupper[i] += mult*upvalue;
        psdata->pluupper[i] = presolve_round(lp, psdata->pluupper[i], FALSE);
      }
      else if(remove && !upfinite)
        doupdate = TRUE;
      else
        psdata->pluupper[i] = lp->infinite;
    }

   /* Cumulate effective lower row bound (only bother with non-finite bound) */
    if(isneg) {
      if((psdata->neglower[i] > -lp->infinite) && upfinite) {
        psdata->neglower[i] += mult*upvalue;
        psdata->neglower[i] = presolve_round(lp, psdata->neglower[i], TRUE);
      }
      else if(remove && !upfinite)
        doupdate = TRUE;
      else
        psdata->neglower[i] = -lp->infinite;
    }
    else {
      if((psdata->plulower[i] > -lp->infinite) && lofinite) {
        psdata->plulower[i] += mult*lovalue;
        psdata->plulower[i] = presolve_round(lp, psdata->plulower[i], TRUE);
      }
      else if(remove && !lofinite)
        doupdate = TRUE;
      else
        psdata->plulower[i] = -lp->infinite;
    }

   /* Validate consistency of eliminated singleton */
    if(remove && (psdata->rowlength[i] == 0) && !psdata->forceupdate) {
      lovalue = unscaled_value(lp, presolve_sumplumin(lp, i, psdata->plulower, psdata->neglower), i);
      upvalue = unscaled_value(lp, presolve_sumplumin(lp, i, psdata->pluupper, psdata->negupper), i);
      if((upvalue < get_rh_lower(lp, i)) ||
         (lovalue > get_rh_upper(lp, i)))
        return( FALSE );
    }
  }
  if(remove) {
    psdata->forceupdate |= doupdate;
    if(tally != NULL)
      (*tally)++;
  }
  return( TRUE );
}

STATIC void presolve_init(lprec *lp, presolverec *psdata)
{
  int    k, ix, ixx, colnr;
  REAL   lobound, upbound;
  MATrec *mat = lp->matA;
  REAL   *value;
  int    *rownr;

  /* First do tallies; loop over nonzeros by column */
  for(colnr = 1; colnr <= lp->columns; colnr++) {

    upbound = get_upbo(lp, colnr);
    lobound = get_lowbo(lp, colnr);
    if(is_semicont(lp, colnr) && (upbound > lobound)) {
      if(lobound > 0)
        lobound = 0;
      else if(upbound < 0)
        upbound = 0;
    }

    ix = mat->col_end[colnr - 1];
    ixx = mat->col_end[colnr];
    rownr = &COL_MAT_ROWNR(ix);
    value = &COL_MAT_VALUE(ix);
    for(; ix < ixx;
        ix++, rownr += matRowColStep, value += matValueStep) {

     /* Retrieve row data and prepare */
      k = *rownr;

     /* Cumulate counts */
      if(*value > 0)
        psdata->plucount[k]++;
      else
        psdata->negcount[k]++;
      if((lobound < 0) && (upbound >= 0))
        psdata->pluneg[k]++;
    }
  }
}

STATIC MYBOOL presolve_updatesums(lprec *lp, presolverec *psdata)
{
  int j;

  /* Initialize accumulation arrays */
  MEMCLEAR(psdata->pluupper, lp->rows + 1);
  MEMCLEAR(psdata->negupper, lp->rows + 1);
  MEMCLEAR(psdata->plulower, lp->rows + 1);
  MEMCLEAR(psdata->neglower, lp->rows + 1);

  /* Loop over active columns */
  for(j = firstActiveLink(psdata->colmap); j != 0; j = nextActiveLink(psdata->colmap, j)) {
    presolve_colfix(lp, psdata, j, lp->infinite, FALSE, NULL);
  }

  /* Check for abort prior to returning */
  return( !userabort(lp, -1) );
}

STATIC MYBOOL presolve_finalize(lprec *lp, presolverec *psdata)
{
  MYBOOL compactvars = FALSE;
  int    i, ke, kb, n;

  ke = lastInactiveLink(psdata->colmap);
  n = countInactiveLink(psdata->colmap);
  if((n > 0) && (ke > 0)) {
    compactvars = TRUE;
#ifndef LegacyPresolveCompact
    del_columnex(lp, psdata->colmap);
#else
    kb = lastActiveLink(psdata->colmap);
    while(kb > ke)
      kb = prevActiveLink(psdata->colmap, kb);
    ke++;
    while ((n > 0) && (ke > 0)) {
      for(i = ke-1; i > kb; i--) {
        del_column(lp, -i);
        n--;
      }
      ke = kb;
      kb = prevActiveLink(psdata->colmap, kb);
    }
#endif
    mat_colcompact(lp->matA, lp->presolve_undo->orig_rows,
                             lp->presolve_undo->orig_columns);
  }
  freeLink(&psdata->colmap);

  ke = lastInactiveLink(psdata->rowmap);
  n = countInactiveLink(psdata->rowmap);
  if((n > 0) && (ke > 0)) {
    compactvars = TRUE;
#ifndef LegacyPresolveCompact
    del_constraintex(lp, psdata->rowmap);
#else
    kb = lastActiveLink(psdata->rowmap);
    while(kb > ke)
      kb = prevActiveLink(psdata->rowmap, kb);
    ke++;
    while ((n > 0) && (ke > 0)) {
      for(i = ke-1; i > kb; i--) {
        del_constraint(lp, -i);
        n--;
      }
      ke = kb;
      kb = prevActiveLink(psdata->rowmap, kb);
    }
#endif
    mat_rowcompact(lp->matA);
  }
  freeLink(&psdata->rowmap);

  if(compactvars)
    varmap_compact(lp, lp->presolve_undo->orig_rows,
                       lp->presolve_undo->orig_columns);

  /* Validate matrix and reconstruct row indexation */
  return(mat_validate(lp->matA));
}

#define get_presolveloops(lp) MAXINT32

STATIC int presolve(lprec *lp)
{
  MYBOOL candelete;
  int    i,j,ix,iix, jx,jjx, n,nn=0, nc,nv,nb,nr,ns,nt;
  int    status, loops;
  REAL   Value, bound, test, epsvalue = PRESOLVE_EPSVALUE,
         initrhs0 = lp->orig_rhs[0];
  presolverec *psdata = NULL;
  MATrec *mat = lp->matA;

 /* Lock the variable mapping arrays and counts ahead of any row/column
    deletion or creation in the course of presolve, solvelp or postsolve */
  if(!lp->varmap_locked)
    varmap_lock(lp);

 /* Check if we have already done presolve */
  status = RUNNING;
  mat_validate(mat);
  if(lp->wasPresolved) {
    if(!lp->basis_valid) {
      crash_basis(lp);
      report(lp, DETAILED, "presolve: Had to repair broken basis.\n");
    }
    if((lp->solvecount > 1) && (lp->bb_level < 1) &&
       ((lp->scalemode & SCALE_DYNUPDATE) != 0))
      auto_scale(lp);
    return(status);
  }

  /* Produce original model statistics */
  REPORT_modelinfo(lp, TRUE, "SUBMITTED");

 /* Finalize basis indicators; if no basis was created earlier via
    set_basis or crash_basis then simply set the default basis. */
  if(!lp->basis_valid)
    lp->var_basic[0] = AUTOMATIC; /* Flag that we are presolving */

#if 0
write_lp(lp, "test_in.lp");    /* Write to lp-formatted file for debugging */
/*write_mps(lp, "test_in.mps");*/  /* Write to lp-formatted file for debugging */
#endif

 /* Do traditional simple presolve */
  yieldformessages(lp);
  nv = 0;
  nc = 0;
  nb = 0;
  nr = 0;
  ns = 0;
  nt = 0;

  if((lp->do_presolve & PRESOLVE_LASTMASKMODE) == PRESOLVE_NONE)
    mat_checkcounts(mat, NULL, NULL, TRUE);

  else {

    if(lp->full_solution == NULL)
      allocREAL(lp, &lp->full_solution, lp->sum_alloc+1, TRUE);

   /* Identify infeasible SOS'es prior to any pruning */
    for(i = 1; i <= SOS_count(lp); i++) {
      nn = SOS_infeasible(lp->SOS, i);
      if(nn > 0) {
        report(lp, NORMAL, "presolve: Found SOS %d (type %d) to be range-infeasible on variable %d\n",
                            i, SOS_get_type(lp->SOS, i), nn);
        status = INFEASIBLE;
        ns++;
      }
    }
    if(ns > 0)
      goto Finish;

   /* Create the presolve data record */
    psdata = (presolverec *) calloc(1, sizeof(*psdata));

   /* Create row and column counts */
    allocINT(lp,  &psdata->rowlength, lp->rows + 1, TRUE);
    psdata->forceupdate = TRUE;
    for(i = 1; i <= lp->rows; i++)
      psdata->rowlength[i] = presolve_rowlength(mat, i, NULL, NULL);
    allocINT(lp,  &psdata->collength, lp->columns + 1, TRUE);
    for(j = 1; j <= lp->columns; j++)
      psdata->collength[j] = presolve_collength(mat, j, NULL, NULL);

   /* Create NZ count and sign arrays, and do general initialization of row bounds */
    createLink(lp->rows, &psdata->rowmap, NULL);
      fillLink(psdata->rowmap);
    createLink(lp->columns, &psdata->colmap, NULL);
      fillLink(psdata->colmap);
    allocREAL(lp, &psdata->pluupper,  lp->rows + 1, FALSE);
    allocREAL(lp, &psdata->negupper,  lp->rows + 1, FALSE);
    allocREAL(lp, &psdata->plulower,  lp->rows + 1, FALSE);
    allocREAL(lp, &psdata->neglower,  lp->rows + 1, FALSE);

    allocINT(lp,  &psdata->plucount,  lp->rows + 1, TRUE);
    allocINT(lp,  &psdata->negcount,  lp->rows + 1, TRUE);
    allocINT(lp,  &psdata->pluneg,    lp->rows + 1, TRUE);
    presolve_init(lp, psdata);
    loops = 0;

   /* Accumulate constraint bounds based on bounds on individual variables.
      This is also the reentry point for multiple presolve iterations. */
Redo:
    if(psdata->forceupdate && !presolve_updatesums(lp, psdata))
      goto Complete;
    else
      psdata->forceupdate = FALSE;
    loops++;
    nn = 0;

   /* Report status of OF and constraint bounds */
#ifdef Paranoia
    Value = my_chsign(is_chsign(lp, 0), lp->presolve_undo->fixed_rhs[0]);
    test  = presolve_sumplumin(lp, 0, psdata->plulower,psdata->neglower)+Value;
    Value = presolve_sumplumin(lp, 0, psdata->pluupper,psdata->negupper)+Value;
    report(lp, NORMAL, "presolve: OF range at loop %d is " MPSVALUEMASK " to " MPSVALUEMASK "\n",
                        loops, test, Value);
    if(lp->spx_trace)
    for(i = firstActiveLink(psdata->rowmap); i > 0; i = nextActiveLink(psdata->rowmap, i)) {
      Value = my_chsign(is_chsign(lp, i), lp->presolve_undo->fixed_rhs[i]);
      test  = presolve_sumplumin(lp, i, psdata->plulower,psdata->neglower)+Value;
      Value = presolve_sumplumin(lp, i, psdata->pluupper,psdata->negupper)+Value;
      report(lp, NORMAL, "presolve: %2d range at loop %d is " MPSVALUEMASK " to " MPSVALUEMASK "\n",
                          i, loops, test, Value);
    }
#endif

   /* Eliminate empty or fixed columns (including trivial OF column singletons) */
    if(is_presolve(lp, PRESOLVE_COLS) && mat_validate(mat)) {
      if(userabort(lp, -1))
        goto Complete;
      n = 0;
      for(j = firstActiveLink(psdata->colmap); j != 0; ) {
        /* Don't presolve members of SOS'es */
        if(SOS_is_member(lp->SOS, 0, j)) {
          j = nextActiveLink(psdata->colmap, j);
          continue;
        }
        candelete = FALSE;
        ix = lp->rows + j;
        iix = presolve_collength(mat, j, psdata->rowmap, psdata->collength);
        Value = get_lowbo(lp, j);
        if(iix == 0) {
          if(Value != 0)
            report(lp, DETAILED, "presolve: Found empty non-zero variable %s\n",
                                  get_col_name(lp,j));
          candelete = TRUE;
        }
        else if(isOrigFixed(lp, ix)) {
          report(lp, DETAILED, "presolve: Eliminated variable %s fixed at %g\n",
                                get_col_name(lp,j), Value);
          candelete = TRUE;
        }
        else if((iix == 1) && (COL_MAT_ROWNR(mat->col_end[j-1]) == 0)) {

          if(get_OF_raw(lp, ix) <= 0)
            Value = get_upbo(lp, j);
          if(is_infinite(lp, Value)) {
            report(lp, DETAILED, "presolve: Unbounded variable %s\n",
                                  get_col_name(lp,j));
            status = UNBOUNDED;
          }
          else {
            /* Fix the value at its best bound */
            report(lp, DETAILED, "presolve: Eliminated trivial variable %s fixed at %g\n",
                                  get_col_name(lp,j), Value);
            candelete = TRUE;
          }
        }
        /* Look for opportunity to fix column based on the dual */
        else if(presolve_coldualfix(lp, j, psdata->rowmap, &Value)) {
          if(is_infinite(lp, Value)) {
            report(lp, DETAILED, "presolve: Unbounded variable %s\n",
                                  get_col_name(lp,j));
            status = UNBOUNDED;
          }
          else {
            /* Fix the value at its best bound */
            report(lp, DETAILED, "presolve: Eliminated dual-zero variable %s fixed at %g\n",
                                  get_col_name(lp,j), Value);
            candelete = TRUE;
          }
        }

        if(candelete) {
          if(!presolve_colfix(lp, psdata, j, Value, TRUE, &nv)) {
            report(lp, NORMAL, "presolve: Found variable bound infeasibility for column %d\n", ix);
            status = INFEASIBLE;
            nn = 0;
            break;
          }
          j = removeLink(psdata->colmap, j);
          n++;
          nn++;
        }
        else
          j = nextActiveLink(psdata->colmap, j);
      }
      if((status == RUNNING) && psdata->forceupdate &&
         !presolve_updatesums(lp, psdata))
        goto Complete;

      psdata->forceupdate = FALSE;
    }

   /* Eliminate linearly dependent rows; loop backwards over every row */
    if(is_presolve(lp, PRESOLVE_LINDEP) && (loops <= MAX_PSLINDEPLOOPS) && mat_validate(mat)) {
      int firstix, RT1, RT2;
      if(userabort(lp, -1))
        goto Complete;
      n = 0;
      for(i = lastActiveLink(psdata->rowmap); (i > 0) && (status == RUNNING); ) {

        /* First scan for rows with identical row lengths */
        ix = prevActiveLink(psdata->rowmap, i);
        if(ix == 0)
          break;

        /* Don't bother about empty rows or row singletons since they are
           handled by PRESOLVE_ROWS */
        j = presolve_rowlength(mat, i, psdata->colmap, psdata->rowlength);
        if(j <= 1) {
          i = ix;
          continue;
        }

#if 0
        /* Enable this to scan all rows back */
        RT2 = lp->rows;
#else
        RT2 = 2;
#endif
        firstix = ix;
        for(RT1 = 0; (ix > 0) && (RT1 < RT2) && (status == RUNNING);
            ix = prevActiveLink(psdata->rowmap, ix), RT1++)  {
          candelete = FALSE;
          if(presolve_rowlength(mat, ix, psdata->colmap, psdata->rowlength) != j)
            continue;

          /* Check if the beginning columns are identical; if not, continue */
          iix = presolve_nextcol(mat, ix, 0, psdata->colmap);
          jjx = presolve_nextcol(mat, i,  0, psdata->colmap);

          if(ROW_MAT_COLNR(iix) != ROW_MAT_COLNR(jjx))
            continue;

          /* We have a candidate row; check if the entries have a fixed non-zero ratio */
          test  = get_mat_byindex(lp, iix, TRUE, FALSE);
          Value = get_mat_byindex(lp, jjx, TRUE, FALSE);
          bound = test / Value;
          Value = bound;

          /* Loop over remaining entries */
          jx  = mat->row_end[i];
          jjx = presolve_nextcol(mat, i, ROW_MAT_COLNR(jjx), psdata->colmap);
          for(; (jjx < jx) && (Value == bound);
              jjx = presolve_nextcol(mat, i, ROW_MAT_COLNR(jjx), psdata->colmap)) {
            iix = presolve_nextcol(mat, ix, ROW_MAT_COLNR(iix), psdata->colmap);
            if(ROW_MAT_COLNR(iix) != ROW_MAT_COLNR(jjx))
              break;
            test  = get_mat_byindex(lp, iix, TRUE, FALSE);
            Value = get_mat_byindex(lp, jjx, TRUE, FALSE);

            /* If the ratio is different from the reference value we have a mismatch */
            Value = test / Value;
            if(bound == lp->infinite)
              bound = Value;
            else if(fabs(Value - bound) > epsvalue)
              break;
          }

          /* Check if we found a match (we traversed all active columns without a break) */
          if(jjx >= jx) {

            /* Get main reference values */
            test  = lp->orig_rhs[ix];
            Value = lp->orig_rhs[i] * bound;

            /* First check for inconsistent equalities */
            if((fabs(test - Value) > epsvalue) &&
               ((get_constr_type(lp, ix) == EQ) && (get_constr_type(lp, i) == EQ))) {
              status = INFEASIBLE;
            }

            else {

              /* Update lower and upper bounds */
              if(is_chsign(lp, i) != is_chsign(lp, ix))
                bound = -bound;

              test = get_rh_lower(lp, i);
              if(test <= -lp->infinite)
                test *= my_sign(bound);
              else
                test *= bound;

              Value = get_rh_upper(lp, i);
              if(Value >= lp->infinite)
                Value *= my_sign(bound);
              else
                Value *= bound;

              if((bound < 0))
                swapREAL(&test, &Value);

              if(get_rh_lower(lp, ix) < test)
                set_rh_lower(lp, ix, test);
              if(get_rh_upper(lp, ix) > Value)
                set_rh_upper(lp, ix, Value);

              /* Check results */
              test  = get_rh_lower(lp, ix);
              Value = get_rh_upper(lp, ix);
              if(fabs(Value-test) < epsvalue)
                set_constr_type(lp, ix, EQ);
              else if(Value < test) {
                status = INFEASIBLE;
              }

              /* Verify if we can continue */
              candelete = (MYBOOL) (status == RUNNING);
              if(!candelete) {
                report(lp, IMPORTANT, "presolve: Range infeasibility found involving rows %d and %d\n",
                                      ix, i);
              }
            }
          }
          /* Perform i-row deletion if authorized */
          if(candelete) {
            presolve_rowupdate(lp, i, psdata->collength, TRUE);
            removeLink(psdata->rowmap, i);
            n++;
            nc++;
            break;
          }
        }
        i = firstix;
      }
    }

   /* Aggregate and tighten bounds using 2-element EQs */
    if(FALSE &&
       (lp->equalities > 0) && is_presolve(lp, PRESOLVE_AGGREGATE) && mat_validate(mat)) {
      if(userabort(lp, -1))
        goto Complete;
      n = 0;
      for(i = lastActiveLink(psdata->rowmap); (i > 0) && (status == RUNNING); ) {
        /* Find an equality constraint with 2 elements; the pivot row */
        if(!is_constr_type(lp, i, EQ) || (presolve_rowlength(mat, i, psdata->colmap, psdata->rowlength) != 2)) {
          i = prevActiveLink(psdata->rowmap, i);
          continue;
        }
        /* Get the column indeces of NZ-values of the pivot row */
        jx = mat->row_end[i-1];
        j =  mat->row_end[i];
        for(; jx < j; jx++)
          if(isActiveLink(psdata->colmap, ROW_MAT_COLNR(jx)))
            break;
        jjx = jx+1;
        for(; jjx < j; jjx++)
          if(isActiveLink(psdata->colmap, ROW_MAT_COLNR(jjx)))
            break;
        jx  = ROW_MAT_COLNR(jx);
        jjx = ROW_MAT_COLNR(jjx);
        if(SOS_is_member(lp->SOS, 0, jx) && SOS_is_member(lp->SOS, 0, jjx)) {
          i = prevActiveLink(psdata->rowmap, i);
          continue;
        }
        /* Determine which column we should eliminate (index in jx) :
           1) the longest column
           2) the variable not being a SOS member
           3) an integer variable  */
        if(presolve_collength(mat, jx, psdata->rowmap, psdata->collength) <
           presolve_collength(mat, jjx, psdata->rowmap, psdata->collength))
          swapINT(&jx, &jjx);
        if(SOS_is_member(lp->SOS, 0, jx))
          swapINT(&jx, &jjx);
        if(!is_int(lp, jx) && is_int(lp, jjx))
          swapINT(&jx, &jjx);
        /* Whatever the priority above, we must have bounds to work with;
           give priority to the variable with the smallest bound */
        test  = get_upbo(lp, jjx)-get_lowbo(lp, jjx);
        Value = get_upbo(lp, jx)-get_lowbo(lp, jx);
        if(test < Value)
          swapINT(&jx, &jjx);
        /* Try to set tighter bounds on the non-eliminated variable (jjx) */
        test  = get_mat(lp, i, jjx); /* Non-eliminated variable coefficient a */
        Value = get_mat(lp, i, jx);  /* Eliminated variable coefficient     b */
#if 1
        bound = get_lowbo(lp, jx);
        if((bound > -lp->infinite)) {
          bound = (get_rh(lp, i)-Value*bound) / test;
          if(bound < get_upbo(lp, jjx)-epsvalue)
            set_upbo(lp, jjx, presolve_round(lp, bound, FALSE));
        }
        bound = get_upbo(lp, jx);
        if((bound < lp->infinite)) {
          bound = (get_rh(lp, i)-Value*bound) / test;
          if(bound > get_lowbo(lp, jjx)+epsvalue)
            set_lowbo(lp, jjx, presolve_round(lp, bound, TRUE));
        }
        i = prevActiveLink(psdata->rowmap, i);
#else
        /* Loop over the non-zero rows of the column to be eliminated;
           substitute jx-variable by updating rhs and jjx coefficients */
        int iiix;
        ix = mat->col_end[jx-1];
        iiix = mat->col_end[jx];
        rownr = &COL_MAT_ROWNR(ix);
        value = &COL_MAT_VALUE(ix);
        for(; ix < iiix;
            ix++, rownr += matRowColStep, value += matValueStep) {
          REAL newvalue;
          iix = *rownr;
          if((iix == i) ||
             ((iix > 0) && !isActiveLink(psdata->rowmap, iix)))
            continue;
          /* Do the update */
          bound = unscaled_mat(lp, *value, iix, jx)/Value;
          bound = my_chsign(is_chsign(lp, iix), bound);
          newvalue = get_mat(lp, iix, jjx) - bound*test;
            set_mat(lp, iix, jjx, presolve_precision(lp, newvalue));
          newvalue = get_rh(lp, iix) - bound*get_rh(lp, i);
            set_rh(lp, iix, presolve_precision(lp, newvalue));
        }
        /* Delete the column */
        removeLink(colmap, jx);
        nc++;
        n++;
        /* Delete the row */
        ix = i;
        i = prevActiveLink(psdata->rowmap, i);
        presolve_rowupdate(lp, ix, psdata->collength, TRUE);
        removeLink(psdata->rowmap, ix);
        nr++;
        n++;
        mat_validate(mat);
#endif
      }
    }

#if 0
   /* Increase A matrix sparsity by discovering common subsets using 2-element EQs */
    if((lp->equalities > 0) && is_presolve(lp, PRESOLVE_SPARSER) && mat_validate(mat)) {
      int iiix;
      if(userabort(lp, -1))
        goto Complete;
      n = 0;
      for(i = lastActiveLink(psdata->rowmap); (i > 0) && (status == RUNNING); ) {
        candelete = FALSE;
        /* Find an equality constraint with 2 elements; the pivot row */
        if(!is_constr_type(lp, i, EQ) || (mat_rowlength(mat, i) != 2)) {
          i = prevActiveLink(psdata->rowmap, i);
          continue;
        }
        /* Get the column indeces of NZ-values of the pivot row */
        jx = mat->row_end[i-1];
        jx = ROW_MAT_COLNR(jx);
        jjx = mat->row_end[i];
        jjx = ROW_MAT_COLNR(jjx);
        /* Scan to find a row with matching column entries */
        ix = lp->col_end[jx-1];
        iiix = lp->col_end[jx];
        rownr = &COL_MAT_ROWNR(ix);
        for(; ix < iiix;
            ix++, rownr += matRowColStep) {
          if(*rownr == i)
            continue;
          /* We now have a single matching value, find the next */
          iix = lp->col_end[jjx-1];
          for(; iix < lp->col_end[jjx]; iix++)
            if(COL_MAT_ROWNR(iix) >= ix)
              break;
          /* Abort this row if there was no second column match */
          if((iix >= lp->col_end[jjx]) || (COL_MAT_ROWNR(iix) > ix) )
            break;
          /* Otherwise, do variable subsitution and mark pivot row for deletion */
          candelete = TRUE;
          nc++;
          /*
           ... Add remaining logic later!
          */
        }
        ix = i;
        i = prevActiveLink(psdata->rowmap, i);
        if(candelete) {
          presolve_rowupdate(lp, ix, psdata->collength, TRUE);
          removeLink(psdata->rowmap, ix);
          n++;
        }
      }
    }
#endif

   /* Eliminate empty rows, convert row singletons to bounds,
      tighten bounds, and remove always satisfied rows */
    if(is_presolve(lp, PRESOLVE_ROWS) && mat_validate(mat)) {
      if(userabort(lp, -1))
        goto Complete;
      n = 0;
      for(i = lastActiveLink(psdata->rowmap); i > 0; ) {

        candelete = FALSE;

       /* First identify any full row infeasibilities
          Note: Handle singletons below to ensure that conflicting multiple singleton
                rows with this variable do not provoke notice of infeasibility */
        if((j > 1) &&
           !psdata->forceupdate && !presolve_rowfeasible(lp, i, psdata,
#if 1
                                 FALSE)
#else
                                 TRUE)  /* Extended debug mode */
#endif
          ) {
          status = INFEASIBLE;
          break;
        }

        j = psdata->plucount[i]+psdata->negcount[i];

       /* Delete non-zero rows and variables that are completely determined;
          note that this step can provoke infeasibility in some tight models */
        if((j > 0)                                       /* Only examine non-empty rows, */
           && (fabs(lp->orig_rhs[i]) < epsvalue)         /* .. and the current RHS is zero, */
           && ((psdata->plucount[i] == 0) ||
               (psdata->negcount[i] == 0))               /* .. and the parameter signs are all equal, */
           && (psdata->pluneg[i] == 0)                   /* .. and no (quasi) free variables, */
           && (is_constr_type(lp, i, EQ)
#ifdef FindImpliedEqualities
               || (fabs(get_rh_lower(lp, i)-
                   presolve_sumplumin(lp, i, psdata->pluupper,psdata->negupper)) < epsvalue)  /* Convert to equalities */
               || (fabs(get_rh_upper(lp, i)-
                   presolve_sumplumin(lp, i, psdata->plulower,psdata->neglower)) < epsvalue)  /* Convert to equalities */
#endif
              )
              ) {

          /* Delete the columns of this row, but make sure we don't delete SOS variables */
          for(ix = mat->row_end[i]-1; ix >= mat->row_end[i-1]; ix--) {
            jx = ROW_MAT_COLNR(ix);
            if(isActiveLink(psdata->colmap, jx) && !SOS_is_member(lp->SOS, 0, jx)) {
              if(!presolve_colfix(lp, psdata, jx, 0.0, TRUE, &nv)) {
                report(lp, NORMAL, "presolve: Found row variable bound infeasibility for column %d\n", jx);
                status = INFEASIBLE;
                nn = 0;
                break;
              }
              removeLink(psdata->colmap, jx);
            }
          }
          /* Then delete the row, which is redundant */
          if(status == RUNNING) {
            candelete = TRUE;
            nc++;
          }
        }
        else

       /* Then delete any empty or always satisfied / redundant row that cannot at
          the same time guarantee that we can also delete associated variables */
        if((j == 0) ||                                     /* Always delete an empty row */
           ((j > 1) &&
            (psdata->pluneg[i] == 0) && ((psdata->plucount[i] == 0) ||
                                         (psdata->negcount[i] == 0)) &&    /* Consider removing if block above is ON! */
            (presolve_sumplumin(lp, i, psdata->pluupper,psdata->negupper)- /* .. or if it is always satisfied (redundant) */
             presolve_sumplumin(lp, i, psdata->plulower,psdata->neglower) < epsvalue))
          ) {
          candelete = TRUE;
          nc++;
        }

       /* Convert row singletons to bounds (delete fixed columns in columns section) */
        else if((j == 1) &&
                (presolve_sumplumin(lp, i, psdata->pluupper,psdata->negupper)-
                 presolve_sumplumin(lp, i, psdata->plulower,psdata->neglower) >= epsvalue)) {
          j = presolve_nextcol(mat, i, 0, psdata->colmap);
          j = ROW_MAT_COLNR(j);

          /* Make sure we do not have conflicting other singleton rows with this variable */
          Value = lp->infinite;
          test = -Value;
          if(presolve_collength(mat, j, psdata->rowmap, psdata->collength) > 1) {
            test  = get_rh_lower(lp, i);
            Value = get_rh_upper(lp, i);
            if(presolve_singletonbounds(lp, i, j, &test, &Value, NULL)) {
              jx = mat->col_end[j];
              for(ix = presolve_nextrow(mat, j, 0, psdata->rowmap);
                  ix < jx; ix = presolve_nextrow(mat, j, iix, psdata->rowmap)) {
                iix = COL_MAT_ROWNR(ix);
                if((iix != i) &&
                   (presolve_rowlength(mat, iix, psdata->colmap, psdata->rowlength) == 1) &&
                   !presolve_altsingletonvalid(lp, iix, j, test, Value)) {
                  status = INFEASIBLE;
                  nn = 0;
                  break;
                }
              }
            }
            else {
              status = INFEASIBLE;
            }
          }

          /* Proceed to fix and remove variable (if it is not a SOS member) */
          if(status == RUNNING) {
            if((fabs(test-Value) < epsvalue) && (fabs(test) > epsvalue)) {
              if(!presolve_colfix(lp, psdata, j, test, (MYBOOL) !SOS_is_member(lp->SOS, 0, j), NULL))
                status = INFEASIBLE;
              else if(SOS_is_member(lp->SOS, 0, j))
                nt++;
              else {
                removeLink(psdata->colmap, j);
                nv++;
              }
            }
            else
              status = presolve_colsingleton(lp, psdata, i, j, &nt);
          }
          if(status == INFEASIBLE) {
            report(lp, NORMAL, "presolve: Found singleton bound infeasibility for column %d\n", j);
            nn = 0;
            break;
          }
          if(psdata->forceupdate != AUTOMATIC) {
            candelete = TRUE;
            nb++;
          }
        }

       /* Check if we have a constraint made redundant through bounds on individual variables */
        else if((presolve_sumplumin(lp, i, psdata->plulower,psdata->neglower) >= get_rh_lower(lp, i)-epsvalue) &&
                (presolve_sumplumin(lp, i, psdata->pluupper,psdata->negupper) <= get_rh_upper(lp, i)+epsvalue)) {
          candelete = TRUE;
          nc++;
        }

#if defined AggressiveRowPresolve
       /* Look for opportunity to tighten constraint bounds;
          known to create problems with scaled ADLittle.mps */
        else if(j > 1) {
          test = presolve_sumplumin(lp, i, psdata->plulower,psdata->neglower);
          if(test > get_rh_lower(lp, i)+epsvalue) {
            set_rh_lower(lp, i, presolve_round(lp, test, TRUE));
            nr++;
          }
          test = presolve_sumplumin(lp, i, psdata->pluupper,psdata->negupper);
          if(test < get_rh_upper(lp, i)-epsvalue) {
            set_rh_upper(lp, i, presolve_round(lp, test, FALSE));
            nr++;
          }
        }
#elif defined UseFullConstraintInfo
        else if((MIP_count(lp) > 0) && (j > 1)) {
//        else if(j > 1) {
          status = presolve_rowtighten(lp, psdata, i, &nt);
          if(status != RUNNING)
            nn = 0;
        }
#endif

        /* Get next row and do the deletion of the previous, if indicated */
        ix = i;
        i = prevActiveLink(psdata->rowmap, i);
        if(candelete) {
          presolve_rowupdate(lp, ix, psdata->collength, TRUE);
          removeLink(psdata->rowmap, ix);
          n++;
          nn++;
        }
        /* Look for opportunity to convert ranged constraint to equality-type */
        else if(!is_constr_type(lp, ix, EQ) && (get_rh_range(lp, ix) < epsvalue))
          set_constr_type(lp, ix, EQ);
      }
    }

   /* Try again if we were successful in this presolve loop */
    if((status == RUNNING) && !userabort(lp, -1)) {
      if((nn > 0) && (loops <= get_presolveloops(lp)) &&
         (psdata->rowmap->count+psdata->colmap->count > 0))
        goto Redo;
    }

Complete:
   /* See if we can convert some constraints to SOSes (only SOS1 handled) */
    if(is_presolve(lp, PRESOLVE_SOS) &&
       (MIP_count(lp) > 0) && mat_validate(mat)) {
      n = 0;
      for(i = lastActiveLink(psdata->rowmap); i > 0; ) {
        candelete = FALSE;
        test = get_rh(lp, i);
        jx = get_constr_type(lp, i);
#ifdef EnableBranchingOnGUB
        if((test == 1) && (jx != GE)) {
#else
        if((test == 1) && (jx == LE)) {
#endif
          jjx = mat->row_end[i-1];
          iix = mat->row_end[i];
          for(; jjx < iix; jjx++) {
            j = ROW_MAT_COLNR(jjx);
            if(!isActiveLink(psdata->colmap, j))
              continue;
            if(!is_binary(lp, j) || (get_mat(lp, i, j) != 1))
              break;
          }
          if(jjx >= iix) {
            char SOSname[16];

            /* Define a new SOS instance */
            sprintf(SOSname, "SOS_%d", SOS_count(lp) + 1);
            ix = add_SOS(lp, SOSname, 1, 1, 0, NULL, NULL);
            if(jx == EQ)
              SOS_set_GUB(lp->SOS, ix, TRUE);
            Value = 0;
            jjx = mat->row_end[i-1];
            for(; jjx < iix; jjx++) {
              j = ROW_MAT_COLNR(jjx);
              if(!isActiveLink(psdata->colmap, j))
                continue;
              Value += 1;
              append_SOSrec(lp->SOS->sos_list[ix-1], 1, &j, &Value);
            }
            candelete = TRUE;
            nc++;
          }
        }

        /* Get next row and do the deletion of the previous, if indicated */
        ix = i;
        i = prevActiveLink(psdata->rowmap, i);
        if(candelete) {
          presolve_rowupdate(lp, ix, psdata->collength, TRUE);
          removeLink(psdata->rowmap, ix);
          n++;
          nn++;
        }
      }
      if(n)
        report(lp, NORMAL, "presolve: Converted %5d constraints to SOS1.\n", n);
    }

   /* Finalize presolve */
#ifdef Paranoia
    i = presolve_validate(lp, psdata->rowmap, psdata->colmap);
    if(i > 0)
      report(lp, SEVERE, "presolve: %d internal consistency failure(s) detected\n", i);
#endif
    if(!presolve_finalize(lp, psdata))
      report(lp, SEVERE, "presolve: Unable to construct internal data representation\n");

   /* Report on bounds Tighten MIP bound if possible (should ideally use some kind of smart heuristic) */
#if 1
    Value = my_chsign(is_chsign(lp, 0), lp->presolve_undo->fixed_rhs[0]-initrhs0);
    test  = presolve_sumplumin(lp, 0, psdata->plulower,psdata->neglower)+Value;
    Value = presolve_sumplumin(lp, 0, psdata->pluupper,psdata->negupper)+Value;
    report(lp, NORMAL, "presolve: OF range identified   " MPSVALUEMASK " to " MPSVALUEMASK "\n",
                        test, Value);
#endif
#if 1
    if((MIP_count(lp) > 0) || (get_Lrows(lp) > 0)) {
      if(is_maxim(lp)) {
        lp->bb_heuristicOF = MAX(lp->bb_heuristicOF, test);
        lp->bb_limitOF     = MIN(lp->bb_limitOF, Value);
      }
      else {
        lp->bb_heuristicOF = MIN(lp->bb_heuristicOF, Value);
        lp->bb_limitOF     = MAX(lp->bb_limitOF, test);
      }
    }
#endif

   /* Report summary information */
#ifdef Paranoia
    i = NORMAL;
#else
    i = DETAILED;
#endif
    j = nc+nb+nt+nv+nr;
    if(j > 0)
      report(lp, i, "presolve:   %8d presolve loops were performed\n", loops);
    if(nv)
      report(lp, i, "            %8d empty or fixed variables.........  %s.\n", nv, "REMOVED");
    if(nb)
      report(lp, i, "            %8d row singletons to variable bounds  %s.\n", nb, "CONVERTED");
    if(nt)
      report(lp, i, "            %8d other variable bounds............  %s.\n", nt, "TIGHTENED");
    if(nc)
      report(lp, i, "            %8d empty or redundant constraints...  %s.\n", nc, "REMOVED");
    if(nr)
      report(lp, i, "            %8d constraint bounds................  %s.\n", nr, "TIGHTENED");

    if(j > 0)
      report(lp, NORMAL, " \n");

    /* Report optimality or infeasibility */
    Value = my_chsign(is_chsign(lp, 0), lp->presolve_undo->fixed_rhs[0]);
    test  = presolve_sumplumin(lp, 0, psdata->plulower,psdata->neglower)+Value;
    Value = presolve_sumplumin(lp, 0, psdata->pluupper,psdata->negupper)+Value;
    if(fabs(test-Value) < epsvalue) {
      report(lp, NORMAL, "presolve: Identified optimal OF value of %g\n",
                         Value);
#if 0
      status = OPTIMAL;
#endif
    }
    else if(status != RUNNING)
      report(lp, NORMAL, "presolve: Infeasibility or unboundedness detected.\n");

    /* Clean up */
    FREE(psdata->rowlength);
    FREE(psdata->collength);
    FREE(psdata->plucount);
    FREE(psdata->negcount);
    FREE(psdata->pluneg);
    FREE(psdata->plulower);
    FREE(psdata->neglower);
    FREE(psdata->pluupper);
    FREE(psdata->negupper);
    FREE(psdata);

  }

  /* Signal that we are done presolving */
  if((lp->usermessage != NULL) &&
     ((lp->do_presolve & PRESOLVE_LASTMASKMODE) != 0) && (lp->msgmask & MSG_PRESOLVE))
     lp->usermessage(lp, lp->msghandle, MSG_PRESOLVE);

  /* Clean out empty SOS records */
  if(SOS_count(lp) > 0) {
    clean_SOSgroup(lp->SOS);
    if(lp->SOS->sos_count == 0)
      free_SOSgroup(&(lp->SOS));
  }

  /* Create master SOS variable list */
  if(SOS_count(lp) > 0)
    make_SOSchain(lp, (MYBOOL) ((lp->do_presolve & PRESOLVE_LASTMASKMODE) != PRESOLVE_NONE));

  /* Resolve GUBs */
#ifdef EnableBranchingOnGUB
  if(is_bb_mode(lp, NODE_GUBMODE))
    identify_GUB(lp, TRUE);
#endif

  /* Crash the basis, if specified */
  crash_basis(lp);

  /* Scale the model based on current settings */
  if(status == RUNNING)
    auto_scale(lp);

  /* Produce presolved model statistics */
  if(nc+nb+nt+nv+nr > 0)
    REPORT_modelinfo(lp, FALSE, "PRESOLVED");

Finish:
  lp->wasPresolved  = TRUE;
  /* lp->timepresolved = timeNow(); */

#if 0
  write_lp(lp, "test_out.lp");   /* Must put here due to variable name mapping */
#endif
#if 0
  REPORT_debugdump(lp, "testint2.txt", FALSE);
#endif

  return( status );

}

STATIC MYBOOL postsolve(lprec *lp, int status)
{
  /* Verify solution */
  if(lp->lag_status != RUNNING) {
    int itemp;

    if((status == OPTIMAL) || (status == SUBOPTIMAL)) {
      itemp = check_solution(lp, lp->columns, lp->best_solution,
                                 lp->orig_upbo, lp->orig_lowbo, DEF_EPSSOLUTION);
      if((itemp != OPTIMAL) && (lp->spx_status == OPTIMAL))
          lp->spx_status = itemp;
      else if((itemp == OPTIMAL) && (status == SUBOPTIMAL))
        lp->spx_status = status;
    }
    else {
      report(lp, NORMAL, "lp_solve unsuccessful after %d iterations and a last best value of %g\n",
             lp->total_iter, lp->best_solution[0]);
      if(lp->bb_totalnodes > 0)
        report(lp, NORMAL, "lp_solve explored %d nodes before termination\n",
               lp->bb_totalnodes);
    }
  }

  /* Check if we can clear the variable map */
  if(varmap_canunlock(lp))
    lp->varmap_locked = FALSE;

  return( TRUE );
}
