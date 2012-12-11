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
template<class T> struct X { };
template<> struct X<int> { 
  int get() const { return 50; }
};
int fi(int) { return 60; }
char fi(char) { return ' '; }
//*/

int main()
{

{
 auto L = []<class T>(T t, 
                      auto a, 
                      decltype(a) (*fp)(decltype(t)),
                      int x) {
  return a + t + fp(t) + x;
 };
 
 int i = L(10, 12, fi, 1000);
 printf("i = %d\n", i);
}
//*
{
  auto L = []<class T, int N, template<class> class V>
                      (T (&t)[N], const V<T>& v, auto a) {
    return t[0] + N + v.get() + a;
  };
  int arr[]{1, 2, 3};
  int i = L(arr, X<int>{}, 500);
  
  //int i = L.operator()<int>(5);
  printf("i = %d\n", i);
}
{
  auto L = []<class T>(auto a) {
    return a;
  };
  int i = L.operator()<char>(77);
  printf("i = %d\n", i);

}
{
  auto L = []<class T>(auto a, T t) {
            return [=]<class T2>(auto b, T t0, T2 t2) {
                  return a + t + b + t0 + t2; 
           };
  };
  auto L2 = L(1000,10000);
  //int i = L2(1, X<char>{}, 4); // error
  int j = L2(1, 10, 100);
  printf("i = %d\n", j); // 11111

}
//*/
}