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


template<class T> struct defer {
  using type = T;
};

int main() 
{
{  
  auto L = [](auto a) -> 
              typename defer<decltype(a)>::type 
              { return a; };
  //CHECK: L('d') = d
  printf("L('d') = %c\n", L('d'));
}  
{  
  auto M = []<class X>(X x) -> 
              typename defer<X>::type 
              { return x; };
  //CHECK: M('d') = d
  printf("M('d') = %c\n", M('d'));
}  

}
