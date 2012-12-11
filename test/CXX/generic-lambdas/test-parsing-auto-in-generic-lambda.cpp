// RUN: %clang_cc1 -fsyntax-only -verify -std=c++1y %s

int main()
{
  auto l1 = [](auto a) -> int { return a + 5; };
  auto l2 = [](auto *p) -> int { return p + 5; };
 
  struct A { int i; char f(int) { return 'c'; } };
  auto l3 = [](auto &&ur, 
                auto &lr, 
                auto v, 
                int i, 
                auto* p,
                auto A::*memvar,
                auto (A::*memfun)(int),
                char c,
                decltype (v) tv
                , auto (&array)[5] //expected-error {{'array' declared as array of 'auto'}}
              ) -> int { return v + i + c; };

}