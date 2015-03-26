// RUN: %clang_cc1 -fsyntax-only -verify %s -std=c++1y -DCXX1Y

// prvalue
void prvalue() {
  auto&& x = [](auto a)->void { };
  auto& y = [](auto *a)->void { }; // expected-error{{cannot bind to a temporary of type}}
}

namespace std {
  class type_info;
}

struct P {
  virtual ~P();
};

void unevaluated_operand(P &p, int i) { //expected-note 2{{declared here}}
  // FIXME: this should only emit oneset of errors below 
  int i2 = sizeof([](auto a, auto b)->void{}(3, '4')); // expected-error{{lambda expression in an unevaluated operand}} \
                                                       // expected-error{{invalid application of 'sizeof'}}
  const std::type_info &ti1 = typeid([](auto &a) -> P& { static P p; return p; }(i)); // expected-warning {{expression with side effects will be evaluated despite being used as an operand to 'typeid'}}
  const std::type_info &ti2 = typeid([](auto) -> int { return i; }(i));  // expected-error{{lambda expression in an unevaluated operand}}\
                                                                         // expected-error 2{{cannot be implicitly captured}}\
                                                                         // expected-note 2{{begins here}}\
                                                                         // expected-note{{in instantiation of}}
}
