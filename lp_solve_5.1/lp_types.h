#ifndef HEADER_lp_types
#define HEADER_lp_types

/* Define data types                                                         */
/* ------------------------------------------------------------------------- */
#ifndef REAL
  #define REAL  double
#endif

/*#define LREAL long double*/    /* Set global update variable as long double */
#define LREAL REAL               /* Set global update variable as standard */
#define RREAL long double        /* Set local accumulation variable as long double */
/*#define RREAL REAL*/           /* Set local accumulation standard variable */

#define RESULTVALUEMASK "%18g"   /* Set fixed-format real-valued output precision;
                                    suggested width: ABS(exponent of DEF_EPSVALUE)+5. */
#define INDEXVALUEMASK  "%8d"    /* Set fixed-format integer-valued output width */

#ifndef DEF_STRBUFSIZE
  #define DEF_STRBUFSIZE   512
#endif
#ifndef MAXINT32
  #define MAXINT32  2147483647
#endif
#ifndef MAXUINT32
  #define MAXUINT32 4294967295
#endif
#ifndef MAXINT64
  #if defined _LONGLONG || defined __LONG_LONG_MAX__ || defined LLONG_MAX
    #define MAXINT64   9223372036854775807ll
  #else
    #define MAXINT64   9223372036854775807l
  #endif
#endif
#ifndef MAXUINT64
  #if defined _LONGLONG || defined __LONG_LONG_MAX__ || defined LLONG_MAX
    #define MAXUINT64 18446744073709551616ll
  #else
    #define MAXUINT64 18446744073709551616l
  #endif
#endif
#ifndef CHAR_BIT
  #define CHAR_BIT  8
#endif
#ifndef MYBOOL
/*  #define MYBOOL  unsigned int */          /* May be faster on some processors */
  #define MYBOOL  unsigned char                         /* Conserves memory */
#endif


/* Constants                                                                 */
/* ------------------------------------------------------------------------- */
#ifndef NULL
  #define NULL                   0
#endif

/* Byte-sized Booleans and extended options */
#define FALSE                    0
#define TRUE                     1
#define AUTOMATIC                2
#define DYNAMIC                  4

/* Library load status values */
#define LIB_LOADED               0
#define LIB_NOTFOUND             1
#define LIB_NOINFO               2
#define LIB_NOFUNCTION           3
#define LIB_VERINVALID           4
#define LIB_STR_LOADED           "Successfully loaded"
#define LIB_STR_NOTFOUND         "File not found"
#define LIB_STR_NOINFO           "No version data"
#define LIB_STR_NOFUNCTION       "Missing function header"
#define LIB_STR_VERINVALID       "Incompatible version"
#define LIB_STR_MAXLEN           23

/* Result ranges */
#define XRESULT_FREE             0
#define XRESULT_PLUS             1
#define XRESULT_MINUS           -1
#define XRESULT_RC               XRESULT_FREE


/* Compiler/target settings                                                  */
/* ------------------------------------------------------------------------- */
#ifdef WIN32
# define __WINAPI WINAPI
#else
# define __WINAPI
#endif

#ifndef __BORLANDC__

  #ifdef _USRDLL

    #if 1
      #define __EXPORT_TYPE __declspec(dllexport)
    #else
     /* Set up for the Microsoft compiler */
      #ifdef LP_SOLVE_EXPORTS
        #define __EXPORT_TYPE __declspec(dllexport)
      #else
        #define __EXPORT_TYPE __declspec(dllimport)
      #endif
    #endif

  #else

    #define __EXPORT_TYPE

  #endif


  #ifdef __cplusplus
    #define __EXTERN_C extern "C"
  #else
    #define __EXTERN_C
  #endif

#else  /* Otherwise set up for the Borland compiler */

  #ifdef __DLL__

    #define _USRDLL
    #define __EXTERN_C extern "C"

    #ifdef __READING_THE_DLL
      #define __EXPORT_TYPE __import
    #else
      #define __EXPORT_TYPE __export
    #endif

  #else

    #define __EXPORT_TYPE
    #define __EXTERN_C extern "C"

  #endif

#endif


#if 0
  #define STATIC static
#else
  #define STATIC
#endif

#if !defined INLINE
  #if defined __cplusplus
    #define INLINE inline
  #elif defined _WIN32 || defined WIN32
    #define INLINE _inline
  #else
    #define INLINE
  #endif
#endif


#ifdef __cplusplus
  #define __EXTERN_C extern "C"
#else
  #define __EXTERN_C
#endif


/* Define macros                                                             */
/* ------------------------------------------------------------------------- */
#define my_min(x, y)            ((x) < (y) ? (x) : (y))
#define my_max(x, y)            ((x) > (y) ? (x) : (y))
#define my_range(x, lo, hi)     ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#ifndef my_mod
  #define my_mod(n, m)          ((n) % (m))
#endif
#define my_if(t, x, y)          ((t) ? (x) : (y))
#define my_sign(x)              ((x) < 0 ? -1 : 1)
#define my_chsign(t, x)         ( ((t) && ((x) != 0)) ? -(x) : (x))
#define my_flipsign(x)          ( fabs((REAL) (x)) == 0 ? 0 : -(x) )
#define my_roundzero(val, eps)  if (fabs((REAL) (val)) < eps) val = 0
#define my_avoidtiny(val, eps)  (fabs((REAL) (val)) < eps ? 0 : val)

#define my_inflimit(val)        (fabs((REAL) (val)) < lp->infinite ? (val) : lp->infinite * my_sign(val) )
#if 0
  #define my_precision(val, eps) ((fabs((REAL) (val))) < (eps) ? 0 : (val))
#else
  #define my_precision(val, eps) restoreINT(val, eps)
#endif
#define my_reldiff(x, y)       (((x) - (y)) / (1.0 + fabs((REAL) (y))))
#define my_boundstr(x)         (fabs(x) < lp->infinite ? sprintf("%g",x) : ((x) < 0 ? "-Inf" : "Inf") )
#ifndef my_boolstr
  #define my_boolstr(x)          (!(x) ? "FALSE" : "TRUE")
#endif
#define my_basisstr(x)         ((x) ? "BASIC" : "NON-BASIC")
#define my_lowbound(x)         ((FULLYBOUNDEDSIMPLEX) ? (x) : 0)


/* Forward declarations                                                      */
/* ------------------------------------------------------------------------- */
typedef struct _lprec    lprec;
typedef struct _INVrec   INVrec;
/*typedef struct _pricerec pricerec;*/
typedef struct _pricerec
{
  REAL    theta;
  REAL    pivot;
  REAL    epspivot;
  int     varno;
  lprec   *lp;
  MYBOOL  isdual;
} pricerec;


#endif /* HEADER_lp_types */
