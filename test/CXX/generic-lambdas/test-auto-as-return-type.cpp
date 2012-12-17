// RUN: %clang -c -std=c++1y %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out
#define USE_PRINTF 1

#ifndef USE_PRINTF
# define printf(...) 

#else

# if USE_PRINTF 
#   include <cstdio>
# else
#   define printf(...) 
# endif

#endif


int main() {
//* Test Reference to auto
{
  int i = 10;
  auto L = [](auto& a) -> auto& { return a; };
  const auto& j = L(i);
  ++i;
  // CHECK: j = 11
  printf("j = %d\n", j);
}
{
  double d = 3.14;
  auto L = [](auto& a) -> auto& { return a; };
  const auto& j1 = L(d);
  ++d;
  // CHECK: j1 = 4.140000
  printf("j1 = %f\n", j1);
  
}
//*/
//* Test Reference to auto
{
  int i = 11;
  auto L = [](auto& a) -> auto* { return &a; };
  const auto* j = L(i);
  ++i;
  // CHECK: *j = 12
  printf("*j = %d\n", *j);
}
{
  double d = 2.77;
  auto L = [](auto& a) -> auto* { return &a; };
  const auto* j1 = L(d);
  ++d;
  // CHECK: *j1 = 3.770000
  printf("*j1 = %f\n", *j1);
  
}
//*/

}