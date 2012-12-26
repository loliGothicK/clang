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

//* Test Nested Lambdas as class-member initializers
template<class T> struct Num {
  T t = T{};
  T num = ([](auto x)[=](auto y) x + y - 2)(t)(t);
};

template<> struct Num<char> {
  char fs = ' ';
  char num = ([](auto x)[=](auto y) x + y)(fs)(fs);
};

double dglobal = 3.14;
template<class T> struct Num<T*> {
  T* ps = &dglobal;
  T num = ([](auto x)[=](auto y) x + y)(*ps)(*ps);
};

Num<int> P;
Num<char> FS;
Num<double*> PS;


template<class T> struct Num2 {
  T t = T{};
  T num = ([](auto z) z)(t);
};
//*/


//* Ok
int fi(int) { return 0; }

template<class T> T foo(T t, T t2 = [](auto a) a) { return t; }
auto fv1 = foo(&fi);
//*/

//* Test generic lambdas as default args:

int fi2(int i) { return i + 1; }

template<class R, class A> int foo2(R (*fp1)(A), R (*fp2)(A) = [](auto a) a + 2) {  
  printf("fp1(A{}) = %d\n", fp1(A{}));
  printf("fp2(A{}) = %d\n", fp2(A{}));
  
  return 5;
}
//CHECK: fp1(A{}) = 1
//CHECK-NEXT: fp2(A{}) = 2
auto fv2 = foo2(&fi2);

//*/

//* Test default lambdas in member templates
template<class T00> struct XX {

  template<class T10> T10 foo(T00 t00, T10 t10, T00 (*fp1)(T00) = [](auto a) a)
  {
    auto L = [](auto a) a;
    auto t = fp1(t00);
    printf("fp1(t00) = %d\n", fp1(t00));
    /*
    struct Local {
      template<class T20> T20 foo_local(T00 t00, T10 t10, T20 t20, T20 (*fp2)(T20, T10) = [](auto a, auto b) a )
      {
        auto L = [](auto a) a;
        //int (*fp)(int) = L;
        //fp(3);
        return T20{};
      }
    };
    auto i = Local().foo_local(t00, t10, double{3.14});
    //*/
    return T10{};
  }
};
XX<int> xx;
auto gxx =  xx.foo(5, 'a');
//*/
int main() 
{
 //* Test Nested Lambdas
  //CHECK: P.num = -2
  printf("P.num = %d\n", P.num);
  //CHECK-NEXT: FS.num = @ (64)
  printf("FS.num = %c (%d)\n", FS.num, FS.num);
  //CHECK-NEXT: PS.num = 6.280000 
  printf("PS.num = %f\n", PS.num);
//*/  

}
