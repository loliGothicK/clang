// RUN: %clang -c -std=c++1y %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out

#ifndef DONT_USE_PRINTF
#define USE_PRINTF 1
#else
#define USE_PRINTF 0
template<class ... PTs> void print(PTs ... pts) { }
#endif

#ifndef DONT_USE_PRINTF
#include "fv_print.h"
using namespace fv;
#endif

# if USE_PRINTF 
#   include <cstdio>
# else
#   define printf(...) 
# endif

//* Test Reference Variadics
namespace fv7718 {
/*
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
*/
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<> int variadic_foo() { return -1; }
int test_variadics() {
  auto L = []<class ... Ts>(Ts&& ... args)
     [&]()
      variadic_foo(
        ([&](Ts&& ... args22) variadic_foo(args22 ...) + args)
                                    (static_cast<Ts&&>(args)...) 
                 ...);
  //CHECK: A
  auto M = L('A', 3.14);            
  auto N = M();
  return 0;
}
int T = test_variadics();
}
//*/

//* Test value variadics
namespace fv7719 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<> int variadic_foo() { return -1; }
int test_variadics() {
  auto L = []<class ... Ts>(Ts ... args)
     [=]()
      variadic_foo(
        ([=](Ts ... args22) variadic_foo(args22 ...) + args)
                                    (args...) 
                 ...);
  auto M = L('A', 3.14);            
  auto N = M();
  return 0;
}
int T = test_variadics();
}
//*/



//* Test non-type variadics
namespace fv7720 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<int N, class T> T get(T t) { return t; }
template<int N, class T> struct get_type {
  typedef T type;
};  
template<> int variadic_foo() { return -1; }
int test_variadics() {
  auto L = []<int ... Ns, class ... Ts>(Ts ... args)
     [=]()
      variadic_foo(
        ([=](typename get_type<Ns,Ts>::type ... args22) 
                    //5)(args...));
                    variadic_foo(get<Ns>(args22) ...) + args)
                                    (args...) 
                    ...);
  auto M = L.operator()<1, 2>('A', 3.14);            
  auto N = M();
  return 0;
}
int T = test_variadics();
}
//*/

//* Test non-type variadics member function
namespace fv7721 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<int N, class T> T get(T t) { return t; }
template<int N, class T> struct get_type {
  typedef T type;
};  
template<> int variadic_foo() { return -1; }

template<int ... Ns>
int test_variadics() {
  auto L = []<class ... Ts>(Ts ... args)
     [=]()
      variadic_foo(
        ([=](typename get_type<Ns,Ts>::type ... args22) 
                    //5)(args...));
                    variadic_foo(get<Ns>(args22) ...) + args)
                                    (args...) 
                    ...);
  auto M = L('A', 6.626);            
  auto N = M();
  return 0;
}
int T = test_variadics<1, 2>();
}
//*/


//*
namespace fv7766 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<> int variadic_foo() { return -1; }
int test_variadics() {
  auto L = []<class ... Ts>(Ts&& ... args)
     [&]()
      variadic_foo(
        ([&](Ts&& ... args22) variadic_foo(args22 ...) + args)
                                    (static_cast<Ts&&>(args)...) 
                 ...);
  auto M = L('A', 3.14);            
  auto N = M();
  return 0;
}
int T = test_variadics();
}
//*/

//*
namespace fv7717 {

template<class ... Ts2> int f(Ts2 ... ts2) {  
  return 0;
}

template<typename...Ts> void nested2(Ts ...ts) { // expected-note 2{{here}}
    // Capture all 'ts', use only one.
 //   f([&ts...] { return ts; } ()...);
    // Capture each 'ts', use it.
    f([&ts] { return ts; } ()...);
    // Capture all 'ts', use all of them.
  //  f([&ts...] { return (int)f(ts...); } ());
    // Capture each 'ts', use all of them. Ill-formed. In more detail:
    //
    // We instantiate two lambdas here; the first captures ts$0, the second
    // captures ts$1. Both of them reference both ts parameters, so both are
    // ill-formed because ts can't be implicitly captured.
    //
    // FIXME: This diagnostic does not explain what's happening. We should
    // specify which 'ts' we're referring to in its diagnostic name. We should
    // also say which slice of the pack expansion is being performed in the
    // instantiation backtrace.
    //f([&ts] { return (int)f(ts...); } ()...); // \
    // expected-error 2{{'ts' cannot be implicitly captured}} \
    // expected-note 2{{lambda expression begins here}}
    //auto L = [=] { return f(ts ...); };
    //L();
  }
  template void nested2(int, char); // ok
}
//*/
//*
template<class T, int N>
struct Arr { T arr[N]; T operator[](int I) const { return arr[I]; } };

template <class _Tp> struct remove_reference        {typedef _Tp type;};
template <class _Tp> struct remove_reference<_Tp&>  {typedef _Tp type;};

template <class _Tp> struct remove_reference<_Tp&&> {typedef _Tp type;};

template <class T> T&& forward(typename remove_reference<T>::type& t) noexcept { return (T&&) t; }
template <class T> T&& forward(typename remove_reference<T>::type&& t) noexcept { return (T&&) t; }

//template<class ... T> struct first_type;
template<class T, class ... Rest> struct first_type {
  typedef T type;
}; 

template<class T, class ... Rest> struct second_type {
  typedef typename first_type<Rest...>::type type;
}; 
//*/

//*
template<class ... Ts> int foo(Ts ... ts) {
  static int i = 0;
  if (++i > 10 ) return 5;
  auto L = [=]() {
             foo(ts ...);
  };
  L();
  return 0;
};
int global = foo(1, 2, 3);
//*/
  
  
template<class ... Ts2> int foo2(Ts2 ... ts2) {  
  printf("foo2(Ts2 ... ts2)\n");
return 0; }
//*
template<class ... Ts1> char foo3(Ts1&& ... ts1) {
  auto L = [&]()
             [&] ()
                foo2(([&](decltype(ts1) A //, Ts1&& ... ts1
                                          ) 
                    ts1 + A) //(static_cast<decltype(ts1)>(ts1), ts1 ... 
                            //) 
                ...);
  
  auto M = L();
  auto N = M();
  return 5;
}
namespace fv2 {
char Cxx = foo3(1, 2.14, 'a'); 

}
//*/

//*
namespace fv3 {
template<class ... Ts1> char foo3(Ts1&& ... ts1) {
  static int i = 0;
  
  if (++i > 5 ) return 'a';
  
  auto L = [&]()
             [&] ()
                foo3(
                  ([&](decltype(ts1) A, Ts1&& ... ts2) 
                        ts1 + A)(static_cast<decltype(ts1)>(ts1), static_cast<Ts1&&>(ts1) ...) 
                ...);
  
  auto M = L();
  auto N = M();
  return 'b';
}
char Cxx = foo3(1, 2.14, 'a'); 

}
//*/

//*
template<class ... Ts1> int foo5(Ts1 ... ts1) {
  static int i = 0;
  if (++i > 5) return 5;
  auto L = [&]()
             [&] ()
                foo5(([&](decltype(ts1) A, Ts1 ... ts2) ts1 + A)(static_cast<decltype(ts1)>(ts1), 
                    static_cast<decltype(ts1)>(ts1) ... 
                ) ...);
  
  auto M = L();
  auto N = M();
  return 10;
  
}
//*/
/*
template <class _Tp> struct remove_reference        {typedef _Tp type;};
template <class _Tp> struct remove_reference<_Tp&>  {typedef _Tp type;};

template <class _Tp> struct remove_reference<_Tp&&> {typedef _Tp type;};

template <class T> T&& forward(typename remove_reference<T>::type& t) noexcept { return (T&&) t; }
template <class T> T&& forward(typename remove_reference<T>::type&& t) noexcept { return (T&&) t; }
//*/
int main() {
  foo5(1, 2, 3);
  
  
//*
{
  int local = 10;
  auto L = [=](auto a) local + a;
  int x = L(5);
}
//*/ 
  
//*
{
  auto B = []<class ... Ts>(Ts&& ... args)
              [&]() 
                foo2((([&](Ts&& i1, Ts&& ... args2) args + i1)(forward<Ts&&>(args), forward<Ts&&>(args) ...)) ...);
  auto L = B(1, 2.2, 'c');
  auto M = L();
}
//*/
//*
{
  auto B = []<class ... Ts>(Ts&& ... args)
              [&]()
                //foo2(args...);
                foo2(([&](Ts&& ... args2) args + foo2(args2...))
                //(forward<Ts&&>(args), forward<Ts&&>(args) ...)) 
                  ...);
  auto L = B(1, 2.2, 'c');
  auto M = L();
}
//*/

//*
{
  auto B = []<class ... Ts>(Ts&& ... args)
              [&]() 
                foo2(([&](Ts&& i1, Ts&& ... args2) args + i1 + foo2(args2...))
                //(forward<Ts&&>(args), forward<Ts&&>(args) ...)) 
                ...);
  auto L = B(1, 2.2, 'c');
}
//*/


//*
{
  auto binder = []<class ... Ts>(Ts ... args)     
                       [args...](auto f)
                            f((args)...);
  auto p = [](int i1, int i2, int i3) {
    printf("%d %d %d\n", i1, i2, i3);
  };
  auto L = binder(1, 2, 3);
  L(p);
}
//*/
  //auto binder = [](auto)
       //[]<class U>(int (X::*mem_fun)(U)) 0;
//*
{
 auto binder = []<class T>(T* obj) 
                  [=]<class ... Ts>(auto (T::*mem_fun)(Ts ...))
                     [=](Ts&& ... args)
                       [=]()
                        (obj->*mem_fun)(args...);
  
  struct A {
    char foo(int i, double d, char c) { 
      printf("char foo(%d, %f, %c)\n", i, d, c);
      return 0; 
    }
  };
  //auto (A::*mf)(int) = &A::foo;
  A a;
  auto B = binder(&a);
  auto mf2 = B(&A::foo);
  auto C = mf2(4, 3.14, 'A');
  C();
}
//*/
//*
{
 auto binder = []<class T>(T* obj) 
                  [=]<class ... Ts>(auto (T::*mem_fun)(Ts ...))
                     [=](typename first_type<Ts&& ...>::type arg1)
                        (obj->*mem_fun)(arg1);
  
  struct A {
    char foo(int i) //, double d) 
    {    
      printf("char foo(int)\n");
      return 0; 
    }
  };
  //auto (A::*mf)(int) = &A::foo;
  A a;
  auto B = binder(&a);
  auto mf2 = B(&A::foo);
  auto c = mf2(4.15);

}
//*/

//*       
{
 auto binder = [](auto& obj) {
                  using class_t = typename remove_reference<decltype(obj)>::type;
                  return [=]<class ... Ts>(auto (class_t::*mem_fun)(Ts ...) const)
                     [=](Ts&& ... args) 
                        (obj.*mem_fun)(args...);
                };
  
  struct A {
    char foo(int i) const{ 
      printf("char foo(int)\n");
      return 0; 
    }
  };
  //auto (A::*mf)(int) = &A::foo;
  A a;
  auto B = binder(a);
  auto mf2 = B(&A::foo);
  auto c = mf2(4.15);

}
//*/       
{
  //auto binder = [](auto)
  //     []<class U>(int (X::*mem_fun)(U)) 0;

}       
/*
{
  int local = 10;
  auto L = [=](auto a) mutable [=](auto b) mutable local += (a + b);
  auto M1 = L(100);
  auto M2 = L(1000);
  auto N1 = M1(' ');
  auto N2 = M2(' ');
  printf("Generic N1 = %d\n", N1);  
  printf("Generic N2 = %d\n", N2);  
}

{
  int local = 10;
  auto L = [=](int a) mutable [=](int b) mutable local += (a + b);
  auto M1 = L(100);
  auto M2 = L(1000);
  auto N1 = M1(' ');
  auto N2 = M2(' ');
  printf("N1 = %d\n", N1);  
  printf("N2 = %d\n", N2);  
}
  */
}