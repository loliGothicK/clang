// RUN: %clang -std=c++1y -c %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out

#define USE_PRINTF 1

#if USE_PRINTF
#include <cstdio>
#else
#define printf(...)
#endif

template<class T> struct X { };
template<> struct X<int> { 
  int get() const { return 50; }
};
int fi(int) { return 60; }
int fi(char) { return ' '; }

// These are used as type parameters
struct T00 { int i; T00(int i) : i(i) { }; int get() const { return i; }; };
struct T10 { int i; T10(int i) : i(i) { }; int get() const { return i; }; };
  
  
int main()
{

{
 auto L = []<class T>(T t, 
                      auto a, 
                      decltype(a) (*fp)(decltype(t)),
                      int x) {
  return a + t + fp(t) + x;
 };
 
 int i = L(' ', 12, fi, 1000);
 //CHECK: i = 1076
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
  
  //CHECK: i = 554 
  printf("i = %d\n", i);
}
{ // Test parameters that can only be explicitly specified
  auto L = []<class T, int N, template<class...> class V> (auto a) {
    return a;
  };
  
  char explicit_call = L.operator()<int, 3, X>('c');
  //CHECK: L.operator()<int, 3, X>('c') = c
  printf("L.operator()<int, 3, X>('c') = %c\n", explicit_call);
}
{
  auto L = []<class T>(auto a) {
    return a;
  };
  char cc = L.operator()<char>(77);
  //CHECK: cc = M 
  printf("cc = %c\n", cc);

}
{
  auto L = []<class T00>(auto a, T00 t00) {
            return [=]<class T10>(auto b, T00 t, T10 t10) {
                  return a + b + t00.get() + t.get() + t10.get(); 
           };
  };

  auto L2 = L(1000,T00(100));
  //int i = L2(1, X<char>{}, 4); // error
  double jj = L2(1.5, T00(10), T10(100));
  //CHECK: jj = 1211.5
  printf("jj = %f\n", jj); 

}
//*/
}