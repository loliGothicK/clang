// RUN: %clang_cc1 -fsyntax-only -verify %s -std=c++1y
// expected-no-diagnostics
namespace ansdmi1 {
struct X {
  auto sz = sizeof(X);
  auto L = [](auto a) { return a; };
  auto L2 = [this](auto a) { return a + this->sz + f(); };
  int f() const { return 0; }
};

auto x1 = X{};
//auto x2 = X{}.L(5);
//auto x3 = X{}.L2(5);
//auto x4 = X{}.L2;
//auto x5 = x4(7);


}
