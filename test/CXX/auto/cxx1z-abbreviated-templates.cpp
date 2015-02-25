// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING

template<class T> struct X { };
namespace multi_declarators {
namespace ns1 {
  auto f(auto = 1), g(int), h(auto* = nullptr, auto = 4); 
  auto f(auto) { }
  auto h(auto*, auto) { }
  decltype(f<double>())*vp;
  decltype(h<double, float>())*vp2;
} // end ns1

namespace ns2 {
  auto f(auto = 1), g(int), h(auto* = nullptr); //expected-note{{previous}}
  auto f(auto) { }
  auto h(auto* = nullptr) { } //expected-error{{redefinition}}
  
} // end ns2
} // end ns multi_declarators

namespace template_not_allowed_at_block_scope {

void foo() {
  auto f(int);
}
void foo2() {
  void f(auto); //expected-error {{templates can only be declared}}
}
} //end ns template_not_allowed_at_block_scope

namespace default_args {
namespace ns0 {
void f(auto a = decltype(a){}) {}
void g(auto a, auto b, auto c = decltype(a + b + c){});
int main() {
  f<int>();
  g<int,char,double>(3, 'a');
}
}
namespace ns1 {
void g(auto a = decltype(a){}, decltype(a) (*fp)(decltype(a)) = [](auto b) {  return b; });

template<class T> void g(T t, decltype(t) (*fp)(decltype(t))) { fp(t); }

int main() {
  g<int>();
}

} // ends ns1

namespace ns2 {
void g(int a = decltype(a){}, decltype(a) (*fp)(decltype(a)) = [](auto b) {  return b; }, auto c = 3);

template<class T> void g(int a, decltype(a) (*fp)(decltype(a)), T t) { fp(t); }

int main() {
  g<double>();
}

}

namespace ns3 {
auto L = [](int (*fp)(int) = [](auto a) { return a; }, auto b = "abc") { return fp(b[0]); };
int main() {
  L.operator()<const char*>();
}

}
namespace ns4 {
void f(auto a, int (*)(int, decltype(a)) = [](auto b, decltype(a)) { return b;});

void f(auto a, int (*fp)(int, decltype(a))) {
  fp(3,a);
}

int main() {
 f(3.14);
}
}

namespace ns5 {
void f(int a, int (*)(int, decltype(a)) = [](auto b, decltype(a)) { return b;}, auto c = decltype(c){});

void f(int a, int (*fp)(int, decltype(a)), auto c) {
  fp(3,a);
}

int main() {
 f<double>(3);
}
}
namespace class_ns0 {
template<class T> struct X {
  void f(auto a = decltype(a){}) { }
};
void foo() {
  X<double>().f<int>();
} 
} // end class_ns0

namespace class_ns1 {
template<class T> struct X {
  void f(auto a = decltype(a){}) { }
};

template<class T> template<class U> 
void X<T>::f(U u);  //expected-error{{must be a definition}}

void foo() {
  X<double>().f<int>();
}
}//end class_ns1
namespace class_ns2 {

template<class T> 
struct X {
  void f(int (*fp)(int) = [](auto a) { return a; }, auto b = decltype(b){}, T = T()) {
    fp(3);
    fp(b);
  }  
  void g(int (*fp)(int) = [](auto a) { return a; }, auto b = decltype(b){}, T t = T());
  void h(int (*fp)(int) = [](auto a) { return a; }, 
         auto b = decltype(b){}, T t = T(), auto c = decltype(c){});
};

template<class T> 
void X<T>::g(int (*fp)(int), auto b, T) {
  fp(b);
  fp(5);
}

template<class T> template<class U>
void X<T>::h(int (*fp)(int), U b, T, auto c) {
  fp(b);
  fp(5);
}

void foo() {
  X<double>().f<int>();
  X<double>().g<float>();
  X<double*>().h<float,double>();
  
}
} //end class_ns2
} //end ns default_args

namespace fun_redecls {
namespace ns0_0 {
void f(auto (*)(auto)->int) { } //expected-note{{previous}}
template<class T> void f(int (*)(T)) { } //expected-error{{redefinition}}
}

namespace ns0 {

void f(X<auto> x) { }

void eggs() {
  X<char> xc;
  f(xc);
}

namespace ns0_1 {
  
X<X<int>(*)(X<char>)> xc;
int f(X<X<auto>(*)(X<auto>)> x) { return 0; }
int i = f(xc);
  
namespace ns0_1_1 {

int* foo(auto (*)(auto (*)(auto (*)(auto, auto))));
int f(char p(double(int, float)));
int *ip = foo(f);

int* foox(auto(auto(auto(auto, auto))));
int *ipx = foox(f);

auto g(auto (auto*)->auto**) -> auto& { } //expected-note{{previous}}
template<class T, class U>
auto g(auto (T*)->U**) -> auto& { } //expected-error {{redefinition}}

} //ns0_1_1
} // ns0_1
} // end ns0

namespace ns1 {
  template<class T> void foo(T, auto = 0); //expected-note {{previous}}
  template<class T, class U> void foo(T, U = 3); //expected-error{{redefinition of default}}
  void foo(auto, auto);
} // end ns1

namespace ns2 {
void foo(auto i); //expected-note{{previous template declaration}}
template<class T> void foo(T = 4); //expected-error{{default arguments cannot be added}}
}

namespace ns3 {
  template<class T> auto foo(T, 
      auto (*)(auto (*)(auto)) = nullptr);  //expected-note{{previous}}
  template<class T0, class T1, class T2, class T3> auto foo(T0, 
      T1 (*)(T2 (*)(T3)) = nullptr); //expected-error{{redefinition of default}}
}

namespace ns4 {
  namespace ns4_1 {
    // these are not redeclarations.
    auto f(auto (auto) ->auto = 0);
    template<class T0, class T1> auto f(T0 (T1) = 0);
    
  }
  namespace ns4_2 {
    auto f(auto (auto)->auto* = 0) -> auto&; //expected-note{{previous}}
    template<class T0, class T1> auto f(T1* (T0) = 0) ->auto&; //expected-error{{redefinition of default}}
  }
  // template parameter order matters - a trailing auto is indexed after its parameters!
  template<class T> auto foo(T, 
      auto (*)(auto (*)(auto) ->auto*) ->auto = nullptr);  
  // not a redeclaration because the template parameters are indexed differently
  template<class T0, class T1, class T2, class T3> auto foo(T0, 
      T1 (*)(T2* (*)(T3)) = nullptr); 
  
}

namespace ns5 {
  // template parameter order matters - a trailing auto is indexed after its parameters!
  // These should be the same declarations
  template<class T> auto foo(T, 
      auto (*)(auto (*)(auto) ->auto*) ->auto = nullptr);  //expected-note{{previous}}
  // not a redeclaration because the template parameters are indexed differently
  template<class T0, class T1, class T2, class T3> auto foo(T0, 
      T3 (*)(T2* (*)(T1)) = nullptr); //expected-error{{redefinition of default}}
  
}



namespace add_to_tpl {
  template<int N, class T0, template<class> class TT, class T1> 
  void foo(T1, auto) { } //expected-note{{previous}}
  template<int N, class T0, template<class> class TT, class T1, class T2> 
  void foo(T1, T2) { } //expected-error{{redefinition}}
}

namespace cs0 {

struct Y {
  void f(auto);
};

template<class T>
void Y::f(T t) { }
} // end cs0

} //end ns fun_redecls

namespace friend_decls {
  
class Y {
  int m;
  friend auto f0(auto t) { return t.m; } //expected-note{{previous}}
};
template<class T>
auto f0(T t) { return t.m; } //expected-error{{redefinition}}

namespace ns1 {

class Y {
  int m;
  friend auto f(auto t) { return t.m; } 
};

int i = f(Y{});
}

namespace ns2 {

template<class T>
class Y {
  T m{};
  friend auto f2(auto t) { return [=](auto, auto) { return t.m; }; }
};

double *dp = f2(Y<double*>{})("a", 3.145);

}
} //end friend_decls

namespace nested_nns {
template<class T>
struct X { T m{}; };

auto f(auto auto::*p, auto x) { return x.*p; }


int i = f(&X<int>::m, X<int>{});
double *d = f(&X<double*>::m, X<double*>{});
namespace ns1 {
  template<class T>
  struct X { T m; };
  int X<auto>::*x2 = &X<int>::m; 

  void foo(int X<auto>::*); //expected-note {{candidate}}
  decltype(foo(&X<int>::m)) *vp0 = nullptr;
  void foo2(int auto::*x) { }
  decltype(foo2(&X<int>::m)) *vp0_1 = nullptr;
  decltype(foo(&X<char>::m)) *vp1 = nullptr; ////expected-error{{no matching}}
  
  void foov(auto X<auto>::* ... p);
  static_assert(sizeof(decltype(foov(&X<int>::m, &X<char*>::m, &X<double*>::m))*),"");
  

} //end ns1
}
namespace dont_abbreviate_for_auto_in_return_type {

auto f() -> auto*{
  return (int *)nullptr;
}
int *ip = f();

} // end ns dont_abbreviate_for_auto_in_return_type

namespace generic_lambda_tests {

 void foo() {
  auto L = [](auto a) -> decltype(decltype(a)::f()){
     return decltype(a)::f();
  };
  //L(3);
  struct A { static void f() { } };
  L(A{});
 }

 namespace ns1 {
  auto f(auto t) { 
    auto L = [=](auto x) { return sizeof(t) + t; };
    return L;
  }
  auto d = f(2.14)("abc");
 }
 namespace ns2 {
  template<class T>
  auto f(auto t) { 
    auto L = [=](auto x, T) { return sizeof(t) + t; };
    return L;
  }
  auto d = f<char**>(2.14)("abc",(char**)nullptr);
 }

} //end ns generic_lambda_tests

namespace variadic_tests {
template<class> struct V1 { };
template<class, class> struct V2 { };
template<class...> struct VV { };

template<class R, class ... Ps> using FunPtrType = R(*)(Ps...);

int foo(X<auto> ... x2) { return 0; }
int x = foo(X<char>{}, X<int>{}, X<float>{});
namespace ns0 {
template<class ... Ts>
int f(V1<Ts (*)(auto)>...v1) { return 0; }

//template<class R, class P> using FunPtrType = R (*)(P);

int v = f(V1<FunPtrType<int,char>>{},V1<FunPtrType<float,int>>{},V1<FunPtrType<double,float>>{});

template<class ... Ts>
int f2(V1<auto (*)(auto) -> Ts>...v1) { return 0; }
int v2 = f2(V1<FunPtrType<int,char>>{},V1<FunPtrType<float,int>>{},V1<FunPtrType<double,float>>{});

} // end sub variadic_tests::ns0

namespace ns1 {

int foo(VV<V1<auto>...> vv) { return 0; }
int x = foo(VV<V1<char>, V1<int>, V1<float>>{});

} //end sub variadic_tests::ns1

namespace ns2 {

int foo(int (*...vp)(auto)) { return 0; }
int x = foo(FunPtrType<int, char>{},FunPtrType<int, double>{},FunPtrType<int, float>{});

} //end sub ns2

namespace ns3 {

template<class ... Ts>
int foo(VV<auto, VV<V2<auto...>, V1<auto>...>, VV<Ts (*)(auto)...>> ... vs) { return 0; }

int x0 = foo(VV<int, VV<V2<char, float*>,V1<int*>, V1<char*>, V1<double>>,
              VV<FunPtrType<int, char>,FunPtrType<float, double>,FunPtrType<char, float*>>>{},
             VV<char, VV<V2<char, float*>,V1<int*>, V1<char*>, V1<double>>,
              VV<FunPtrType<int, char>,FunPtrType<float, double>,FunPtrType<char, float*>>>{},
             VV<float*, VV<V2<char, float*>,V1<int*>, V1<char*>, V1<double>>,
              VV<FunPtrType<int, char>,FunPtrType<float, double>,FunPtrType<char, float*>>>{}
); 

template<class ... Ts>
int foo(VV<Ts (*)(auto)...> vs) { return 0; }
int x = foo(VV<FunPtrType<int, char>,FunPtrType<float, double>,FunPtrType<char, float*>>{});

} //end sub ns3

namespace ns4 {
template<class ... Ts> void foo(VV<Ts,auto>...p); // Is the auto a parameterpacktype
decltype(foo(VV<int, char>{}, VV<float, double>{}))*vp = nullptr;
namespace ns4_1 {
template<class> struct X { };
template<class, class> struct X2 { };

int *foo(auto... a) { return nullptr; }
int *ip = foo(X<int>{}, char{}, float{}, double{}, "abc");

int *foop(auto* ... a);
int *ip1 = foop("abc", (int*)0, (char*)0, (double*)0);

int *foox(X<auto*> ... a);
int *ip1x = foox(X<char*>{}, X<int*>{});

void foon(X2<auto...> x2);
decltype(foon(X2<char,double>{})) *vp = nullptr;

}
}//end ns4

namespace ns5 {
int foo(int (*...vp)(auto)) { return 0; } //expected-note{{candidate}}
int foox(auto (*...vp)(auto)) { return 0; }

template<class R, class T> R f(T) { return R{}; }
int x = foo(f<int, char>, f<int, double>);
int x1 = foo(f<int, char>, f<char, double>); //expected-error{{no matching}}

int x0 = foox(f<int, char>, f<char, double>);

} //end ns5
namespace ns6 {
template<class ... > struct P { };
P<char*, double*> bar(float, P<short*, char**> (*)(int*), P<float***, char**, P<short>> (*)(double**, int**, short));
auto f(auto (*)(auto, auto (*)(auto)->P<auto, auto>, auto (*)(auto...)->P<auto...>) -> P<auto...>) { return; }

static_assert(sizeof(decltype(f(bar))*), "");
namespace ns6_1 {
P<char*, double*> bar2(float, P<short*, char**> (*)(int*), P<float***, char**, P<short>> (*)(double**, int**, short), 
            P<int, char**>*, P<float&, double*>&);

template<class T> struct X {
  template<class U> struct Y {
    auto f(auto (*)(auto, auto (*)(auto)->P<auto, auto>, auto (*)(auto...)->P<auto...>) -> P<auto...>, T, U) { return; }
  };
};
static_assert(sizeof(decltype(X<P<int, char**>&&>::Y<P<float&, double*>&&>{}.f(bar, P<int, char**>{}, P<float&, double*>{}))*), "");
} //end ns6_1
} //end ns6
} //end variadic_tests
