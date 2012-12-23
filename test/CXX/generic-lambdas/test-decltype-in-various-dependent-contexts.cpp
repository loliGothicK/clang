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

template<class T> struct X { using type = T; };

int main() 
{
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
//*
{
 // These dependent use of decltypes within bodies of generic lambdas
 // can confuse the ASTContext typer in keeping all the types 
 // straight, if all decltypes are canonicalized
  auto L = [](auto x, auto a) { 
      X<decltype(x)> y{};  
      X<typename decltype(x(y))::type> xy{}; 
      return a; 
  };
  auto M = [=](auto a)
              L(([](auto b) -> decltype(b) b), a);
              
  auto N = M(3.14);
  //CHECK-NEXT: M(6.626) = 6.626000
  printf("M(6.626) = %f\n", M(6.626));
}
//*/
//*
{
  auto L = [](auto b)
            [](auto a) -> decltype(a) a;
  auto M = L('3');
  auto N = M(3.14);
}
//*/
} // main