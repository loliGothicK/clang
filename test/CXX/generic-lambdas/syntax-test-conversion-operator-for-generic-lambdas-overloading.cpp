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
  
}