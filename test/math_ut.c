#include <stdio.h>
#include <stdlib.h>
#include "poet_math.h"

#define ERROR_MARGIN .0005

typedef int bool;
#define true 1
#define false 0

double d_abs(double d) {
  return ((d > 0) ? d : -d);
}

void assert(bool expression, char * error_message) {
  if (!expression) {
    printf("%s", error_message);
    exit(-1);
  }
}

double fp_to_db(fp_t a) {
  return (double) (a / ((double) (1 << 16)));
}

void addition_test(fp_t a, fp_t b, double c, double d) {
  char error_message[256];
  double expected;
  fp_t ans;
  bool expression;

  ans = FP_ADD2(a, b);
  expected = c + d;

  expression = d_abs(fp_to_db(ans) - expected) < ERROR_MARGIN;

  sprintf(error_message, "\nExpected %f but calculated %f\n", expected, fp_to_db(ans));

  assert(expression, error_message);
}

void subtraction_test(fp_t a, fp_t b, double c, double d) {
  char error_message[256];
  double expected;
  fp_t ans;
  bool expression;

  ans = FP_SUB(a, b);
  expected = c - d;

  expression = d_abs(fp_to_db(ans) - expected) < ERROR_MARGIN;

  sprintf(error_message, "\nExpected %f but calculated %f\n", expected, fp_to_db(ans));

  assert(expression, error_message);
}

void multiplication_test(fp_t a, fp_t b, double c, double d) {
  char error_message[256];
  double expected;
  fp_t ans;
  bool expression;

  ans = FP_MULT2(a, b);
  expected = DB_MULT2(c, d);

  expression = d_abs(fp_to_db(ans) - expected) < ERROR_MARGIN;

  sprintf(error_message,
      "\nExpected %f but calculated %f with a=%f b=%f\n",
      expected, fp_to_db(ans), c, d);

  assert(expression, error_message);
}

void division_test(fp_t a, fp_t b, double c, double d) {
  char error_message[256];
  double expected;
  fp_t ans;
  bool expression;

  ans = FP_DIV(a, b);
  expected = DB_DIV(c, d);

  expression = d_abs(fp_to_db(ans) - expected) < ERROR_MARGIN;

  sprintf(error_message,
      "\nExpected %f but calculated %f with a=%f b=%f\n",
      expected, fp_to_db(ans), c, d);

  assert(expression, error_message);
}

void conversion_to_fp_test() {
  char error_message[256];
  bool expression;
  fp_t a1;
  double a2;

  printf("Testing FP_CONST macro...");

  // Test 1
  a1 = FP_CONST(1.2);
  a2 = fp_to_db(a1);
  expression = (d_abs(1.2 - a2) < ERROR_MARGIN);
  sprintf(error_message, "\nDouble was %f but fixed point was %f\n", 1.2, a2);
  assert(expression, error_message);

  // Test 2
  a1 = FP_CONST(89.234);
  a2 = fp_to_db(a1);
  expression = d_abs(89.234 - a2) < ERROR_MARGIN;
  sprintf(error_message, "\nDouble was %f but fixed point was %f\n", 89.234, a2);
  assert(expression, error_message);

  // Test 3
  a1 = FP_CONST(189.24);
  a2 = fp_to_db(a1);
  expression = d_abs(189.24 - a2) < ERROR_MARGIN;
  sprintf(error_message, "\nDouble was %f but fixed point was %f\n", 189.24, a2);

  // Test 4
  a1 = FP_CONST(-1.2);
  a2 = fp_to_db(a1);
  expression = d_abs(-1.2 - a2) < ERROR_MARGIN;
  sprintf(error_message, "\nDouble was %f but fixed point was %f\n", -1.2, a2);
  assert(expression, error_message);

  printf("Done\n");
}

void addition_tests() {
  fp_t a;
  fp_t b;

  printf("Testing fixed point addition...");

  // Test 1
  a = FP_CONST(1.52);
  b = FP_CONST(-.32);
  addition_test(a,b,1.52,-.32);

  // Test 2
  a = FP_CONST(902.314);
  b = FP_CONST(123.432);
  addition_test(a,b,902.314,123.432);

  // Test 3
  a = FP_CONST(-34.374);
  b = FP_CONST(3.432);
  addition_test(a,b,-34.374,3.432);

  // Test 4
  a = FP_CONST(-310.132);
  b = FP_CONST(-893.001);
  addition_test(a,b,-310.132,-893.001);

  // Test 5
  a = FP_CONST(20000.132);
  b = FP_CONST(20000.001);
  addition_test(a,b,32768,0);

  // Test 5
  a = FP_CONST(-20000.132);
  b = FP_CONST(-20000.001);
  addition_test(a,b,-32768,0);

  printf("Done\n");
}

void subtraction_tests() {
  fp_t a;
  fp_t b;

  printf("Testing fixed point subtraction...");

  // Test 1
  a = FP_CONST(1.52);
  b = FP_CONST(-.32);
  subtraction_test(a,b,1.52,-.32);

  // Test 2
  a = FP_CONST(902.314);
  b = FP_CONST(123.432);
  subtraction_test(a,b,902.314,123.432);

  // Test 3
  a = FP_CONST(-34.374);
  b = FP_CONST(3.432);
  subtraction_test(a,b,-34.374,3.432);

  // Test 4
  a = FP_CONST(-310.132);
  b = FP_CONST(-893.001);
  subtraction_test(a,b,-310.132,-893.001);

  // Test 5
  a = FP_CONST(20000.132);
  b = FP_CONST(-20000.001);
  subtraction_test(a,b,32768,0);

  // Test 5
  a = FP_CONST(-20000.132);
  b = FP_CONST(20000.001);
  subtraction_test(a,b,-32768,0);

  printf("Done\n");
}

void multiplication_tests() {
  fp_t a;
  fp_t b;

  printf("Testing fixed point multiplication...");

  // Test 1
  a = FP_CONST(1.52);
  b = FP_CONST(-.32);
  multiplication_test(a,b,1.52,-.32);

  // Test 2
  a = FP_CONST(2.314);
  b = FP_CONST(3.432);
  multiplication_test(a,b,2.314,3.432);

  // Test 3
  a = FP_CONST(-34.374);
  b = FP_CONST(3.432);
  multiplication_test(a,b,-34.374,3.432);

  // Test 4
  a = FP_CONST(-0.132);
  b = FP_CONST(-3.001);
  multiplication_test(a,b,-0.132,-3.001);

  // Test 5
  a = FP_CONST(128);
  b = FP_CONST(1024);
  multiplication_test(a,b,32768,1);

  // Test 6
  a = FP_CONST(-128);
  b = FP_CONST(-1024);
  multiplication_test(a,b,32768,1);

  // Test 7
  a = FP_CONST(-128);
  b = FP_CONST(1024);
  multiplication_test(a,b,-32768,1);

  printf("Done\n");
}

void division_tests() {
  fp_t a;
  fp_t b;

  printf("Testing fixed point division...");

  // Test 1
  a = FP_CONST(1.0);
  b = FP_CONST(1.0);
  division_test(a,b,1.0,1.0);

  // Test 2
  a = FP_CONST(12.0);
  b = FP_CONST(2.0);
  division_test(a,b,12.0,2.0);

  // Test 3
  a = FP_CONST(1.5);
  b = FP_CONST(0.3);
  division_test(a,b,1.5,0.3);

  // Test 4
  a = FP_CONST(-20.0);
  b = FP_CONST(0.5);
  division_test(a,b,-20.0,0.5);

  // Test 5
  a = FP_CONST(11.34);
  b = FP_CONST(20.23);
  division_test(a,b,11.34,20.23);

  // Test 6
  a = FP_CONST(101.342);
  b = FP_CONST(002.231);
  division_test(a,b,101.342,002.231);

  // Test 7
  a = FP_CONST(-20000);
  b = FP_CONST(.001);
  division_test(a,b,-32768,1);

  // Test 8
  a = FP_CONST(20000);
  b = FP_CONST(.001);
  division_test(a,b,32768,1);

  // Test 9
  b = FP_CONST(20);
  a = FP_CONST(.001);
  division_test(a,b,1,32768);



  // Test 9
  // ints
  int32_t c = 128;
  int32_t d = 5;
  division_test(c, d, 128.0, 5.0);

  printf("Done\n");
}

int main() {
  printf("--------------------\nRunning Unit Tests\n--------------------\n\n");
  conversion_to_fp_test();
  addition_tests();
  subtraction_tests();
  multiplication_tests();
  division_tests();
  printf("\n");
  printf("--------------------\nFinished Unit Tests\n--------------------\n\n");

  return 0;
}
