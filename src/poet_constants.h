#ifndef _POET_CONSTANTS_H
#define _POET_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "poet_math.h"

static const real_t R_ZERO             =   CONST(0.0);
static const real_t R_ONE              =   CONST(1.0);
static const real_t R_TWO              =   CONST(2.0);

// estimate_base_workload constants
static const real_t X_HAT_MINUS_START  =   CONST(0.0);
static const real_t X_HAT_START        =   CONST(0.2);
static const real_t Q                  =   CONST(0.00001);
static const real_t P_START            =   CONST(1.0);
static const real_t P_MINUS_START      =   CONST(0.0);
static const real_t H_START            =   CONST(0.0);
static const real_t R                  =   CONST(0.01);
static const real_t K_START            =   CONST(0.0);

// calculate_xup constants
#define FAST 1
#define SLOW 0

#if FAST
static const real_t P1                 =   CONST(0.0);
static const real_t P2                 =   CONST(0.0);
static const real_t Z1                 =   CONST(0.0);
#elif SLOW
static const real_t P1                 =   CONST(0.1);
static const real_t P2                 =   CONST(0.8);
static const real_t Z1                 =   CONST(0.7);
#else
static const real_t P1                 =  -CONST(0.5);
static const real_t P2                 =   CONST(0.0);
static const real_t Z1                 =   CONST(0.0);
#endif

static const real_t MU                 =   CONST(1.0);
static const real_t E_START            =   CONST(1.0);
static const real_t EO_START           =   CONST(1.0);

// general constants
static const int CURRENT_ACTION_START  =  1;

#ifdef FIXED_POINT
static const real_t BIG_REAL_T         =   0x7FFFFFFF;
#else
static const real_t BIG_REAL_T         =   100000.0;
#endif

#ifdef __cplusplus
}
#endif

#endif
