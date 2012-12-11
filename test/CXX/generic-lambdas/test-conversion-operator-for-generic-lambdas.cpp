// RUN: %clang -std=c++1y -c %s -emit-llvm -o %s.bc 
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out
#define USE_PRINTF 1

#if USE_PRINTF
#include <cstdio>
#else
#define printf(...)
#endif
//*
template<class T> T f(T t) { return t; }

char f(int) { return 'a'; }

//*/
int main()
{
 //*
  auto L2 = [](auto a) {
    printf("L2::a = %d\n", a);
    return f(a);
  };
  //CHECK: L2::a = 32
  L2(' ');
  char (*fp2)(int) = L2;
  
  //CHECK: L2::a = 3
  //CHECK-NEXT: fp2(3) = a
  printf("fp2(3) = %c\n", fp2(3));
  int (*fp3)(int) = L2; //expected-error {{no viable conversion}}
  //fp3(6);
  char (*fp4)(char) = L2;
  fp4(' ');
//*/
/*
  auto Fac = [](size_t n, auto self) {
    if (n <= 0) return 1;
    else
      return self(n - 1, self) * n;
  };
  int x = Fac(4, Fac);
//*/  
/*  
  auto F = [](auto n) -> int{
    printf("F::n = %d\n", n);
    return f(n);
  };
  F(42);
  int (*fp5)(int) = F;
  fp5(10);
//*/

//*
 {
    struct X { };
    auto L = [](auto, auto)
    {
      return 5;
    };
    L(3, X());
  }
//*/
//*
  {
    auto L = [](auto a) { };
    
    L(3);
    L(4);
  }
//*/
//*
{
  auto L = [](int i) { return i + 1; };
  
  int (*fp)(int) = L;
  printf("fp(5) = %d\n", fp(5));
}
//*/              
//*/
 /*
  // Translational Semantics for the conversion operator
  struct LM {
    
    template<class T> static T& declval();

    template<class T> 
      auto operator()(T t) -> typename T::return_type
      {
        typename T::return_type tr{};
        printf("tr = %d\n", tr);
        return tr;
      }
      
      template<class T> static auto Invoke(T t) -> decltype( 
            declval<LM>().operator()(t))
      {
        LM l;
        return l(t);
      }
  };
  
  LM l;
  struct X {
    typedef int return_type;
  } x;
  int i = l(x);  
  int j = LM::Invoke(x);
//*/
}