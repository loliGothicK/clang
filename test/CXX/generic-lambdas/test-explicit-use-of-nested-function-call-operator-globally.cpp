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
struct X { };

double dglobal_var = 6.626;
auto A = []<int N>(auto a, auto b) b(a);
auto B = [](auto b, auto b2) A.operator()<3>(dglobal_var + b2, b); 

auto A2 = [](auto a) a;
auto B2 = [](auto a) A2.operator()(a);
auto C2 = B2(X{});

int main() {
{
  auto M = B(([](auto b) b), 3.14);
  //CHECK: M = 9.766000
  printf("M = %f\n", M);
}
 auto M2 = [](auto a) B2.operator()(a);
 //CHECK-NEXT: M2(2.77) = 2.770000
 printf("M2(2.77) = %f\n", M2(2.77));
    
}