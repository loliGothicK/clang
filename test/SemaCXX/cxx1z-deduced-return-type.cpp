// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only %s -fdelayed-template-parsing -DDELAYED_TEMPLATE_PARSING

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