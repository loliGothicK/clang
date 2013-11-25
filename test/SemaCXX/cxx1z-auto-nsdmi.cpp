// RUN: %clang_cc1 -std=c++1y -verify -fsyntax-only -fblocks -emit-llvm-only %s
#define DUMP(x) x

namespace ansdmi1 {
struct Y {
  auto a; //expected-error{{requires an initializer}}
};
struct X {
  auto sz = sizeof(X);
  const auto sz_const = sizeof(*this);
  auto L = [](auto a) { return a; };
  auto L2 = [this](auto a) { return a + this->sz + f(); };
  int f() const { return 0; }
  // FIXME: this error should say something along the lines of 'sz' not being deduced yet
  auto f2() const -> decltype(this->sz) { return sizeof(sz); } //expected-error{{cannot initialize return object}}
};

auto x1 = X{};
auto x11 = X{}.sz_const;
auto x12 = X{}.L;
auto x123 = x12(5);

//auto x2 = X{}.L(5);
//auto x3 = X{}.L2(5);
//auto x4 = X{}.L2;
//auto x5 = x4(7);


}

namespace size_calc {

struct X {
  auto sz1 = sizeof(X);
  auto d = 3.5;
  auto sz2 = d + sz1;
}; 

static_assert(sizeof(X) == sizeof(double) * 3, "sizeof class should be 3 times a double");

}

namespace recursive_size_calc {
struct X { 
  // FIXME: this crashes for now with infinite recursion - we need to check
  // to see if we are trying to capture the class being defined by copy
  // when creating a lambda, and if so, error out.
  auto c_not_ok = [x = *this]() ->decltype(auto) { return x; }; //expected-error{{incomplete type}}
  auto r_ok = [&x = *this]() ->decltype(auto) { return x; };
  auto p_ok = [x = this]() ->decltype(auto) { return x; };
  auto &rx = *this;
  auto cx = *this; //expected-error{{incomplete type}} \
                   //expected-error{{no viable conversion}}
  auto *px = this;

};

X global_x;

int test() {
  // FIXME: why does this cause an assertion failure if a wrong member is used
  //auto L = X{}.m;
  auto r_ok = X{}.r_ok;
  auto p_ok = X{}.p_ok;
  return 0;
}
int run = test();

}


namespace capturing_this_in_generic_lambdas {

namespace explicit_capture {
struct X {
  int mi = 5;
  auto GL = [this](auto i) {
    return this->mi;
  };
};

int test() {
  X x;
  x.GL(3);
  return 0;
}

int run = test();
} // explicit capture

namespace implicit_capture {
struct X {
  int mi = 5;
  auto GL = [=](auto i) {
    return this->mi;
  };
};

int test() {
  X x;
  x.GL(3);
  return 0;
}

int run = test();
} // end ns

namespace nested_lambda_this_capture {

struct X {
  int mi = 5;
  auto GL = [=](auto i) {
    return [=](auto j) {
      return this->mi + i + j;
    };
  };
  auto GL_bad = [=](auto i) {
    return [x = *this](auto j) { //expected-error{{incomplete}}
      return 0;
    };
  };
  auto GL2 = [=](auto i) {
    return [&x = *this](auto j) { //expected-error{{incomplete}}
      return 0;
    };
  };
};

int test() {
  X x;
  x.GL(3)(6.26);
  return 0;
}
int run = test();
} // end ns nested_lambda_this_capture

} //end ns capturing_this_in_generic_lambdas

namespace static_auto_nsdmi {
// was already allowed
struct X {
  const static auto s = 3;
  auto x = s;
};

}

namespace adl_with_auto_nsdmi {

namespace yns {
  struct Y { };
}
char foo(int, yns::Y) { return 'a'; }
template<class T>
struct X {
  const static auto s = 3;
  auto a = s + foo(3.14, T{});
};
namespace yns {
double foo(double, Y) { return 1000; }
}
int test() {
 X<yns::Y> x; 
 static_assert(sizeof(x) == sizeof(double), "adl should play a role in determining type");
 return 0;
}
int run = test();
}

namespace more_tests {
struct Y { };
char foo(int, Y) { return 'a'; }
template<class T>
struct X {
  auto L = [this](auto a) { return 0; };
  const static auto s = 3;
  auto a = s + foo(3.14, T{});
  auto mem_foo() {
    return [this](auto i) {
      return this->mi;
    };
  }
  int mi = 5;
  auto GL = [this](auto i) {
    return this->mi;
  };
};

double foo(double, Y) { return 1000; }

int test() {
  X<Y> x;
  auto M = x.mem_foo();
  auto M2 = x.GL;
 
  x.mem_foo()(5);
  M(5);
  //FV_DUMP(M2(5));
}
int run = test();
}
