// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only %s -fdelayed-template-parsing -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s

template<class, class> struct is_same { enum { value = 0 }; };
template<class T> struct is_same<T, T> { enum { value = 1 }; };

namespace cannot_determine_common_type {

struct B;
struct C;

struct A {
  int a;
};
A ga;


struct B {
  int b;
};
B gb;

struct C {
  int c;
  operator B&() const;
};
C gc;

C::operator B&() const { return gb; }

auto fv159() {  
  return ga;
  return gc; //expected-error{{'auto' in return type deduced as}} 
  return gb;
}
}


namespace cannot_determine_common_type2 {

struct B;
struct C;

struct A {
  int a;
};

struct B {
  int b;
};

struct C {
  int c;
  operator A&() const;
  operator B&() const;
};

A ga;
B gb;
C gc;

C::operator A&() const { return ga; }
C::operator B&() const { return gb; }

auto fv159() {
  
  return ga;
  return gc; //expected-error{{'auto' in return type deduced as}} 
  return gb;
}

}

namespace order_dependent_common_type {
struct B;
struct C;
struct A {
  int a;
  operator B&() const;
} ga;
struct B {
  int b;
 // operator A&() const { return ga; }
};
struct C {
  int c;
  //operator A&() const;
  operator B&() const;
};
B gb;
C gc;

A::operator B&() const { return gb; }
//C::operator A&() const { return ga; }
C::operator B&() const { return gb; }

auto fv159() {
  
  return gb;
  return gc;
  return ga;
 

  return A{};
}
static_assert(is_same<decltype(fv159()), B>::value, "");
}

namespace order_dependent_common_type2 {
struct B;
struct C;
struct A {
  int a;
  operator B&() const;
} ga;
struct B {
  int b;
 // operator A&() const { return ga; }
};
struct C {
  int c;
  //operator A&() const;
  operator B&() const;
};
B gb;
C gc;

A::operator B&() const { return gb; }
//C::operator A&() const { return ga; }
C::operator B&() const { return gb; }

auto fv159() {  
  return ga;
  return gc;
  return gb;
  return ga;

}

auto& fv159_144() {  
  return ga;
  return gc;
  return gb;
  return ga;
  return A{};
}

}

namespace base_to_derived {
struct A { };
struct B : A { };
struct C : B { };
struct D : C { };

auto foo() {
  return nullptr;
  const D *d = nullptr;
  return 0;
  return d;
  return nullptr;
  return (C*) 0;
  return (B*) 0;
  return d;

  return (const D*) 0;
  return (A*) 0;
  return nullptr;
  return 0;
  return 0;
}
static_assert(is_same<decltype(foo()), const A*>::value, "");   



auto foo2() {
  return nullptr;
  const D *d = nullptr;
  return 0;
  return d;
  return nullptr;
  return (C*) 0;
  return (B*) 0;
  return d;

  return (const D*) 0;
  return (A*) 0;
  return nullptr;
  return (void*)0;
  return 0;
  return 0;
}
static_assert(is_same<decltype(foo2()), const void*>::value, "");   

/*
// We need to add an error message since we can not convert to unsigned char *
auto foo3() {
  return nullptr;
  const D *d = nullptr;
  return 0;
  return d;
  return nullptr;
  return (C*) 0;
  return (B*) 0;
  return d;

  return (const D*) 0;
  return (A*) 0;
  return nullptr;
  return (unsigned char*)0; 
  return 0;
  return 0;
}
static_assert(is_same<decltype(foo3()), const unsigned char*>::value, "");
*/
}

namespace nullptr_conversions {
auto foo_nullptr() {
  return 0;
  return nullptr;
  return 0;
  return nullptr;
}
static_assert(is_same<decltype(foo_nullptr()), decltype(nullptr)>::value, "");   
namespace ns1 {
auto *foo_nullptr() {
  return nullptr; //expected-error {{cannot deduce}}
}
auto *foo2() {
  return nullptr;
  return (void*)0;
}
static_assert(is_same<decltype(foo2()), void*>::value, "");   
} // end ns1
}

namespace lvalue_to_rvalue_and_beyond {
struct A { };
struct B : A { };
struct C : B { };
struct D : C { };

template<class T> auto&& rref(T&& t) { return static_cast<T&&>(t); }

auto&& fooR() {
  static C c;
  return c;
}

static_assert(is_same<decltype(fooR()), C&>::value, "");   

auto&& foo() {
  return A(); //expected-warning{{reference to local}}
  return B(); //expected-warning{{reference to local}}
  static C c;
  return c; //expected-warning{{reference to local}}
}
static_assert(is_same<decltype(foo()), A&&>::value, "");   

namespace ns1 {
  struct X {
    int d;
  };
  X&& getXRRef();
  static_assert(is_same<decltype((getXRRef().d)), int&&>::value, "");   
} // end ns1
namespace ns2 {
  struct Y;
  extern Y &gy;
  struct X { 
    operator Y& () { return gy; }  
  };
  struct Y { 
    Y() = default; 
    Y(X) { } 
  } gy_;
  Y &gy = gy_;
  auto& foo() {
    X x;
    Y y;
    return x;
    return y; //expected-warning{{reference to stack}}
    return X{};
  }
  static_assert(is_same<decltype(foo()), Y&>::value, "");   
  
  auto& foo2() {
    X x;
    Y y;
    return x;
    return Y{}; //expected-error{{deduced as}}
    return X{};
  }
  
  static_assert(is_same<decltype(foo2()), Y&>::value, "");   
}
}

namespace recursion_checks {


struct Y;
extern Y &gy;
struct X { 
  operator Y& () { return gy; }  
};

struct Y { 
  Y() = default; 
  Y(X) { } 
} gy_;

Y &gy = gy_;

auto& foo() {
  X x;
  Y y;
  return x;
  return y; //expected-warning{{reference to stack}}
  return X{};
  return foo();
}

auto foo2L = [](auto self) {
  return 'a';
  return 1;
  return true;
  decltype(self(self)) b;
  return 4;
  return 'a';
  return false;
  return short{};
  using uint = unsigned int;
  return uint{};
};

static_assert(is_same<decltype(foo2L(foo2L)), int>::value, "");

auto foo2() {
  return 'a';
  return 1;
  return true;
  decltype(foo2()) b;
  return 4;
  return 'a';
  return false;
  return short{};
  using uint = unsigned int;
  return uint{};
}

auto foo3() {
  return 'a';
  return 1;
  return true;
  decltype(foo3()) b;
  return 4;
  return 'a';
  return 1.0; //expected-error{{but deduced as}}
}

auto foo3L = [](auto self) {
  return 'a';
  return 1;
  return true;
  decltype(self(self)) b;
  return 4;
  return 'a';
  return false;
  return short{};
  using uint = unsigned int;
  return uint{};
  return 1.0; //expected-error{{but deduced as}}
};

static_assert(is_same<decltype(foo3L(foo3L)), int>::value, ""); //expected-note{{in instantiation of}}


}