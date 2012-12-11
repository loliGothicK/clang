// RUN: %clang -std=c++1y -c %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out

#define USE_PRINTF 1
#if USE_PRINTF 
#include <cstdio>
#else
#define printf(...) 
#endif

int fp(char c, char* pc) { return c - 'a' + 1; }

int fp(char c, int* pi) { return c - 'a' + 1; }

int main() {
  auto L = [](auto a, decltype(a) b) 
  {
    return static_cast<decltype(a)>(a + b);
  };

  auto x = L('a', 4);
  //CHECK: sizeof(x) = 1 x = 101
  printf("sizeof(x) = %d x = %d\n", sizeof(x), x);

  auto L2 = [](auto a, auto *b, int (*fp)(decltype(a), decltype(b))) 
  {
    return static_cast<decltype(a)>(a + fp(a, b));
  };
  char c = 'b';
  auto y = L2('a', &c, &fp);
  printf("sizeof(y) = %d y = %c\n", sizeof(y), y);
  
  
}