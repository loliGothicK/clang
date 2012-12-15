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
//*
{
  auto L = [](auto a) [=](decltype(a) b) a + b;
  auto M = L(3);
  auto N = M(4);
  //CHECK: L(3)(4) = 7
  printf("L(3)(4) = %d\n", L(3)(4));
}
//*/
//* Test decltype in return
{
  auto L = [](auto& a) -> decltype(a)& a;
  
  int i = 5;
  int &j = L(i);
  //CHECK: j = 5
  printf("j = %d\n", j);
  ++i;
  //CHECK: ++i; j = 6
  printf("++i; j = %d\n", j);
  
}
//*/
//* Test nested decltype return
{
  auto L = [](auto& a) [&](decltype(a) b) -> decltype(a)& a;
  
  int i = 5;
  int k = 10;
  auto M = L(k);
  int &j = M(i);
  //CHECK: j = 10 
  printf("j = %d\n", j);
  ++k;
  //CHECK: ++k; j = 11
  printf("++k; j = %d\n", j);
  
}
//*/

//* Test conversion to function pointer with decltype return
{
  auto L = [](auto& a) -> decltype(a)& a;
  int& (*fp)(int&) = L;  
  int i = 5;
  int &j2 = fp(i);
  //CHECK: j2 = 5
  printf("j2 = %d\n", j2);
  ++i;
  //CHECK: ++i; j2 = 6
  printf("++i; j2 = %d\n", j2);
  
}
//*/

//* Test nested decltype return and conversion to fptr
{
  auto L = [](auto& a) [](decltype(a)& b) -> decltype(b)& b;
  
  int i3 = 5;
  int k3 = 10;
  auto M = L(k3);
  int& (*fp)(int&) = M;
  int &j3 = M(k3);
  //CHECK: j3 = 10 
  printf("j3 = %d\n", j3);
  ++k3;
  //CHECK: ++k3; j3 = 11
  printf("++k3; j3 = %d\n", j3);
  ++k3;
  //CHECK: ++k3; fp(j3) = 12
  printf("++k3; fp(j3) = %d\n", fp(j3));
  
  
}
//*/

}