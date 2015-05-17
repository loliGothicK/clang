// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING

namespace require_constant_expression_in_static_if {
namespace ns1 {
void foo(int n) { //expected-note{{declared here}}
  static_if( n == 3 ) { //expected-error{{not an integral constant expression}}\
                          expected-note{{read of non-const variable}}
  }
}
} // end ns1


namespace ns2 {
void foo(int n) { 
  static_if( true ) { 
  
  } else if( n == 1) {  // this is ok - not a static if
                          
  }
}
} // end ns2

}

namespace test_parsing_and_nondependent_namelookup_regardless_of_instantiation {
void foo(int n) {
  static_if(true) {
    ++n;
  } else {
    x(); //expected-error{{use of undeclared identifier}}
  }
}
}


namespace test_only_right_branch_instantiated {

namespace ns1 {
struct X1 {
  using type1 = int;
  enum { value = 1 };
};

template<int C, class T>
void f(T n) {
  static_if (T::value == 1) {
    typename T::type1 t;
  } else static_if (T::value == 2) {
    typename T::type2 t;
  } else {
    typename T::type3 t;
  }  
}


template<int C, class T>
void f2(T n) {
  static_if (T::value == 1) {
    typename T::type1 t;
  } else if (T::value == 2) {
    typename T::type2 t;  //expected-error{{no type named 'type2'}}
  } else {
    typename T::type3 t;
  }  
}

void main() {
  f<1>(X1{});
  struct X3 { using type3 = int; enum { value = 3 }; };
  f2<3>(X3{}); //expected-note{{in instantiation}}
}

} //end ns1 


namespace ns2 {
template<int N>
struct X {
  enum { value = N };
  
};

template<int C, class T>
void f(T n) {
  static_if (T::value == C) {
    static_assert(T::value == C, "");
  } else {
    static_assert(T::value != C, "");
  }  
}

void main() {
  f<1>(X<1>{});
  f<1>(X<2>{});
}

} //end ns2

namespace ns3 {
template<int N>
struct X {
  enum { value = N };
  
};

template<int C, class T>
void f(T n) {
  static_if (T::value == C) {
    static_assert(T::value == C, "");
    does_not_exist();  
#ifndef DELAYED_TEMPLATE_PARSING
    //expected-error@-2{{use of undeclared identifier}}
#endif
  } else {
    static_assert(T::value != C, "");
  }  
}


} //end ns3
namespace ns4 {

struct X {
  constexpr operator bool () { return true; }
  void operator ++() { }
};

template<class T>
void f(T t) {

  static_if (t) {
    ++t;
  }

}


template<class T>
void g(T t) {  //expected-note{{declared here}}

  static_if (t) {   //expected-error{{not an integral constant expression}}\
                          expected-note{{read of non-const variable}}
    ++t;
  }

}

void main() {
  f(X{});
  g(3); //expected-note{{instantiation}}
}

} //end ns4

} //end ns test_only_right_branch_instantiated

namespace test_condition_var {
constexpr int X = 5; 
namespace ns1 {
void foo() {
  static_if(const int B = X) {

  } else {
    if(!B) { }
  }
}
} //end ns1

namespace ns2 {
struct X {
  constexpr operator bool() { return false; }
} X;
void foo() {
  static_if(const int B = X) {

  } else {
    if(!B) { }
  }
}
} //end ns2

namespace ns3 {

struct X {
  constexpr operator bool () { return true; }
  void operator ++() { }
};

template<class T>
void f(T t) {

  static_if (const bool B = t) {
    ++t;
  }

}


template<class T>
void g(T t) {  

  static_if (const bool B = t) {   //expected-error{{not an integral constant expression}}\
                          expected-note{{initializer}} \
                          expected-note{{declared here}}
    ++t;
  }

}

void main() {
  f(X{});
  g(3); //expected-note{{instantiation}}
}

} //end ns3
} //end ns test_condition_var

namespace constexpr_func_tests {

namespace ns1 {
struct X {
  constexpr operator int () { return 5; }
  void operator ++() { }
};
constexpr int fac(int n) {
  if (n >= 0) return fac(n - 1) * n;
  return n;
}

template<class T>
void f(T t) {

  static_if (const bool B = fac(t)) {
    ++t;
  }

}

constexpr int foor(int n) { //expected-note{{declared here}}
  static_if(foor(n - 1)) { //expected-error{{not an integral constant expression}} expected-note{{undefined}}
    return 1;
  }
  return 0;
}

void main() {
  f(X{});
}


}

} // end ns constexpr_func_tests