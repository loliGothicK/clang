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


int main() 
{
 // These dependent use of decltypes within bodies of generic lambdas
 // can confuse the ASTContext typer in keeping all the types 
 // straight, if all decltypes are canonicalized
  auto L = [](auto x, auto a) { decltype(x) y = x; return a; };
  auto M = [=](auto a)
              L(([](auto b) -> decltype(a) b), a);
              
  auto N = M(3.14);
  //CHECK: M(3.14) = 3.140000
  printf("M(3.14) = %f\n", M(3.14));
}