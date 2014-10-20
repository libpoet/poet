#ifndef _POET_MATH_H
#define _POET_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

/*
 * Define Q16 fixed point type
 * Define addition, subtraction, multiplication, and division operations
 * for type
 */

#define MAX_FP 0x7FFFFFFF
#define MIN_FP 0x80000000

#define UNDERFLOW_MARGIN .01

typedef int32_t fp_t;

#define FP_CONST(x) ((fp_t) (((x) >= 0) ? ((x) * 65536.0 + 0.5) : ((x * 65536.0 - 0.5))))
#define DB_CONST(x) (x)

static inline fp_t FP_ADD2(fp_t a, fp_t b) {
  fp_t sum = a + b;

#ifdef POET_MATH_OVERFLOW
  // Check for overflow
  // Can only happen if a and b have the same sign,
  // and the sign of the sum is not equal to that sign
	if (!((a ^ b) & 0x80000000) && ((a ^ sum) & 0x80000000)) {
    if (a > 0) {
      sum = (fp_t) MAX_FP;
    } else {
      sum = (fp_t) MIN_FP;
    }
  }
#endif

#ifdef OVERFLOW_WARNING
  double a1 = (double) (a / ((double) (1 << 16)));
  double b1 = (double) (b / ((double) (1 << 16)));
  double answer1 = (double) (sum / ((double) (1 << 16)));

  double temp = answer1 - (a1+b1);
  if (!(temp < 1 && temp > -1)) {
    printf("\nFixed point overflow...attempted to add %f and %f\n", a1, b1);
    printf("Expected %f but received %f\n", a1+b1, answer1);
  }
#endif

  return sum;
}

static inline fp_t FP_SUB(fp_t a, fp_t b) {
  fp_t difference = a - b;

#ifdef POET_MATH_OVERFLOW
  // Check for overflow
  // Can only happen if a and b have different signs,
  // and the sign of the difference is not equal to that sign
	if (((a ^ b) & 0x80000000) && ((a ^ difference) & 0x80000000)) {
    if (a > 0) {
      difference = (fp_t) MAX_FP;
    } else {
      difference = (fp_t) MIN_FP;
    }
  }
#endif

#ifdef OVERFLOW_WARNING
  double a1 = (double) (a / ((double) (1 << 16)));
  double b1 = (double) (b / ((double) (1 << 16)));
  double answer1 = (double) (difference / ((double) (1 << 16)));

  double temp = answer1 - (a1-b1);
  if (!(temp < 1 && temp > -1)) {
    printf("\nFixed point overflow...attempted to subtract %f and %f\n", a1, b1);
    printf("Expected %f but received %f\n", a1-b1, answer1);
  }
#endif

  return difference;
}


static inline fp_t FP_MULT2(fp_t a, fp_t b) {
  int64_t prod = (int64_t) a * b;
  fp_t answer = prod >> 16;

#ifdef POET_MATH_OVERFLOW
  // Check for overflow
  // Upper 17 bits should all be the same
  // If they are not, return the maximum f
  uint32_t upper = (prod >> 47);
  if ((prod < 0 && ~upper) || (prod > 0 && upper)) {
    if ((a >= 0) == (b >= 0)) {
      answer = (fp_t) MAX_FP;
    } else {
      answer = (fp_t) MIN_FP;
    }
  }
#endif

#ifdef OVERFLOW_WARNING
  double a1 = (double) (a / ((double) (1 << 16)));
  double b1 = (double) (b / ((double) (1 << 16)));
  double answer1 = (double) (answer / ((double) (1 << 16)));

  double temp = answer1 - (a1*b1);
  if (!(temp < 1 && temp > -1)) {
    printf("\nFixed point overflow...attempted to multiply %f by %f\n", a1, b1);
    printf("Expected %f but received %f\n", a1*b1, answer1);
    return answer;
  }
#endif

#ifdef UNDERFLOW_WARNING
  if (a == 0 || b == 0) {
    return answer;
  }


  double a2 = (double) (a / ((double) (1 << 16)));
  double b2 = (double) (b / ((double) (1 << 16)));
  double answer2 = (double) (answer / ((double) (1 << 16)));

  double temp2 = 1 - answer2 / (a2*b2);
  if (!(temp2 < UNDERFLOW_MARGIN && temp > -UNDERFLOW_MARGIN)) {
    printf("\nFixed point underflow...attempted to multiply %f by %f\n", a2, b2);
    printf("Expected %f but received %f\n", a2*b2, answer2);
  }
#endif

  return answer;
}

static inline fp_t FP_DIV(fp_t a, fp_t b) {
  int64_t quotient = ((((int64_t) a) << 16) / b);
  fp_t answer = (fp_t) quotient;

#ifdef POET_MATH_OVERFLOW
  // Check for overflow
  // Upper 17 bits should all be the same
  // If they are not, return the maximum f
  uint32_t upper = (quotient >> 31);
  if ((answer < 0 && ~upper) || (answer > 0 && upper)) {
    if ((a >= 0) == (b >= 0)) {
      answer = (fp_t) MAX_FP;
    } else {
      answer = (fp_t) MIN_FP;
    }
  }
#endif

#ifdef OVERFLOW_WARNING
  double a1 = (double) (a / ((double) (1 << 16)));
  double b1 = (double) (b / ((double) (1 << 16)));
  double answer1 = (double) (answer / ((double) (1 << 16)));

  double temp = answer1 - (a1/b1);
  if (!(temp < 1 && temp > -1)) {
    printf("\nFixed point overflow...attempted to divide %f by %f\n", a1, b1);
    printf("Expected %f but received %f\n", a1/b1, answer1);
    return answer;
  }
#endif

#ifdef UNDERFLOW_WARNING
  if (a == 0) {
    return answer;
  }

  double a2 = (double) (a / ((double) (1 << 16)));
  double b2 = (double) (b / ((double) (1 << 16)));
  double answer2 = (double) (answer / ((double) (1 << 16)));

  double temp2 = 1 - answer2 / (a2/b2);
  if (!(temp2 < UNDERFLOW_MARGIN && temp > -UNDERFLOW_MARGIN)) {
    printf("\nFixed point underflow...attempted to divide %f by %f\n", a2, b2);
    printf("Expected %f but received %f\n", a2/b2, answer2);
  }
#endif

  return answer;
}

// For ease of use, multiply >2 fixed point values at once
#define FP_MULT3(a, b, c) (FP_MULT2(FP_MULT2((a), (b)), (c)))
#define FP_MULT4(a, b, c, d) (FP_MULT2(FP_MULT3((a), (b), (c)), (d)))

// Same operations for floating point numbers
#define DB_MULT2(a, b) ((a) * (b))
#define DB_MULT3(a, b, c) (DB_MULT2(DB_MULT2((a), (b)), (c)))
#define DB_MULT4(a, b, c, d) (DB_MULT2(DB_MULT3((a), (b), (c)), (d)))
#define DB_DIV(a, b) ((a) / (b))

/*
 * If fixed point is defined, use fixed point functions
 * Otherwise use normal floating point functions
 */

#ifdef FIXED_POINT

typedef fp_t real_t;

#define CONST(x) FP_CONST((x))
#define mult(a,b) (FP_MULT2((a), (b)))
#define mult3(a,b,c) (FP_MULT3((a), (b), (c)))
#define mult4(a,b,c,d) (FP_MULT4((a), (b), (c), (d)))
#define div(a,b) (FP_DIV((a),(b)))

#define int_to_real(a) ((real_t) (a) << 16)
#define real_to_db(a) ((double) ((a) / ((double) (1 << 16))))
#define real_to_int(a) (((a) + CONST(.5)) >> 16)

#else

typedef double real_t;

#define CONST(x) DB_CONST((x))
#define mult(a,b) (DB_MULT2((a), (b)))
#define mult3(a,b,c) (DB_MULT3((a), (b), (c)))
#define mult4(a,b,c,d) (DB_MULT4((a), (b), (c), (d)))
#define div(a,b) (DB_DIV((a),(b)))

#define int_to_real(a) ((double) (a))
#define real_to_db(a) (a)
#define real_to_int(a) ((a) + .5)

#endif

#ifdef __cplusplus
}
#endif

#endif
