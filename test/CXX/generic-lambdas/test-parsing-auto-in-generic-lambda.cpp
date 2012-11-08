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
                //, auto (&array)[5]
              ) -> int { return v + i + c; };

}