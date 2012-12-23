// RUN: %clang -c -std=c++1y %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out
#define USE_PRINTF 1

#ifndef USE_PRINTF
# define printf(...) 

#else

# if USE_PRINTF 
#   include <cstdio>
# else
#   define printf(...) 
# endif

#endif
template<class T> struct X { 
  static const bool value = false;
};
template<> struct X<char> {
  static const bool value = true;
};

auto B = []<bool b>(auto a) a(b);

auto L = []<class T>(T t) {
  constexpr bool b = X<T>::value; 
  return B.operator()<b>([](bool b) b);
};


int main()
{
  //CHECK: L('c') = 1
  printf("L('c') = %d\n", L('c'));
  //CHECK-NEXT: L(2.77) = 0
  printf("L(2.77) = %d\n", L(2.77));

  auto M = [](auto a) { 
    constexpr bool b = false;
    return B.operator()<b>([](auto a) a);
  };    
}