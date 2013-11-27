// RUN: %clang_cc1 -std=c++1y -verify -fsyntax-only -fblocks -emit-llvm-only %s
#define DUMP(x) x

namespace ansdmi1 {
struct Y {
  auto a; //expected-error{{requires an initializer}}
};
struct BadSizeof { //expected-note{{not complete until the closing '}'}}
  auto sz = sizeof(BadSizeof); //expected-error{{incomplete type}}
};
struct BadSizeof2 { //expected-note{{not complete until the closing '}'}}
  const auto sz = sizeof(*this); //expected-error{{incomplete type}}
};

struct X {
  auto sz = sizeof(this);
  auto L = [](auto a) { return a; };
  auto L2 = [this](auto a) { return a + this->sz + f(); };
  int f() const { return 0; }
  // FIXME: this error should say something along the lines of 'sz' not being deduced yet
  auto f2() const -> decltype(this->sz) { return sz; } //expected-error{{cannot initialize return}}
};

auto x1 = X{};
auto x12 = X{}.L;
auto x123 = x12(5);

//auto x2 = X{}.L(5);
//auto x3 = X{}.L2(5);
//auto x4 = X{}.L2;
//auto x5 = x4(7);


}

namespace size_calc {

struct X {
  auto sz1 = sizeof(this);
  auto d = 3.5;
  auto sz2 = d + sz1;
}; 

static_assert(sizeof(X) == sizeof(double) * 3, "sizeof class should be 3 times a double");

}

namespace recursive_size_calc {
struct X { //expected-note 2{{not complete until the closing}}
  // FIXME: this crashes for now with infinite recursion - we need to check
  // to see if we are trying to capture the class being defined by copy
  // when creating a lambda, and if so, error out.
  auto c_not_ok = [x = *this]() ->decltype(auto) { return x; }; //expected-error{{incomplete type}}
  auto r_ok = [&x = *this]() ->decltype(auto) { return x; };
  auto p_ok = [x = this]() ->decltype(auto) { return x; };
  auto &rx = *this;
  auto cx = *this; //expected-error{{incomplete type}}
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

struct X { //expected-note {{not complete until the closing}}
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
    return [&x = *this](int j) { 
      return 0;
    };
  };
  //FXIME: this should not create an error
   auto GL3 = [=](auto i) {
    return [&x = *this](auto j) { 
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

namespace nested_structs_with_auto_nsdmi {
struct X1 { //expected-note 3{{not complete until the closing '}'}}
  auto az1 = alignof(X1); //expected-error{{incomplete type}}
  struct X2 { //expected-note {{not complete until the closing}}
    auto sz1 = sizeof(X1); //expected-error{{incomplete type}}
    auto sz2 = sizeof(X2); //expected-error{{incomplete type}}
    auto x1 = X1{}; //expected-error{{incomplete}}
    auto *x2 = this;
    //FIXME: this is an error currently.
    //int L = ([] (int i) { return i; })(2);
  };
};

}

namespace inconsistent_types {
  struct A { }; //expected-note 3{{candidate constructor}}
  struct B { };
  struct X {
    auto a = A{};
    X() : a(B{}) { } //expected-error{{no matching constructor}}
  };
} // end ns

namespace sizeof_and_alignof_and_create_object_within_lambda {
  struct A { //expected-note 3{{not complete until the closing '}'}}
    auto L = []() {
      return sizeof(*this);//expected-error{{incomplete type}} 
    };
    auto L2 = []() {
      int sz = sizeof(*this); //expected-error{{incomplete type}}
      int az = alignof(decltype(*this)); //expected-error{{incomplete type}}
    };
    auto az = sizeof(az);
    auto az2 = alignof(decltype(az2));
  };
}

namespace bad_tests_fv2 {
struct X1 { //expected-note{{is not complete}}
  X1(int) { }
  auto LI = []() {
    X1 x1{5}; //expected-error{{incomplete type}}
    return 4;
  };
  
  struct X2 { //expected-note {{is not complete}}
    auto sz = sizeof(X2); //expected-error{{incomplete type}} 
  };
};

struct X2 { //expected-note 3{{is not complete}}
  auto az = []() {
    int i = alignof(decltype(*this));   //expected-error{{incomplete type}} 
    constexpr auto sz = alignof(decltype(*this)); //expected-error{{incomplete type}} 
  };
  static const int AZ = sizeof(X2); //expected-error{{incomplete type}} 
};


}

namespace more_tests_fv2 {

struct X1 { 
 X1(int i) { }
 int I = ([]() {
   X1 x1(3);
   return 1;
 })();
 X1 &x12 = *this;
 auto &x13 = *this;
 auto L12 = [=](int a) {
   return [&x = *this]() { };
 };
 auto InnerL12 = L12(4);
 int i = 10;
 auto a = this->i;
 int f() { return 0; }
 char f() const { return 0; }
 auto b = f();
 const auto c = const_cast<const X1*>(this)->f();
 auto ptr = this;
 auto &self = *this;
 struct X2 {
  auto *self = this;
  
 
 };
 auto L = [&r = *this, p = this, this]() {
   return this->f();
 };
 
 auto L2 = ([&r = *this, p = this, this]() {
   return const_cast<const X1*>(this)->f();
 })();
 auto fc() const {
   constexpr unsigned sz = sizeof(*this);
 }

 
};

} // end ns more_tests_fv

namespace template_tests_1 {
namespace ns1 {
template<class T> 
struct X1 {
  auto t = T{4};
  auto GL = [=](auto a, T t2) {
    return t;
  };
};

int test() {
  X1<int> x1i;
  x1i.GL(x1i, 4);
  X1<double> xld;
  xld.GL(xld, 3.14);
  struct Y { Y(int) { } };
  X1<Y> x1y;
  x1y.GL(x1y, Y{2});
} // end test
} // end ns1

namespace ns2 {
template<class T> 
struct X1 : T {
  //using T::f;
  static void f(int i) { }
  auto GL = [=](auto a) {
    f(a);
  };
};

int test() {
  struct Y { 
    static void f(int i) { } 
  };
  X1<Y> x1y;
  x1y.GL(3);
  static_assert(sizeof(x1y.GL) == 1, "no capture of this for static function");
} // end test
} // end ns2

namespace ns3 {
template< class T >
struct MyType : T {
  auto data = func();
  static const int erm = sizeof(data);
  int func() { return 0; }
};

} // end ns3

}
