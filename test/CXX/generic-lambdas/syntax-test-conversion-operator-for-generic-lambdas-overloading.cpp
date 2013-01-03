// RUN: %clang_cc1 -fsyntax-only -verify -std=c++1y %s

template<class T> T f(T t) { return t; }

char f(int) { return 'a'; }

int main()
{
  auto L2 = [](auto a) {  //expected-note {{candidate template ignored:}}
    return f(a);
  };
  char (*fp2)(int) = L2;
  // note the function char f(int) above ...
  int (*fp3)(int) = L2; //expected-error {{no viable conversion}}
  char (*fp4)(char) = L2;
  
  // test two parameters
  {
    auto L3 = [](auto a, auto b) { //expected-note {{failed template argument deduction}}
      return a + b;
    };
    double (*fp0)(double, double) = L3; // ok  
    int (*fp1)(double, int) = L3; //expected-error {{no viable conversion}}
    double (*fp2)(double, int) = L3; // ok  
    int (*fp4)(int, int) = L3; //ok
  }
  // test nested lambdas
  {
    auto L = [](auto a)                 
              [](decltype(a) a, auto b)  //expected-note 2{{failed template argument deduction}}
                                a + b;
    auto M = L('c');
    int (*fp0)(char, int) = M; // ok
    char (*fp1)(char, int) = M; //expected-error {{no viable conversion}}
    int (*fp2)(int, char) = M; //expected-error {{no viable conversion}}
    
  }
  
/* This should be an error char and int
// but how do we test for messages from the (frontend) and no line number?
{
 struct Local {
  template<class T>
  static char foo(T t) { return t; }
 };
  auto F = [](auto f, auto n)       //fvexpected-errorxx {{'auto' in return type}}
    Local::foo(!n ? 1 : f(f, n - 1) * n);
  auto R = F(F, 2); //fvexpected-errorxx {{no matching function for call}}
}
//*/
}