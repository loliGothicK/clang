#define USE_PRINTF 1

#if USE_PRINTF
#include <cstdio>
#else
#define printf(...)
#endif
namespace doug_gregor {
template<bool C, typename T, typename U> struct if_;

template<typename T, typename U> struct if_<false, T, U> {
  typedef U type;
};

template<typename T, typename U> struct if_<true, T, U> {
  typedef T type;
};

struct X {
  X(int);
  operator int() const;
};

template<bool C>
constexpr typename if_<C, int, X>::type
f(typename if_<C, int, const int&>::type arg) { return arg; }

void test() {
  const int x = 17;
  int l = 5;
  auto g = [&](auto a) {
    const int i = 0; //f<sizeof(a) == sizeof(char)>(x); -- need to get this to work!
    
    return a + x + ++l;
  };
  printf("g('a') = %c l = %d\n", g('a'), l); // okay: does not capture x
  printf("g(17) = %d l = %d\n", g(17), l);  // error: captures x
}

}

int main()
{
//*
  int x2 = 10;
  auto L2 = [=] (auto a) 
  {
    return x2;
  };
  //L2(3);
//*/

//*
  int x = 10;
  int c = 12;
  auto L = [&,c](auto a) mutable { 
      ++c;
      x += a;
      return c; 
  };
  int lc = L(3); 
  lc = L('0');
  printf("x = %d lc = %d L()=%d\n", x, lc, L(1));
  doug_gregor::test();
//*/
/*
  int x0 = 5;
  struct X {
    template <class T> T operator()(T t) const { return x0 + t; }
  };
  X x;
  int j = x(1);
//*/


/*
  [](auto b) { return a; };
  struct M {
    template<class T> void foo(T) {
      [](auto c) -> T { return T{}; };
    }
  };
  char c = L('a');
  double d = L(3.14);
  const char* s = L("abcdefghijklm");
  printf("i = %d c = %c d = %f s = %s\n", i, c, d,s);
  
  auto L2 = [](auto* a, auto b, int i) -> int {
    return 4 + i + c;
  };
  int *ip = 0;
  printf("L2('jim',0,5)=%d\n", L2("jim", 0, 5));
//*/

}