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
struct OneLevelNesting {
    int x;
    template<class T>
    void foo(T t) const {
      auto L = [=](auto a) a + x;
      auto Lx = [this](auto a) a + x;
      printf("L(t) = %f\n", L(t));
      printf("Lx(t) = %f\n", Lx(t));
    }
    OneLevelNesting(int x) : x(x) { }
  };

struct TwoLevelNesting {
    int x;
    template<class T>
    void foo(T t) const {
      auto L2 = [=](auto a) [=] (auto b) b + a + x;
      auto Lx2 = [this](auto a)[this,a] (auto b) b + a + x;
      printf("L2(t)(t) = %f\n", L2(t)(t));
      printf("Lx2(t)(t) = %f\n", Lx2(t)(t));
    }
    TwoLevelNesting(int x) : x(x) { }
  };
  
int main()
{
//* One level nesting
{
  OneLevelNesting o(5);
  //CHECK: L(t) = 15.140000
  //CHECK-NEXT: Lx(t) = 15.140000
  o.foo(10.14);
}
{
  TwoLevelNesting two(7);
  //CHECK: L2(t)(t) = 27.600000
  //CHECK-NEXT: Lx2(t)(t) = 27.600000
  two.foo(10.3);
}

//*/  
}