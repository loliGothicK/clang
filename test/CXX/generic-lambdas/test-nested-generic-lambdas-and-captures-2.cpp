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
template<class T00>
struct LL {
  template<class T10, int N11, template<class> class TT12> 
      T10 foo(T10 (&ta)[N11], T00 too, TT12<T00> tt12) {
          return ta[0];
  }
};
void check_for_learning_member_template() {
  int iarr[3] = { 1, 2, 3};
  LL<double> ll;
  ll.foo(iarr, 3.17, ll);
}

void check_nested_lambda_full_generic() {
  auto L = [](auto a) 
              [](auto a)
                  []<class T10, int N11, template<class> class TT12>(
                        T10 (&ta)[N11], decltype(a) b, TT12<decltype(a)> tt12) ta[0] + b;
  int iarr[3]{1, 2, 3};
  auto N = L('c');
  auto M = N(2.77);
  LL<double> ll;
  auto x = M(iarr, 6.5, ll);
  printf("x = %f\n", x);
}
void check_nested_variadic() {
  auto L = [](auto a) 
              []<class ... T>(T ... t) {                
              };
  auto M = L('a');
  M(1, 2, 4, 6, 7, 'c', 4.56);

}
void check_nested_conversion_operator() {
  auto L = [](auto a) [=](int x, auto a) [](auto b) b;
  auto M = L('a');
  auto N = M(3, 6.17);
  int (*pf)(int) = N;
  printf("pf() = %d\n", pf(3));
  
}
void check_nested_conversion_operator_within_nongeneric_lambda_interspersed() {
  auto L = [](auto a) [](decltype(a) x) [](decltype(x) b) b;
  auto M = L('a');
  auto N = M(5);
  int x = N(7.6);
  char (*pf)(char) = N;
  printf("pf() = %d\n", pf(' '));
  
}

void check_nested_captures() {
  int local = 5;
  auto L = [&](auto a) [&,a](auto b) mutable a + b + local;//{ return a++ + b + local; };
  auto N = L(' ');
  //auto M = N(3.14);
  printf("N(3.14) = %f\n", N(3.14)); 
  //++local;
  printf("N(3) = %d\n", N(3)); 
  //++local;
  printf("N(3.14) = %f\n", N(3.14)); 
  printf("N(3.14) = %f\n", N(3.14));   
  auto M = L(2.77);
  
  printf("M(3.14) = %f\n", M(3.14)); 
  //++local;
  printf("M(3) = %f\n", M(3)); 
  //++local;
  printf("M(3.14) = %f\n", M(3.14)); 
  printf("M(3.14) = %f\n", M(3.14));   
  printf("N(3.14) = %f\n", N(3.14));   
  
 }

 void check_nested_captures_with_changes() {
  int local = 5;
  auto L = [&](auto a) [&,a](auto b) mutable a++ + b + local;//{ return a++ + b + local; };
  auto N = L(' ');
  // CHECK: BEGIN check_nested_captures_with_changes
  // CHECK-NEXT: N(3.14) = 40.14
  printf("BEGIN check_nested_captures_with_changes\n");
  printf("N(3.14) = %f\n", N(3.14)); 
  //++local;
  printf("N(3) = %d\n", N(3)); 
  //++local;
  printf("N(3.14) = %f\n", N(3.14)); 
  printf("N(3.14) = %f\n", N(3.14));   
  auto M = L(2.77);
  
  printf("M(3.14) = %f\n", M(3.14)); 
  //++local;
  printf("M(3) = %f\n", M(3)); 
  //++local;
  printf("M(3.14) = %f\n", M(3.14)); 
  printf("M(3.14) = %f\n", M(3.14));   
  printf("N(3.14) = %f\n", N(3.14));   
  printf("END check_nested_captures_with_changes\n");
  
 }
  void check_simple_expression_syntax() {
    int local = 10;
    auto L = [&] local++;
    printf("check_simple_expression_syntax:: L() = %d\n", L());
    printf("check_simple_expression_syntax:: L() = %d\n", L());
    printf("check_simple_expression_syntax:: L() = %d\n", L());
    
  }
  void check_array_in_generic_lambda() {
    auto L = []<class T, int N>(auto b, T (&ta)[N]) b + ta[0];
    auto M = L; //('c');
    double arr[]{1.0, 2.0};
    auto N = M(4.5, arr);
    printf("%f\n", N);
  }
  template<class T, int N> int length(T (&ta)[N]) { return N; }

  void do_tests() {
    typedef void (*TestFunPtr_t)();
    TestFunPtr_t test_funs[] = {
        check_nested_captures
        , check_nested_captures_with_changes
        , check_nested_conversion_operator_within_nongeneric_lambda_interspersed
        , check_nested_conversion_operator
        , check_nested_variadic
        , check_nested_lambda_full_generic
        , check_simple_expression_syntax
        , check_array_in_generic_lambda
    };
    for (int i = 0; i < length(test_funs); ++i )
      test_funs[i]();
  }
//*/
int main() {
//*
  do_tests();
//*/
//*
  int local = 10;
  auto L = [=](auto a) [=](auto b) local;
  auto N = L('a');
  auto M = N(3.14);
//*/  
/*
{
  int local = 5;
  auto L = [=](int a) [=](int b) local; //[=](auto b) a + b + local;
  auto N = L('a');
  //auto M = N(3.14);
}  
//*/
}