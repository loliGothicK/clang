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
int main() {
/* Compile Time Error 
{
 struct Local { };
 auto F888 = [](auto f, auto n) 
    n <= 0 ? n + 1 :
           ([=](int (*f2)(decltype(F888), int)) //-> decltype(n) 
           n <= 0 ? Local{} : f2(f2, n - 1) * n)(f);
           
  auto R = F888(F888, 2);
  
  printf("F888(F888, 3.14) = %f\n", F888(F888, 3.14));
  printf("F888(F888, 5) = %d\n", F888(F888, 5));

}
//*/
{
  auto L = [](int i) [=](auto a) a + i;
  auto N = L(3);
  auto M = N(2.77);
}
//* Using non-generic lambdas with capture and assertion failure
{
 struct Local { };
 auto F888 = [](auto f, auto n) 
    n <= 0 ? n + 1 :
           ([f](int n2) -> int 
           n2 <= 0 ? 1 : f(f, n2 - 1) * n2)(n);
           
  auto R = F888(F888, 2);
  //CHECK: F888(F888, 3.14) = 6.000000 
  printf("F888(F888, 3.14) = %f\n", F888(F888, 3.14));
  //CHECK-NEXT: F888(F888, 5) = 120 
  printf("F888(F888, 5) = %d\n", F888(F888, 5));

}
//*/

//* Using non-generic lambdas 
{
 struct Local { };
 auto F888 = [](auto f, auto n) 
    n <= 0 ? n + 1 :
           ([f](int n2) -> int 
           n2 <= 0 ? 1 : n2)(n);
           
  auto R = F888(F888, 2);
  
  printf("F888(F888, 3.14) = %f\n", F888(F888, 3.14));
  printf("F888(F888, 5) = %d\n", F888(F888, 5));

}
//*/

//* Using non-generic lambdas - implict return type
{
 struct Local { };
 auto F888 = [](auto f, auto n) 
    n <= 0 ? n + 1 :
           ([f](int n2) //-> int 
           n2 <= 0 ? 1 : n2)(n);
           
  auto R = F888(F888, 2);
  
  printf("F888(F888, 3.14) = %f\n", F888(F888, 3.14));
  printf("F888(F888, 5) = %d\n", F888(F888, 5));

}
//*/

//* Using non-generic lambdas - assertion error on decltype(F888)
{
 struct Local { };
 auto F888 = [](auto f, auto n) 
    n <= 0 ? n + 1 :
           ([f](int n2) -> int 
           n2 <= 0 ? 1 : f(f, n2 - 1))(n);
           
  auto R = F888(F888, 2);
  
  printf("F888(F888, 3.14) = %f\n", F888(F888, 3.14));
  printf("F888(F888, 5) = %d\n", F888(F888, 5));

}
//*/
//* Using non-generic lambdas - assertion error on decltype(F888)
{
 struct Local { };
 auto F888 = [](auto f, auto n) 
    n <= 0 ? n + 1 :
           ([f](int n2) -> decltype(n2) 
           n2 <= 0 ? 1 : n2)(n);
           
  auto R = F888(F888, 2);
  
  printf("F888(F888, 3.14) = %f\n", F888(F888, 3.14));
  printf("F888(F888, 5) = %d\n", F888(F888, 5));

}
//*/
}