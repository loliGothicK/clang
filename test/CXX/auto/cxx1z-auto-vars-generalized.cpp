// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING

template<class, class> struct is_same_type { enum { value = 0 }; };
template<class T> struct is_same_type<T, T> { enum { value = 1 }; };

namespace std {
  typedef decltype(sizeof(int)) size_t;

  // libc++'s implementation
  template <class _E>
  class initializer_list
  {
    const _E* __begin_;
    size_t    __size_;

    initializer_list(const _E* __b, size_t __s)
      : __begin_(__b),
        __size_(__s)
    {}

  public:
    typedef _E        value_type;
    typedef const _E& reference;
    typedef const _E& const_reference;
    typedef size_t    size_type;

    typedef const _E* iterator;
    typedef const _E* const_iterator;

    initializer_list() : __begin_(nullptr), __size_(0) {}

    size_t    size()  const {return __size_;}
    const _E* begin() const {return __begin_;}
    const _E* end()   const {return __begin_ + __size_;}
  };
}


namespace std_init_list_deduction {

template<class,class> struct X { };

void foo() {
  auto x0 = {X<int*,char**>{}, X<int*, char**>{}};
  static_assert(is_same_type<decltype(x0), std::initializer_list<X<int*,char**>>>::value, "");
  auto &&x01 = {X<int*,char**>{}, X<int*, char**>{}};
  static_assert(is_same_type<decltype(x01), std::initializer_list<X<int*,char**>>&&>::value, "");
  
  std::initializer_list<auto> x1 = {1, 2, 3}; 
  static_assert(is_same_type<decltype(x1), std::initializer_list<int>>::value, "");
  
  std::initializer_list<auto> x2 = {X<int,char>{}, X<int, char>{}};
  static_assert(is_same_type<decltype(x2), std::initializer_list<X<int,char>>>::value, "");
  std::initializer_list<X<auto,auto>> x3 = {X<int,char>{}, X<int, char>{}};
  static_assert(is_same_type<decltype(x3), std::initializer_list<X<int,char>>>::value, "");
  std::initializer_list<X<auto...>> x4 = {X<float,char>{}, X<float, char>{}};
  static_assert(is_same_type<decltype(x4), std::initializer_list<X<float,char>>>::value, "");
  std::initializer_list<X<auto...>> x5 = {X<float,char>{}, X<float, int>{}}; //expected-error {{cannot deduce}}
}


}

namespace fun_deduction {
void foo(int, char*);

auto (*fp1)(int, auto*) = foo;
auto (*fp2)(auto ...) = foo;
auto (*fp3)(int, auto ...) = foo;
void (*fp4)(int, auto ...) = foo;
void (*fp5)(auto, char*, auto ...) = foo;

auto (&rfp1)(int, auto*) = foo;
auto (&rfp2)(auto ...) = foo;
auto (&rfp3)(int, auto ...) = foo;
void (&rfp4)(int, auto ...) = foo;
void (&rfp5)(auto, char*, auto ...) = foo;
namespace ns1 {
  template<class> struct X { };
  
  void f(X<X<char>>);
  void (*fp)(X<X<auto>>) = f;
}
namespace ns2 {

void foo(char, char);
auto (*fp)(auto a, decltype(a)) = foo;

}
namespace add_auto_post_variadic_fppack {
  int* foo(int, float*) { return 0; }
  auto (*fp)(int, auto...) -> auto* = foo;
  namespace empty_fun_variadic {
    void foo();
    auto (*fp)(auto ...) = foo;
  } //end ns empty_fun_variadic
  namespace ns1 {
    template<class ... Ts> struct V { };
    V<V<auto ...>, V<auto...>> v = V<V<>,V<char, double>>{};
    V<V<auto ...>, V<auto...>, V<auto>...> v2 = V<V<>,V<char, double>,V<int*>,V<char*>>{};
  }
}

}

namespace nested_namespace_specifier_memfun_deduction {

struct X {
  void foo(int, char*);
};

auto (auto::*mfp1)(auto...) = &X::foo;

namespace ns1 {
struct X {
  //void foo();
  void foo(int);
  X foo(int*, char);
  void foo(char);
  int m;
  int a[10];
};
auto (auto::*mpf2)(int*, auto ...) = &X::foo;
void (auto::*mfp1)(char, auto...) = &X::foo;

auto auto::* x = &X::m;
auto auto::* a = &X::a;

int auto::x2 = &X::m; //expected-error{{nested name specifier}}

auto auto::x3 = &X::m; //expected-error{{nested name specifier}}
auto auto::auto::x4 = &X::m; //expected-error{{expected unqualified-id}}

namespace array_deduction {

int a[10];

auto (*ap)[10] = &a;
auto (auto::*ap2)[10] = &X::a;


}

} // end ns1


namespace ns2 {
template<class T> struct X { T t; T f(T); };
auto X<auto...>::*ip147 = &X<int>::t;
auto X<auto>::*ip = &X<int>::t;
auto (X<auto>::*fp)(auto) = &X<int>::f;
auto (X<auto>::*fp2)(int) = &X<int>::f;
auto (X<auto>::*fp3)(char) = &X<int>::f; //expected-error{{incompatible initializer}}

struct Y {
  int X<auto>::*ip = &X<int>::t;           //expected-error{{not allowed}}
  int (X<int>::*fp)(auto) = &X<int>::f;    //expected-error{{not allowed}}
  int (X<auto>::*fp2)(int) = &X<int>::f;   //expected-error{{not allowed}}
};

namespace no_nsdmi_deduction {
template<class T>
struct X { T m; };
X<int> xi{};
struct S {
  constexpr static X<auto> *const sp = &xi;
  X<auto> *const p = &xi; //expected-error{{not allowed}}
};
} // end ns no_nsdmi_deduction

} // end ns2
namespace ns3 {
template<class R, class T>
R f(T t) { return R{}; }

template<class R, class ... Ts>
R fv(Ts ... t) { return R{}; }

auto (*(fun()))(auto) { return f<int, char>; }
auto (*(funv()))(auto...) { return fv<int, char, double, float*>; }


struct Y {
auto (*(mem_fun()))(auto) { return fun(); }
auto (*(mem_vfun()))(auto...) { return funv(); }

auto (*(Y::*(mem_ptr_fun()))())(auto) { return &Y::mem_fun; }

auto (*(Y::*(mem_ptr_fun2()))())(auto...) { return &Y::mem_vfun; }
auto (Y::*(mem_ptr_funv()))(auto...) { return &Y::mem_vfun; }

auto (*(auto::*(mem_ptr_fun3()))())(auto) { return &Y::mem_fun; }
auto (auto::*(mem_ptr_funv3()))(auto...) { return &Y::mem_vfun; }

auto (*(fun_array()))[10] { static int arr[10]; return &arr; }
auto (*(fun_array2()))[10] { static char *arr[10]; return &arr; }
  int marr1[10];
  char *marr2[10];
auto (auto::*(mem_fun_array()))[10] { static int arr[10]; return &Y::marr1; }
auto (auto::*(mem_fun_array2()))[10] { static char *arr[10]; return &Y::marr2; }

//auto (auto::*(mem_ptr_funv2()))(auto...) { return &Y::mem_vfun; }


};
} // end ns3'
} // end ns nested_namespace_specifier_memfun_deduction


namespace variadic_deduction {
  template<class ...> struct V { };
  template<class> struct P1 { };
  template<class, class> struct P2 { };
  namespace ns1 {
    int f(V<P1<char>, P2<char, double>, P2<float, int>, P2<short, int*>> v);
    auto (*vp)(V<P1<auto>, P2<auto, auto>...>) = f;
    static_assert(is_same_type<decltype(vp), decltype(&f)>::value, "");
    
    decltype(f)* fv(V<P1<char>, P2<char, double>, P2<float, int>, P2<short, int*>> v0, V<P1<char*>, P2<char, double>, P2<float, int>, P2<short, int*>> v1);

    auto (*vp2)(V<P1<auto>, P2<auto, auto>...> ... x) = fv;
    auto (*vp2_0)(V<P1<auto>, P2<auto, auto>...> ... ) = fv;
 
    decltype(f)* fv3(V<P1<char>, P2<char, double>, P2<float, int>, P2<short, int*>> v0, V<P1<char*>, P2<char******, double>, P2<float, int>, P2<short, int*>> v1);    
    auto (*vp3)(V<P1<auto>, P2<auto, auto>...> ... x) = fv3; //expected-error{{incompatible}}
  }
  namespace ns2 {
  template<class ... Ts> struct V { };

  template<class T> void f(T t) {
    V<auto...> v = V<T>{};
  }
  int main() {
    f(3.0);
  }
  } //end ns2
  
}


namespace class_template_deduction {
template<class T, class U = char> struct P { };

P<auto, auto> p = P<int, char>{};
P<auto> q = P<int, char>{};
P<auto> q1 = P<int, float>{}; //expected-error{{incompatible initializer}}
P<auto...> q2 = P<int, float>{};
P<P<auto>,P<auto>> pp = P<P<int>, P<float>>{};
P<P<auto...>,P<auto>> pp2 = P<P<int>, P<float>>{};
P<P<auto...>,P<auto...>> pp3 = P<P<int,short>, P<float,double>>{};

template<class T> using alias = int;

alias<auto> a = alias<char>{}; //expected-error{{incompatible}}


} //end ns class_template_deduction


namespace auto_return {
template<class T, class U = char> struct P { };
template<class ...> struct V { };
  P<auto, auto> f() { return P<char, float>{}; }
  P<auto> f2() { return P<float, char>{}; }
  P<auto...> f3() { return P<float, char>{}; }
  
  V<P<auto...>,P<auto>...> f4() { return V<P<float, char>,P<double*>, P<int*>>{}; }
  namespace ns1 {
    template<class...> struct X { };

    X<X<auto>...> foo() { return X<X<int>, X<char>>{}; }
    namespace ns2 {
      template<class...> struct X { };
      X<X<int>, X<char>, X<double*>> f();
      X<X<int>, X<char>, X<double*, char>> g();
      X<X<auto>...> (*foo)() = f;
      
      X<X<auto...>...> (*foo3)() = g; //expected-error{{does not contain any unexpanded}}
      //template<class ... Ts, template<class ...> class ... TTs> void bar(X<TTs<Ts...>...> (*ts)()) { }
      //decltype(bar(g))* vp = nullptr;
      X<X<auto>...> (*foo2)() = g; //expected-error{{incompatible}}
    }
  }
}

namespace variable_templates {
template<class T, class U>
void foo(T t, U u) {  }

template<class T, class U>
auto (*fp)(auto, auto) = foo<T, U>;

decltype(fp<int, const char*>(3, "hello"))* vp = nullptr;

template<class T> struct X { T m; };

template<class T>
auto auto::*mp = &X<T>::m;

decltype(mp<char*>) *mp2 = nullptr;
namespace ns1 {
template<class ... Ts> struct V { };

template<class T, class U, class ... Ts>
V<V<auto ...>, V<auto...>, V<auto>...> v = V<V<>,V<char, double, Ts...>, V<T>, V<U>, V<Ts>...>{};
decltype(v<int, char, float, double*, short*, bool*>) *ip0;

template<class T> void ft(char*, int*, T);
template<class T> auto (*fp)(auto, auto, T) = ft<T>;
decltype(fp<int***>)*ip;
}
}

namespace ill_formed_deduction_ctx {

using fun_ptr = auto (); //expected-error{{not allowed}}

template<class T> using fun_ptr_t = auto (T); //expected-error{{not allowed}}

typedef void foo(auto); //expected-error{{not allowed}}


void foo(int);
struct Y {
  void (*fpy)(auto) = foo; //expected-error{{not allowed}}
};

namespace ns1 {
template<class T> struct X {
  void (*p)(auto) = foo; //expected-error{{not allowed}}
};

}

namespace ns2 {

template<class T> struct X { T t; };

struct Y {
  X<auto> (*foo)() = (X<int> (*)())nullptr; //expected-error {{not allowed}}
  X<auto> x = X<int>{}; //expected-error{{not allowed}}
  X<auto> *p = (X<int>*>)nullptr;  //expected-error{{not allowed}}
};

struct Y2 {
  X<auto> foo();
  X<auto*> foo2();
  X<auto...> foo3();
};
} //end ill_formed_deduction_ctx::ns2
} //end ill_formed_deduction_ctx



