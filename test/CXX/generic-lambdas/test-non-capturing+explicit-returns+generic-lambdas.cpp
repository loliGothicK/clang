// RUN: %clang -std=c++1y -c %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out


#define USE_PRINTF 1

#if USE_PRINTF
#include <cstdio>
#else
#define printf(...)
#endif

int main()
{
  
  auto l1 = [](auto a, auto b, auto c, auto d) -> int { 
    printf(a, b, c, d);
    return 5; 
  };
  
  l1("test %d %s %f", 5, "dave", 7.3); // ok
  
  auto l2 = [](auto* a, int i) -> int {
    auto* p = a + i;
    return (*a += i);
  };
  //l2(6, 7); // error
  int x = 0;
  int ret_x = l2(&x, 3);
  printf("local x = %d &  ret x= %d\n", x, ret_x);
  
  struct A {
    int x;
    float foo(int) { return 0.0; }
    static double bar(int) { return 0.0; }
  };
  
  enum { EY = 10 };
  auto y = [=](auto a
          , auto* b
          , int *p
          //, decltype(a) da
          , auto& lr
          , auto&& ur
          , auto (*fp)(int)
          , auto A::*memvar
          , auto (A::*memfun)(int)
          //, auto (&arr)[5]
          ) -> int {
    decltype(a) x = a + 5;
    printf("%s\n", b);
    printf("EY = %d", EY);
    return 5;
  };
  
  y(3, "abc", &x, x, '4', &A::bar, &A::x, &A::foo);
}