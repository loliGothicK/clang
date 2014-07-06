// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING

template<class, class> struct is_same_type { enum { value = 0 }; };
template<class T> struct is_same_type<T, T> { enum { value = 1 }; };
namespace check_basic_lookup {
  struct X {
    using auto = zzz; //expected-error{{unknown type name}} 
  };
}

template <typename T>
struct noeval_type {
  const T& ref;
  noeval_type(const T& ref) : ref(ref) {}
  operator T const&() { return ref; }
  using auto = T const&;
};

template <typename T>
noeval_type<T> noeval(const T& ref) { 
  return ref;
}

namespace check_works_as_var_decl {
  struct X {
    using auto = double;
    operator double() const { return 0.0; }
  };
  int test() {
    auto d = X{};
    static_assert(is_same_type<double, decltype(d)>::value, "");
    auto noeval_d = noeval(X{});
    static_assert(is_same_type<const X&, decltype(noeval_d)>::value, "");
    
    return 0;
  }
  template<class U, class T> int foo(T t) {
    auto d = t;
    static_assert(is_same_type<U, decltype(d)>::value, ""); // expected-error{{failed}}
    return 0;
  }
  
  int x = foo<double>(X{}); //OK
  
  int x2 = foo<X>(X{}); //expected-note{{in instantiation}}

} //end check_works_as_var_decl

namespace check_uses_conversion_ctor {
  struct Y;
  struct X {
    using auto = Y;
  };
  struct Y {
    Y(X) { }
  };
  void isY(Y&) { }
  void test() {
    auto y = X{};
    isY(y);
  }
}


namespace check_works_as_var_decl_in_template {
  template<class T>
  struct X {
    using auto = T;
    operator T() const { return T{}; }
  };
  
  template<class U, class T> int foo(T t) {
    auto d = t;
    static_assert(is_same_type<U, decltype(d)>::value, ""); // expected-error{{failed}}
    return 0;
  }
  auto x44 = X<char>{};
  int x45 = foo<char>(X<char>{});   //OK
  int x = foo<double>(X<double>{}); //OK
    
  int x2 = foo<X<double>>(X<double>{}); //expected-note{{in instantiation}}
  int x49 = foo<double*>(X<double*>{});
  
} //end check_works_as_var_decl

namespace check_returntype_deduction {
  template<class T>
  struct X {
    T t;
    using auto = T;
    constexpr operator T() const { return t; }
    friend constexpr bool operator==(X<T> l, X<T> r) { return l.t == r.t; }
  };

  template<class U>
  constexpr auto foo(U u) { return X<U>{u}; }
  
  void test() {
    static_assert(foo(3) == 3, "");
    static_assert(foo(X<int>{42}) == X<int>{42}, ""); 
    static_assert(foo(4.00) == 4.00, ""); 
     
  }  
}

namespace check_does_not_work_for_template_deduction_and_generic_lambdas {
  template<class T>
  struct X {
    T t;
    using auto = T;
    constexpr operator T() const { return t; }
    friend constexpr bool operator==(X<T> l, X<T> r) { return l.t == r.t; }
  };

  template<class U>
  constexpr U foo(U u) { 
    return u;
  }
  
  void test() {
    static_assert(foo(X<int>{42}) == X<int>{42}, ""); 
    auto L = [](auto x) {
      static_assert(is_same_type<decltype(x), X<double>>::value, "");
    };
    L(X<double>{});    
  }
}

namespace check_redecl {
struct X {
  using auto = double; // expected-note{{previous}}
  using auto = char;   // expected-error{{redeclaration}}
};
}

namespace check_currently_allowed_in_union {
  union U {
    using auto = char;
    char c = 'a';
    int i;
    double d;
    operator char() const { return c; }
  };
  void test() {
    auto c = U{};
    static_assert(is_same_type<char, decltype(c)>::value, "");
    
  }
  
}


namespace check_not_inherited {
  struct B {
    using auto = double;
    operator double() const { return 0.0; }
  };
  struct D : B {
  };  
  void test() {
    auto b = B{};
    auto d = D{};
    static_assert(is_same_type<double, decltype(b)>::value, "");
    static_assert(is_same_type<D, decltype(d)>::value, "");
  }
}

namespace check_obfuscating_deductions {

namespace deduce_to_base {
  struct B {
  };
  struct D : B {
    using auto = B&;
  };
  void test() {
    D d;
    auto d2 = d;
    static_assert(is_same_type<B&, decltype(d2)>::value, "");
  }
} // deduce_to_base

namespace deduce_to_rref {
  struct B {
    using auto = B&&;
  };
  void test() {
    auto b = B{};
    static_assert(is_same_type<B&&, decltype(b)>::value, "");
  }
}

}

