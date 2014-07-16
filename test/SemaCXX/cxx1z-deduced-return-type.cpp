// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only %s -fdelayed-template-parsing -DDELAYED_TEMPLATE_PARSING

namespace cannot_determine_common_type {
struct B;
struct C;
struct A {
  int a;
  //operator B&() const;
};
struct B {
  int b;
};
struct C {
  int c;
  //operator A&() const;
  operator B&() const;
};
A ga;
B gb;
C gc;

//A::operator B&() const { return gb; }
//C::operator A&() const { return ga; }
C::operator B&() const { return gb; }

auto fv159() {
  
  return ga;
  return gc;
  return gb;
  //return fv();

  //return A{};
}
//*/
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
  
  //return fv();

  //return A{};
}
//*/
}

