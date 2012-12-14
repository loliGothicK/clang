// RUN: %clang -c -std=c++1y %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out
#define USE_PRINTF 1

#if USE_PRINTF
#include <cstdio>
#else
#define printf(...)
#endif

int main() {
//* Test use of decltype and conversion to fptr
{  
  auto L = [](auto a, decltype(a) b) a + b;
  int (*fp)(int, int) = L;
  double (*fdp)(double, double) = L;
  // CHECK: fp(3,4) = 7
  printf("fp(3,4) = %d\n", fp(3,4));
  // CHECK-NEXT: fdp(3.14,2.77) = 5.910000
  printf("fdp(3.14,2.77) = %f\n", fdp(3.14,2.77));
}              
//*/              
//*test use of nested decltype and conversion to fptr
{  
  auto L = [](auto a)[](decltype(a) b)[](decltype(b) c) c;
  auto M = L('a');
  auto N = M('b');
  auto O = N('c');
  char (*fp)(char) = N;
  // CHECK-NEXT: fp('c') = c
  printf("fp('c') = %c\n", fp('c'));
}              
//*/              
  

}