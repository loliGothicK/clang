// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -emit-llvm-only %s
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing %s -DDELAYED_TEMPLATE_PARSING
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fms-extensions %s -DMS_EXTENSIONS
// RUN: %clang_cc1 -std=c++1z -verify -fsyntax-only -fblocks -fdelayed-template-parsing -fms-extensions %s -DMS_EXTENSIONS -DDELAYED_TEMPLATE_PARSING
template<class T1, class T2> struct pair {
  T1 first;
  T2 second;
};

template<class T1, class T2> constexpr auto make_pair(T1 t1, T2 t2) {
  pair<T1, T2> p = { t1, t2 };
  return p;
}
namespace test_ref_capture {
namespace ns1 {
constexpr auto foo(int n) {
  int r = n;
  auto LX = [&] { return [&] { return ++r; }; };
  return LX;
}

constexpr auto L = foo(3);
constexpr auto M = L();
constexpr auto N = L();
static_assert(M() == 4, "");
static_assert(N() == 5, "");
static_assert(M() == 6, "");
} // end ns1

namespace ns2 {
constexpr auto foo(int n) {
  auto LX = [&] { return [&] { return ++n; }; };
  return LX;
}

constexpr auto L = foo(3);
constexpr auto M = L();
constexpr auto N = L();
static_assert(M() == 4, "");
static_assert(N() == 5, "");
static_assert(M() == 6, "");
} // end ns2

namespace ns3 {
constexpr auto foo(int n) {
  auto LX = [&] (int n) { return [&] { return ++n; }; };
  return LX;
}

constexpr auto L = foo(3);
constexpr auto M = L(3);
constexpr auto N = L(3);
static_assert(M() == 4, "");
static_assert(N() == 4, "");
static_assert(M() == 5, "");
} // end ns3

namespace ns4 {
constexpr auto foo(int n) {
  auto LX = [&] (int l1) { return [&] (int l2) { return ++n + ++l1 + l2; }; };
  return LX;
}

constexpr auto L = foo(1);
constexpr auto M = L(10);

static_assert(M(100) == 113, "");
static_assert(M(200) == 215, "");
static_assert(M(300) == 317, "");

} // end ns4

namespace ns5 {
constexpr auto foo2(int n) {
  int r = n;
  auto L = [&] { return ++r; };
  struct B : decltype(L) {
    constexpr B(const decltype(L)& l) : decltype(L)(l) { }
  };
  return B{L};
}

constexpr auto B = foo2(3);

static_assert(B() == 4, "");
static_assert(B() == 5, "");

} // end ns5

namespace ns6 {
constexpr auto foo2(int n) {
  int r = n;
  auto L = [&] { return ++r; };
  struct A { decltype(L) l; } a = { L };
  return a;
}

constexpr auto A = foo2(3);

static_assert(A.l() == 4, "");
static_assert(A.l() == 5, "");

} // end ns6

namespace ns7 {
constexpr auto obj1(int data) {
  auto setL = [&] (int n) { return data = n; };
  auto getL = [&] { return data; };
  auto addL = [&] { return ++data; };
  auto subL = [&] { return --data; };
  struct A { 
    decltype(setL) set; 
    decltype(getL) get; 
    decltype(addL) add; 
    decltype(subL) sub; 
  } a = { setL, getL, addL, subL };
  return a;
}

constexpr auto Obj1 = obj1(3);

static_assert(Obj1.set(10) == 10, "");
static_assert(Obj1.add() == 11, "");
static_assert(Obj1.sub() == 10, "");
static_assert(Obj1.get() == 10, "");
static_assert(Obj1.set(100) == 100, "");
static_assert(Obj1.add() == 101, "");
static_assert(Obj1.sub() == 100, "");
static_assert(Obj1.get() == 100, "");

constexpr auto obj2(int data, int data2) {
  auto O1 = obj1(data);
  auto O2 = obj1(data2);
  struct A {
    decltype(O1) first;
    decltype(O2) second;
  } a = { O1, O2 };
  return a;
}


constexpr auto Obj2 = obj2(10, 100);


} // end ns7
namespace ns8 {
constexpr auto obj1(int n) {
  return [&] { return ++n; };
};
constexpr auto obj2(int n) {
  auto LX = obj1(n + 1);
  return LX;
}

constexpr auto L = obj2(5);
static_assert(L() == 7, "");

} // end ns8

namespace ns9 {

constexpr auto obj1(int&& n) {
  return [&] { return ++n; };
};
constexpr auto obj2(int n) {
  auto LX = obj1(n + 1);
  return LX;
}

constexpr auto L = obj2(5);
static_assert(L() == 7, "");
static_assert(L() == 8, "");

} // end ns9


namespace ns10 {

template<class T>
constexpr auto obj1(T&& n) {
  return [&] { return ++n; };
};

constexpr auto obj2(int n) {
  struct A { int n; 
  
    constexpr A(int n) : n(n) { }
    constexpr int operator++() { return ++n; }
  };
  auto LX = obj1(A(n + 1));
  return LX;
}

constexpr auto L = obj2(5);
static_assert(L() == 7, "");
static_assert(L() == 8, "");

} // end ns10


namespace ns11 {
constexpr auto obj1(int n) {
  auto L = [=] () mutable { return ++n; };
  auto M1 = [&] (int m) { return m + L(); };
  auto M2 = [&] (int m) { return m - L(); };
  return make_pair(M1, M2);
}
constexpr auto P = obj1(10);
static_assert(P.first(100) == 111, "");
static_assert(P.second(100) == 88, "");
} // end ns11


namespace ns12 {
constexpr auto obj1(int n) {
  const char *str = "abcdefghijklmnopqrstuvwxyz";
  auto L = [=] () mutable { return str[n++]; };
  auto M1 = [&] (int m) { return L(); };
  auto M2 = [&] (int m) { return L(); };
  return make_pair(M1, M2);
}
constexpr auto P = obj1(1);
static_assert(P.first(100) == 'b', "");
static_assert(P.second(100) == 'c', "");
} // end ns12


namespace ns13 {
constexpr auto obj1(int n) {
  const char *str = "abcdefghijklmnopqrstuvwxyz";
  auto L = [&] () mutable { return str[n++]; };
  auto M1 = [&] (int m) { str = "123456789@#$"; return L(); };
  auto M2 = [&] (int m) { str = "!@#$%^&*()-_=+{}<>"; return L(); };
  return make_pair(M1, M2);
}
constexpr auto P = obj1(1);
static_assert(P.first(100) == 'b', "");
static_assert(P.second(100) == 'c', "");
} // end ns13

} //end test_ref_capture
