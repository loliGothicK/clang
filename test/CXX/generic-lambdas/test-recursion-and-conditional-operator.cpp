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
//*
template<class T>
struct B { T n; B(T n) : n(n) { } T get() { return n; } };

template<class T>
struct D : B<T> { D(T n) : B<T>(n) { } };

template<class T> B<T> base(T n) { return B<T>{n}; }
template<class T, class R> D<T> derived(B<T> b, R r) { return D<T>{b.get()}; }
//*/

int main() {

//*
{
  auto Lx = [](auto f, auto n)
    !n ? 1 : f(f, n - 1) * n;
    
  auto M = Lx(Lx, 4);
  //CHECK: Lx(Lx, 4) = 24
  printf("Lx(Lx, 4) = %d\n", Lx(Lx, 4));
}
//*/
//* Recursive case is false
{
  struct Local { 
    template<class T> T get(T t) { return t; }
  };
  
  auto Ly = []<class T>(auto f, T n) {
    return n == -5 ? base(n) : derived(f(f, n - 1), Local{}.get<T>(n));
  }; 
  auto M = Ly(Ly, 5);
  //CHECK-NEXT: Ly(Ly, 5).get() = -5 
  printf("Ly(Ly, 5).get() = %d\n", Ly(Ly, 5).get());
}
//*/

//* Recursive case is true
{
  auto Lz = [](auto f, auto n)
    n ? f(f, n - 1) * n : 1;
    
  auto M = Lz(Lz, 6);
  //CHECK-NEXT: Lz(Lz, 6) = 720
  printf("Lz(Lz, 6) = %d\n", Lz(Lz, 6));
}
//*/
//* Simpler Nested Recursion
{
  auto FactX = [](auto f, auto n) 
    n < 2 ?
      (!n ? 1 : f(f, n - 1) * n)
      : f(f, n - 1) * n;
  auto M = FactX(FactX, 4);
  //CHECK-NEXT: FactX(FactX, 4) = 24
  printf("FactX(FactX, 4) = %d\n", FactX(FactX, 4));
}
//*/
//*
{
  auto La = [](auto f1) [=](auto n) !n ? 1 : f1(f1, n - 1) * n;
  auto Ma = La([](auto f2, auto n2) !n2 ? 1 : f2(f2, n2 - 1) * n2);
  auto Na = Ma(5);
  //CHECK-NEXT: Na = 120
  printf("Na = %d\n", Na);
}
//*/

//*
{
  auto F = [](auto f) -> auto& { return f; };
  F(5);
}
//*/
//* Does this parse the conditional expression?
{
  auto L = [](auto a) (a);
  auto Fy = [](auto f, auto n)
    {
      return (!n ? 1 : f(f, n - 1) * n) + 5;
    };
  auto R = Fy(Fy, 3);
  //CHECK-NEXT: Fy(Fy,3) = 86
  printf("Fy(Fy,3) = %d\n", Fy(Fy,3));
}
//*/
//*
{
  auto F = [](auto f, auto n) ->auto& 
    !n ? 1 : f(f, n - 1) * n;
  auto R = F(F, 2);
  //CHECK-NEXT: F(F,3) = 6  
  printf("F(F,3) = %d\n", F(F,3));
}
//*/
//* 
{
  auto X = [](auto f, auto n) ->auto&&
    !n ? 1 : f(f, n - 1);
  auto R = X(X, 2);
  //CHECK-NEXT: X(X,3) = 1
  printf("X(X,3) = %d\n", X(X,3));
  
}
//*/



//* This should NOT be an error ---
{
 struct Local {
  template<class T>
  static T foo(T t) { return t; }
 };
  auto Y = [](auto f, auto n) 
    Local::foo(!n ? 1.0 : f(f, n - 1) + n);
  auto R = Y(Y, 3);
  //CHECK-NEXT: Y(Y,7) = 29.000000
  printf("Y(Y,7) = %f\n", Y(Y, 7));
  
}
//*/

//* Need Decltype - although we should be able to do this without decltype
{
 //struct Local { };
 auto F555 = [](auto f, auto n) 
    n <= 0 ? n + 1 :
           ([f](auto n2) -> decltype(n) 
           n2 <= 0 ? 1 : f(f, n2 - 1) * n2)(n);
           
  auto R = F555(F555, 2);
  //CHECK-NEXT: F555(F555, 3.14) = 0.150143 
  printf("F555(F555, 3.14) = %f\n", F555(F555, 3.14));
  //CHECK-NEXT: F555(F555, 5) = 120
  printf("F555(F555, 5) = %d\n", F555(F555, 5));

}
//*/

//* Need Decltype for now
{
 //struct Local { };
 auto F666 = [](auto f, auto n) 
    n <= 0 ? n + 1 :
           ([=](auto f2) -> decltype(n) 
           n <= 0 ? 1 : f2(f2, n - 1) * n)(f);
           
  auto R = F666(F666, 2);
  //CHECK-NEXT: F666(F666, 3.14) = 0.150143
  printf("F666(F666, 3.14) = %f\n", F666(F666, 3.14));
  //CHECK-NEXT: F666(F666, 5) = 120
  printf("F666(F666, 5) = %d\n", F666(F666, 5));

}
//*/

//* Complex Nested Recursive case is true
{
  auto Fact4 = [](auto f, auto n) -> auto
    n > 5 
      ? (n == 6 
          ? f(f, n - 1) * n 
          : f(f, n - 1) * n)
      : (n < 3 
          ? (!n ? 1 : f(f, n - 1) * n)
          : f(f, n - 1) * n);
    
  auto M = Fact4(Fact4, 6);
  //CHECK-NEXT: Fact4(Fact4, 6) = 720
  printf("Fact4(Fact4, 6) = %d\n", Fact4(Fact4, 6));
}
//*/
//* Nested Recursive case with branches swapped
{
  auto Fact5 = [](auto f, auto n)
    n < 5 
      ? (n < 3 
          ? (!n ? 1 : f(f, n - 1) * n)
          : f(f, n - 1) * n)
      : (n == 6 
          ? f(f, n - 1) * n 
          : f(f, n - 1) * n);
      
  auto M = Fact5(Fact5, 6);
  //CHECK-NEXT: Fact5(Fact5, 6) = 720
  printf("Fact5(Fact5, 6) = %d\n", Fact5(Fact5, 6));
}

//*/
//* Simpler Nested Recursion
{
  auto Fact2 = [](auto f, auto n) 
    n < 2 ?
      (!n ? 1 : f(f, n - 1) * n)
      : f(f, n - 1) * n;
  auto M = Fact2(Fact2, 4);
  //CHECK-NEXT: Fact2(Fact2,4) = 24
  printf("Fact2(Fact2,4) = %d\n", M);
}

//*/
/* This should  be an error ---
{
 struct Local {
  template<class T>
  static T foo(T t) { return t; }
 };
  auto Y = [](auto f, auto n)  {
    return Local::foo(!n ? 1.0 : f(f, n - 1) + n);
    return 'c';
  };
  auto R = Y(Y, 3);
  printf("Y(Y, 7) = %f\n", Y(Y, 7));
  
}
//*/

//* ok 
{
  auto F = [](auto f, auto n) ->auto& 
    !n ? 1 : f(f, n - 1) * n;
  auto R = F(F, 2);
}
//*/
//* ok 
{
  auto F = [](auto f, auto n) 
    (!n ? n + 1 : f(f, n - 1) * n) + n + ([=](auto x) n + x)(n);
  auto R = F(F, 2);
}
//*/
/* not ok 
{
  auto F = [](auto f, auto n) 
    (!n ? n + 1 : f(f, n - 1) * n) + n + ([=](auto x) n + x)(f(f, n - 1));
  auto R = F(F, 2);
}
//*/

/* Not ok -  
{
  auto F = [](auto f, auto n) 
    (!n ? n + 1 : f(f, n - 1) * n) + 6.0; // + ([=](auto x) n + x)(n);
  auto R = F(F, 2);
}
//*/
//* 
{
  auto Lzx = [](auto f, auto n)
    n <= 0 ? 1 + n: ([=]() f(f, n - 1) * n)() ;
    
  auto M = Lzx(Lzx, 6.626);
  //CHECK-NEXT: Lzx(Lzx, 6.626) = 1046.281429
  printf("Lzx(Lzx, 6.626) = %f\n", Lzx(Lzx, 6.626));
}
//*/

}