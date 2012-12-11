// RUN: %clang -std=c++1y -c %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out

#define USE_PRINTF 1
#if USE_PRINTF 
#include <cstdio>
#else
#define printf(...) 
#endif

int main() {
  
  auto L = [](auto& a, auto b) {  
             return 12;  
           };

  int i = 0;
  int x = L(i, 10);  

  {
    auto L2 = [x](auto* a, auto b) {  
               return a + b + x;  
             };

    int i = 0;
    auto* x = L2(&i, 10);  
    printf("x = %p diff = %d\n", x, x - &i);
  }  
}