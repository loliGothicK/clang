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
template<class T> struct X { int get() const { return 10; } };
int main() {
{
  auto L = []<int N>(auto (&ta)[N]) { return ta[0]; };
  int arr[]{1, 2, 3};
  //CHECK: L(arr) = 1
  printf("L(arr) = %d\n", L(arr));
  
}

}