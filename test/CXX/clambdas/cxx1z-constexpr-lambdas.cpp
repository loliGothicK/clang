// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING

namespace test_non_literal_capture {

struct NLit {
  volatile int v; //expected-note{{volatile type}}
};
constexpr NLit V; //expected-error {{cannot have non-literal type}}
//TODO: this should be an error
constexpr auto L = [v = V] { return 0; }; 
                                          
constexpr auto L2 = [&v = V] { return 0; }; // OK

} //end test_non_literal_capture
namespace test_if_conversion_is_constepxr {

namespace ns1 {
void f() {
  auto L = [](int a) { return a; };
  constexpr int (*lp)(int) = L;
  lp(5);
  
  auto GL = [](auto a) { return a; };
  constexpr char (*lpc)(char) = GL;
  lpc('a');
  
}

} //end ns1
namespace ns2 {
constexpr auto foo(int n) {
  int (*fp) (int) = [] (int n) { static int i; return 0; };
  return fp;
}
constexpr auto L = foo(3);

} // ns2
} //end test_if_conversion_is_constepxr

namespace test_non_capturing_lambda_is_constexpr {
  
namespace ns1 {
  auto L = [](int a) { return a; };
  constexpr int I = L(3);
} //end ns1  
  

} // end ns test_non_capturing_lambda_is_constexpr

namespace test_if_nonconstexpr_lambda_does_not_error {

namespace ns1 {
auto L = [](int a) { 
  volatile int i = 0;
  int arr[a];
  return a + i; 
};

int main() {
  volatile int x = 5;
  L(x);
  return 0;
}
} //end ns1

} // end ns test_if_nonconstexpr_lambda_does_not_error


namespace test_captures_within_constexpr_lambdas {

namespace ns1 {
  void explicit_scalar_caps_by_val() {
    const int N = 3;
    auto L = [N] { return N; };
    static_assert(L() == 3, "");
    auto Plus3 = [N](int n) { return N + n; };
    static_assert(Plus3(4) == 7, "");
    auto Plus3GL = [N](auto n) { return N + n; };
    static_assert(Plus3GL(4) == 7, "");
    
    {
      int n = 3;
      auto L = [n] { return n; }; //expected-note{{not valid in a constant expression}}
      constexpr int I = L(); //expected-error{{must be initialized by a constant expression}}\
                             //expected-note{{in call to}}
      
    }
  }
  
  void explicit_scalar_caps_by_ref() {
    const int N = 3;
    auto L = [&N] { return N; };
    static_assert(L() == 3, "");
    auto Plus3 = [&N](int n) { return N + n; };
    static_assert(Plus3(4) == 7, "");
    auto Plus3GL = [&N](auto n) { return N + n; };
    static_assert(Plus3GL(4) == 7, "");
    
    {
      int n = 3;
      auto L = [&n] { return n; }; //expected-note{{not valid in a constant expression}}
      constexpr int I = L(); //expected-error{{must be initialized by a constant expression}}\
                             //expected-note{{in call to}}
                             
      
    }
  }
  struct Literal { int i; double d = 3.14; 
    constexpr int operator+(int n) const {
      return i + n;
    }
  };
  
  void explicit_literal_caps_by_val() {
    constexpr Literal N = { 3 };
    auto L = [N] { return N; };
    static_assert(L().i == 3, "");
    auto Plus3 = [N](int n) { return N.i + n; };
    static_assert(Plus3(4) == 7, "");
    auto Plus3GL = [N](auto n) { return N.i + n; };
    static_assert(Plus3GL(4) == 7, "");
    
    {
      Literal n = { 3 };
      auto L = [n] { return n; }; //expected-note{{not valid in a constant expression}}\
                                  //expected-note{{in call to}}
      constexpr int I = L().i;   //expected-error{{must be initialized by a constant expression}}\
                             //expected-note{{in call to}}
                             
      
    }
  }
  void explicit_Literal_caps_by_ref() {
    constexpr Literal N = { 3 };
    auto L = [&N] { return N; };
    static_assert(L().i == 3, "");
    auto Plus3 = [&N](int n) { return N.i + n; };
    static_assert(Plus3(4) == 7, "");
    auto Plus3GL = [&N](auto n) { return N.i + n; };
    static_assert(Plus3GL(4) == 7, "");
   
  }
  
  void implicit_scalar_caps_by_val() {
    const int N = 3;
    auto L = [=] { return N; };
    static_assert(L() == 3, "");
    auto Plus3 = [=](int n) { return N + n; };
    static_assert(Plus3(4) == 7, "");
    auto Plus3GL = [=](auto n) { return N + n; };
    static_assert(Plus3GL(4) == 7, "");
    
  }

  void implicit_scalar_caps_by_ref() {
    const int N = 3;
    auto L = [&] { return N; };
    static_assert(L() == 3, "");
    auto Plus3 = [&](int n) { return N + n; };
    static_assert(Plus3(4) == 7, "");
    auto Plus3GL = [&](auto n) { return N + n; };
    static_assert(Plus3GL(4) == 7, "");
    
  }
  
  void implicit_literal_caps_by_val() {
    constexpr Literal N = { 3 };
    auto L = [=] { return N; };
    static_assert(L().d == 3.14, "");
    auto Plus3 = [=](int n) { return N.d + n; };
    constexpr double D1 = Plus3(4);
    auto Plus3GL = [=](auto n) { return N.d + n; };
    constexpr double D = Plus3GL(4);
  }

  void implicit_literal_caps_by_ref() {
    constexpr Literal N = { 3 };
    auto L = [&] { return N; };
    
    static_assert(L().d == 3.14, "");
    
    auto Plus3 = [&](int n) { return N + n; };
    static_assert(Plus3(4) == 7, "");
    
    auto Plus3GL = [&](auto n) { return N + n; };
    static_assert(Plus3GL(4) == 7, "");
    
  }
  
   void init_scalar_caps_by_val() {
    const int N = 3;
    auto L = [I = N] { return I; }; //expected-note{{not valid in a constant expression}}
    static_assert(L() == 3, "");//expected-error{{is not an integral constant expression}}\
                                //expected-note{{in call to}}
    // a constexpr lambda is ok.
    constexpr auto LC = [I = N] { return I; };
    static_assert(LC() == 3, "");
    auto Plus3 = [I = N](int n) { return I + n; };//expected-note{{not valid in a constant expression}}
    static_assert(Plus3(4) == 7, ""); //expected-error{{is not an integral constant expression}}\
                                //expected-note{{in call to}}
    auto Plus3GL = [I = N](auto n) { return I + n; };//expected-note{{not valid in a constant expression}}
    static_assert(Plus3GL(4) == 7, "");//expected-error{{is not an integral constant expression}}\
                                //expected-note{{in call to}}
  }
  const int static_N = 5;
  void init_scalar_caps_by_ref() {
    const int N = 3; //expected-note{{declared here}}
    // A reference can only be a constant expression if it refers to an object of static storage duration, N is not such an object
    auto L = [&I = N] { return I; };//expected-note{{not valid in a constant expression}}
    static_assert(L() == 3, "");//expected-error{{is not an integral constant expression}}\
                                //expected-note{{in call to}}
    
    constexpr auto LC =  //expected-error{{must be initialized by a constant expression}} \
                         //expected-note{{reference to 'N' is not a constant expression}}
              [&I = N] { return I; }; //expected-note{{subexpression not valid}}
    static_assert(LC() == 3, "");//expected-error{{is not an integral constant expression}}\
                                //expected-note{{in call to}}
    constexpr auto LC2 = [&I = static_N] { return I; };
    static_assert(LC2() == 5, "");
    auto Plus3 = [&I = N](int n) { return I + n; }; //expected-note{{not valid in a constant expression}}
    static_assert(Plus3(4) == 7, "");//expected-error{{is not an integral constant expression}}\
                                //expected-note{{in call to}}
    auto Plus3GL = [&I = N](auto n) { return I + n; }; //expected-note{{not valid in a constant expression}}
    static_assert(Plus3GL(4) == 7, "");//expected-error{{is not an integral constant expression}}\
                                //expected-note{{in call to}}
    
  }
  
} //end ns1

} // end test_captures_within_constexpr_lambdas

namespace test_lambdas_within_constexpr_functions {

namespace ns1 {
constexpr int f(int n) {
  auto L = [] { };
  return n;
}

int x = f(3);
} // end ns1

namespace ns2 {
constexpr auto f() {
  return [] { return 1; };
}
constexpr auto L = f();
} //end ns2

namespace ns3 {

constexpr auto f() {
  return [] { return 1; };
}
constexpr auto L = f()();
constexpr int L2 = [] { return 1; }();


} //end ns3

namespace lambda_captures {
namespace ref_captures {
  namespace ns1 {
    constexpr int foo(int n) {
      auto L = [&n] { return ++n; };
      return L();
    }
    constexpr int foo_init(int n) {
      auto L = [&i = n] { return ++i; };
      return L();
    }
    int main() {
      constexpr int I = foo(5);
      static_assert(I == 6, "");
      constexpr int I2 = foo_init(5);
      static_assert(I2 == 6, "");
    }
  } //end ns1
  
  namespace ns2 {
    constexpr int foo(int m) {
      int n = m + m;
      auto L = [&n] { return ++n; };
      n = m;
      return L();
    }
    constexpr int foo_init(int m) {
      int n = m + m;
      auto L = [&i = n] { return ++i; };
      return L();
    }
    int main() {
      constexpr int I = foo(5);
      static_assert(I == 6, "");
      constexpr int I2 = foo_init(5);
      static_assert(I2 == 11, "");
    }
  } //end ref_captures::ns2
  namespace ns3 {
  constexpr int foo(int n) {
    int &r1 = n;
    int &r2 = r1;
    auto L = [&] {

      return ++r2;
    };
    n += 100;
    return L();
  }
  static_assert(foo(10) == 111, "");
  } // end ns3
} //end ns ref_captures

namespace gen_lambdas {
  namespace ns1 {
    constexpr int foo(int n) {
      auto L = [&n](auto a) { return ++n + a[0]; };
      return L("hello");
    }
    constexpr int foo_init(int n) {
      auto L = [&i = n](auto *a) { return ++i + a[0]; };
      return L("hi");
    }
    int main() {
      constexpr int I = foo(5);
      static_assert(I == 6 + 'h', "");
      constexpr int I2 = foo_init(5);
      static_assert(I2 == 6 + 'h', "");
    }
  } // end gen_lambdas::ns1
} //end gen_lambdas
namespace ns1 {

constexpr int f(int n) {
  int x = 7;
  auto L = [n, x] { return n + x; };
  int ret = L();
  return ret;
}

constexpr int I = f(5);
static_assert(I == f(5), "");
} //ens ns1

namespace ns2 {
constexpr auto f(int n) {
  int x = n + 1;
  auto L = [=] { return n + x; };
  return L;
}

void foo() {
  constexpr auto L = f(5);
  constexpr int I = L();
  static_assert(I == f(5)(), "");
}
} //end ns2

namespace ns3 {
auto L = [](auto a, auto b) { 
   decltype(a) a2 = a;
   return [=](auto f) { return a + b + a2 + f(a); };
};

constexpr auto LC = [](auto a, auto b) { 
   decltype(a) a2 = a;
   return [=](auto f) { return a + b + a2 + f(a); };
};

struct Lit { int n; };

constexpr Lit operator+(Lit l, Lit r) { return Lit{}; }

constexpr auto MC = LC('0', 10);
constexpr auto NC = MC([](auto a) { return a + 1; });

static_assert(NC == ('0' + 10 + '0' + '0' + 1), "");

} // end ns3
namespace capture_this {
namespace ns1 {
struct A {
  int a;
  constexpr auto mf() const {
    return [this] { return a; };
  }  
};

constexpr A f(int n) {
  A x = { n };
  return x;
}

constexpr A a = f(5);
constexpr auto L = a.mf();
constexpr int ma = L();
static_assert(ma == 5, "");
} // end capture_this::ns1
namespace ns2 {

constexpr auto foo(int n) {
  struct L {
    int m;
    constexpr auto getRef() const { return [&] { return m; }; };
    constexpr auto getVal() const { return [=] { return m; }; };

  } l = { n };
  return l;
}

constexpr auto LocObj = foo(10);
constexpr auto LambRef = LocObj.getRef();
constexpr auto LambVal = LocObj.getVal();

static_assert(LambRef() == 10, "");
static_assert(LambVal() == 10, "");

} // end ns2

namespace ns3 {

constexpr auto foo(int n) {
  struct L {
    int m;
    constexpr auto getRef() const { return [&] { return [&] { return m; };  }; };
    constexpr auto getVal() const { return [=] { return [=] { return m; }; }; };
    constexpr auto getRefVal() const { return [&] { return [=] { return m; };  }; };
    constexpr auto getValRef() const { return [=] { return [&] { return m; }; }; };
    

  } l = { n };
  return l;
}

constexpr auto LocObj = foo(10);
constexpr auto LambRef = LocObj.getRef()();
constexpr auto LambVal = LocObj.getVal()();
constexpr auto LambRefVal = LocObj.getRefVal()();
constexpr auto LambValRef = LocObj.getValRef()();

static_assert(LambRef() == 10, "");
static_assert(LambVal() == 10, "");
static_assert(LambRefVal() == 10, "");
static_assert(LambValRef() == 10, "");

} // end ns3

namespace ns4 {

constexpr auto foo(int n) {
  struct L {
    int m;
    constexpr auto getRef() const { return [&] { return [&] () -> auto&{ return (*this); };  }; };
    constexpr auto getVal() const { return [=] { return [=] () -> auto& { return (*this); }; }; };
    

  } l = { n };
  return l;
}

constexpr auto LocObj = foo(10);
constexpr auto &LambRef = LocObj.getRef()()();
constexpr auto &LambVal = LocObj.getVal()()();

static_assert(&LambRef == &LocObj, "");
static_assert(&LambVal == &LocObj, "");

} // end ns4


} // end capture_this

namespace array_captures {
namespace ns1 {
  constexpr int array_by_val(int n) {
      
      int arr[2][3] = { { 1, n, 3 }, { n, 5, 6 } };
      auto L = [arr] { return arr[0][1] + arr[1][0] + arr[1][2]; };
      return L();
    }
  constexpr int I = array_by_val(3);
  static_assert(I == 12, "");
} //end array_captures::ns1
namespace ns2 {
  constexpr auto array_by_val(int n) {
    int arr[2][3] = { { 1, n, 3 }, { n, 5, 6 } };
    auto L = [arr] { return arr[0][1] + arr[1][0] + arr[1][2]; };
    return L;
  }
  constexpr auto L = array_by_val(3);
  static_assert(L() == 12, "");
} //end array_captures::ns2
namespace ns3 {
  constexpr auto array_by_ref(int n) {
    int arr[2][3] = { { 1, n, 3 }, { n, 5, 6 } }; //expected-note{{declared here}}
    auto L = [&arr] { return arr[0][1] + arr[1][0] + arr[1][2]; };
    return L;
  }
  constexpr auto L = array_by_ref(3); //expected-error {{must be initialized by a constant expression}} \
                                      //expected-note{{reference to 'arr'}}
   
} //end array_cap::ns3
namespace ns4 {
  constexpr int array_by_ref(int n) {
    int arr[2][3] = { { 1, n, 3 }, { n, 5, 6 } }; 
    auto L = [&arr] { return arr[0][1] + arr[1][0] + arr[1][2]; };
    ++arr[0][1];
    return L();
  }
  constexpr int I = array_by_ref(3); 
  static_assert(I == 13, "");
   
} //end array_cap::ns4

namespace ns5 {
constexpr int foo(int n) {
  int arr[10] = { n, n + 1, n + 2, n + 3, n + 4, n + 5 };
  auto L = [=] () mutable {
    for (int i = 0; i < 10; ++i)
      ++arr[i];
    return [&] {
      for (int i = 0; i < 10; ++i)
        arr[i] += 100;
      return [=] () mutable {
        for (int i = 0; i < 10; ++i)
          arr[i] += 1000;
        return arr[0];
      };
    };
  };
  auto M = L();
  auto N = M();
  auto N2 = M();
  int I = N2();
  return I;
}

constexpr int I = foo(3);
static_assert(I == 1204, "");
} // end ns5

namespace ns6 {
constexpr int foo(int n) {
  int arr[10][15] = { { n, n + 1, n + 2, n + 3, n + 4, n + 5 } };
  auto L = [=] () mutable {
    for (int i = 0; i < 10; ++i)
      ++arr[0][i];
    return [&] {
      for (int i = 0; i < 10; ++i)
        arr[0][i] += 100;
      return [=] () mutable {
        for (int i = 0; i < 10; ++i)
          arr[0][i] += 1000;
        return arr[0][0];
      };
    };
  };
  auto M = L();
  auto N = M();
  auto N2 = M();
  int I = N2();
  return I;
}

constexpr int I = foo(3);
static_assert(I == 1204, "");
} // end ns6

namespace ns7 {
constexpr auto foo(int n) {
  int arr[10][15] = { { n, n + 1, n + 2, n + 3, n + 4, n + 5 } };
  auto L = [=] {
    return [=] {
      return [=]  {
        return arr[0][0] + arr[0][1];
      };
    };
  };
  return L;  
}

constexpr auto L = foo(3);

static_assert(L()()() == 7, "");
} // end ns7

namespace ns8 {
constexpr auto obj1(int n) {
  const char *str = "abcdefghijklmnopqrstuvwxyz";
  auto L = [&]  { return str[n++]; };
  auto M1 = [&] { str = "123456789@#$"; return L(); };
  auto M2 = [&] { str = "!@#$%^&*()-_=+{}<>"; return L(); };
  struct { char first; char second; char third; } ret = { L(), M1(), M2() };
  return ret;
}
constexpr auto P = obj1(1);
static_assert(P.first == 'b', "");
static_assert(P.second == '3', "");
static_assert(P.third == '$', "");
} //end ns8

namespace ns9 {
constexpr auto obj1(int n) {
  const char *str = "abcdefghijklmnopqrstuvwxyz";
  auto L = [=] () mutable { return str[n++]; };
  auto M1 = [=, &L] () mutable { str = "123456789@#$"; return L(); };
  auto M2 = [=, &L] () mutable { str = "!@#$%^&*()-_=+{}<>"; return L(); };
  struct { char first; char second; char third; } ret = { L(), M1(), M2() };
  return ret;
}
constexpr auto P = obj1(1);
static_assert(P.first == 'b', "");
static_assert(P.second == 'c', "");
static_assert(P.third == 'd', "");
} //end ns9

namespace ns10 {

constexpr int foo(int n) {
  const char *pc = "hello world";
  auto L = [&] {
    return [&] { return pc[n]; };
  };
  return L()();
}
static_assert(foo(0) == 'h', "");

} //end ns10
namespace ns11 {
constexpr int foo(int n) {
  struct X {
    char c;
  } x = { 'h' };
  auto L = [&] {
    x.c = 'i';
    return [&] { return x.c; };
  };
  return L()();
}

static_assert(foo(0) == 'i', "");


} // end ns11
} //end array_captures
} // end lambda_captures

namespace test_pointer_captures {

namespace ns1 {
const int g = 10;
const int h = 20;
constexpr auto obj2(int n) {
  const int *p = &g;
  auto L = [&] { return *p + *p + n; };
  p = &h;
  ++n;
  return L();
}
constexpr auto P = obj2(1);
static_assert( P == 42, "");

} // end ns1


namespace ns2 {

constexpr auto obj2(int n) {
  const int *p = nullptr;
  auto L = [&] { return *p + *p + n; };
  p = &n;
  ++n;
  return L();
}
constexpr auto P = obj2(1);
static_assert( P == 6, "");

} // end ns2


} // end test_pointer_captures

} //end test_lambdas_within_constexpr_functions

namespace test_recursion {
namespace ns1 {
/*constexpr*/
auto getFactorializer = [] {
  auto FacImpl = [](auto fac, int n) {
    if (n <= 0) return 1;
    return n * fac(fac, n - 1);
  };
  auto Fact = [=](int n) {
    return FacImpl(FacImpl, n);
  };
  return Fact;
};

/*constexpr*/auto Factorial = getFactorializer();
static_assert(Factorial(5) == 120, "");

} //end ns1

namespace ns2 {

constexpr auto getFactorializer = [] {
  unsigned int optimization[] = { 1, 1, 2, 6, 24, 120 };
  auto FacImpl = [=](auto fac, unsigned n) {
    if (n <= 5) return optimization[n];
    return n * fac(fac, n - 1);
  };
  auto Fact = [=](int n) {
    return FacImpl(FacImpl, n);
  };
  return Fact;
};

constexpr auto Factorial = getFactorializer();

static_assert(Factorial(5) == 120, "");
static_assert(Factorial(10) == 3628800, "");

} //end ns2

namespace ns3 {

constexpr auto getFactorializer = [] {
  unsigned int optimization[6] = {}; 
  auto for_each = [](auto *b, auto *e, auto pred) {
    auto *it = b;
    while (it != e) pred(it++ - b);
  };
  
  for_each(optimization, optimization + 6, [&](int n) {
    if (!n) optimization[n] = 1;
    else 
      optimization[n] = n * optimization[n-1];
  });
  
  auto FacImpl = [=](auto fac, unsigned n) {
    if (n <= 5) return optimization[n];
    return n * fac(fac, n - 1);
  };
  auto Fact = [=](int n) {
    return FacImpl(FacImpl, n);
  };
  return Fact;
};

constexpr auto Factorial = getFactorializer();

static_assert(Factorial(5) == 120, "");
static_assert(Factorial(10) == 3628800, "");


}
} //end test_recursion
namespace test_lambda_call_thru_fptr {
namespace ns1 {

void foo() {
  auto L = [](int a) { return a + 5; };
  constexpr int (*fp)(int) = L;
  constexpr int I = fp(3);
  static_assert(I == L(3), "");
  static_assert(I == 8, "");
}


void gen_foo() {
  auto L = [](auto a) { return a + 5; };
  constexpr int (*fp)(int) = L;
  constexpr int I = fp(3);
  static_assert(I == L(3), "");
  static_assert(I == 8, "");
}
} //end ns1

} // end ns test_lambda_call_thru_fptr

namespace test_captures_more {
namespace ns1 {

constexpr int foo(int x) {
  auto L = [x] () mutable { return ++x; };
  int m = L();
  return x;
}
static_assert(foo(3) == 3, "");
 
} // end test_captures_more::ns1
namespace ns2 {
constexpr int fooL(int n) {
  int &x = n;
  auto L = [x] () mutable { ++x; ++x; return x;};  
  return L() + L();
}

constexpr auto L = fooL(4);

static_assert(L == 14, "");

} //end test_captures_more::ns2
namespace ns3 {
constexpr int foo(int n) {
  int arr[2][3] = { { 1, n, 3 }, { 4, 5, 6 } };
  auto &arrRef = arr;
  auto L = [arrRef] { return arrRef[0][1]; };
  
  return L();
}
static_assert(foo(3) == 3, "");

} // end test_captures_more::ns3
namespace ns4 { // capture_lambdas
constexpr auto foo(int n) {
  auto L = [=] { return n; };
  return [=] { return L; };
}

int main() {
  constexpr auto L1 = foo(3);
  static_assert(L1()() == 3, "");
}
} //end ns4

namespace nested_captures {
namespace ns1 {
template<class T1, class T2>
struct P {
  T1 first;
  T2 second;
};
template<class T1, class T2> constexpr 
P<T1, T2> make_P(T1 t1, T2 t2) {
  return P<T1, T2>{t1, t2};
}

constexpr auto foo(int n) {
  auto L = [=] () mutable { 
    return [&] 
       { return ++n; }; 
  };
  auto M = L();
  n = 10;
  return M(), M();
}

constexpr auto foo_ref(int n) {
  auto L = [&] () mutable { 
    return [&] 
       { return ++n; }; 
  };
  auto M = L();
  n = 10;
  return M(), M();
}


constexpr auto bar(int n) {
  auto L = [=] () mutable { 
    return make_P([&] 
       { return ++n; }, [&] { return --n; }); 
  };
  auto P = L();
  return P.first(), P.second();
}


int main() {
  constexpr auto L1 = foo(3);
  static_assert(L1 == 5, "");
  
  constexpr auto L2 = bar(3);
  static_assert(L2 == 3, "");
  static_assert(foo_ref(3) == 12, "");
  
}

} //end ns1

namespace ns2 {
  constexpr auto foo(int n) {
    auto L = [&] {
      return [&] {
        return [&] {
          return ++n;
        };
      };
    };
    n = n + n;
    return L()()();
  }
static_assert(foo(3) == 7, "");
static_assert(foo(5) == 11, "");

} // end ns2

namespace ns3 {
  constexpr auto fooN1(int n) {
    auto L = [&] {
      return [=] () mutable {
        return [&] {
          return ++n;
        };
      };
    };
    ++n;
    auto M1 = L();
    n *= 2;
    auto M2 = L();
    auto N1 = M1();
    auto N11 = M1();
    return N11(), N1();
  }
  
  constexpr auto fooN11(int n) {
    auto L = [&] {
      return [=] () mutable {
        return [&] {
          return ++n;
        };
      };
    };
    ++n;
    auto M1 = L();
    n *= 2;
    auto M2 = L();
    auto N1 = M1();
    auto N11 = M1();
    return N11(), N1(), N11();
  }
  
static_assert(fooN1(3) == 6, "");
static_assert(fooN11(5) == 9, "");


  constexpr auto fooN112(int n) {
    auto L = [&] {
      ++n;
      return [=] () mutable {
        ++n;
        return [&] {
          return ++n;
        };
      };
    };
    
    auto M1 = L();
   
    auto M2 = L();
    auto N1 = M1();
    auto N11 = M1();
    N11(), N1(), N11();
    return n;
  }
static_assert(fooN112(4) == 6, "");


  constexpr auto foo3(int n) {
    auto L = [&] {
      ++n;
      return [=] () mutable {
        ++n;
        return [&] {
          return ++n;
        };
      };
    };
    
    auto M1 = L();
   
    auto M2 = L();
    auto N1 = M1();
    auto N11 = M1();
    N11(), N1(), N11();
    return n;
  }
static_assert(fooN112(4) == 6, "");


} // end ns3


} // end test_captures_more::nested_captures

} // end test_captures_more

namespace test_return_lambdas_contained_in_structs {
  namespace ns1 {
  constexpr auto fooObj(int n) {
    auto L = [n] { return n; };
    struct A { decltype(L) l; } a = { L };
    return a;
  }

  constexpr auto L2 = fooObj(4);
  static_assert(L2.l() == 4, "");
  } //end ns1
} //end test_return_lambdas_contained_in_structs

namespace test_lambdas_within_templates {
  namespace ns1 {
    template<class T>
    constexpr auto fooObj(T n) {
      auto L = [n] { return n; };
      struct A { decltype(L) l; } a = { L };
      return a;
    }

    constexpr auto L2 = fooObj(4);
    static_assert(L2.l() == 4, "");
  } //end ns1
  
} //end test_lambdas_within_templates

namespace test_explicit_constexpr {
namespace ns1 {
  // The closure is a literal - even though the call operator isn't.
  constexpr auto L = [] { static int I; return 0; };
  int I = L();
} //end ns1

namespace ns2 {
  auto L = []() constexpr { static int I; return 0; }; //expected-error{{not permitted in a constexpr function}}
  
}
} //end test_explicit_constexpr

namespace test_default_template_arguments {
namespace ns1 {
template<int I = ([]{ return 5; })()> constexpr int f() { return I; }
static_assert(f() == 5, "");
} //end ns1
} // end test_default_template_arguments
