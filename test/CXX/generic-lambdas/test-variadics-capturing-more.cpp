// RUN: %clang -c -std=c++1y %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out

#ifndef DONT_USE_PRINTF
#define USE_PRINTF 1
#else
#define USE_PRINTF 0
template<class ... PTs> void print(PTs ... pts) { }
#endif

#ifndef DONT_USE_PRINTF
#include "fv_print.h"
using namespace fv;
#endif


#if USE_PRINTF
#include <cstdio>
#else
#define printf(...)
#endif

/*
namespace fv676rand {

template<class ... VFTs> int variadic_fun(VFTs ... vfts) { 
  return sizeof ...(vfts); 
}

  
template<int ... Ns> struct IntPack {
  template<class ... MemTs> static int variadic_memfun(MemTs ... MemArgs) {
    return variadic_fun(([=]() {
      cout << "\nMemArgs = " <<  MemArgs << "\n";
      cout << "\nNs = " << Ns;
      cout << "\nvariadic_fun(MemArgs...) = " << variadic_fun(MemArgs ...) << "\n";
      cout << "\nMemArgs[Ns] = " <<  MemArgs[Ns] << "<--\n";
      return 0;
    })()...);
  }  
};
  
int main() {
  IntPack<0, 1, 2>::variadic_memfun("123", "ABC", "XYZ");
}
int run = main();
}
*/

namespace fvghj531ran {
template<class T> int variadic_print(T t) {
  print("\n", t);
  return 1;
}

template<class T, class ... VFTs> int variadic_print(T t, VFTs ... vfts) { 
  print("\n", t);
  variadic_print(vfts ...);
  return sizeof ...(vfts); 
}

template<class T> int variadic_call(T t) {
  t();
  return 1;
}

template<class T, class ... VFTs> int variadic_call(T t, VFTs ... vfts) { 
  t();
  variadic_call(vfts ...);
  return sizeof ...(vfts); 
}

template<int N> struct XX { };
template<int ... IPacks> struct int_pack { };

int main() {

  auto L = //[]<class ... Ts>(Ts ... OuterArgs)
             [=]<class ... InnerTs>(InnerTs ... InnerArgs)
               [=]() 
                 [=]()
                    variadic_call(
                       [=]() mutable { decltype(InnerArgs) I = InnerArgs; variadic_print(InnerArgs ...); return 0; } ...);
   auto M = L; //('A', 1);
   //CHECK: 2
   auto N = M(2, 3.14);
   auto O = N();
   auto P = O();
  return 0;
}
int run = main();
}
#if 1
namespace fvg6785432 {

template<class T> int variadic_print(T t) {
  print("\n", t);
  return 1;
}

template<class T, class ... VFTs> int variadic_print(T t, VFTs ... vfts) { 
  print("\n", t);
  variadic_print(vfts ...);
  return sizeof ...(vfts); 
}

template<class T> int variadic_call(T t) {
  t();
  return 1;
}

template<class T, class ... VFTs> int variadic_call(T t, VFTs ... vfts) { 
  t();
  variadic_call(vfts ...);
  return sizeof ...(vfts); 
}

template<int N> struct XX { };
template<int ... IPacks> struct int_pack { };
int main() {
  constexpr int arr[]{1, 2, 3};
  XX<arr[0]> xx;
//*
{
  auto L =  [=]<int ... Ns>()
              [=]()
                //[=]<class ... InnerInnerTs>(InnerInnerTs ... InnerInnerArgs)                
                    variadic_print(([=]<class ... IIITs>(IIITs ... iiits) {
                      print("\nIndex = ", Ns);
                      //Ns;
                      //variadic_print(iiits...);
                      variadic_print(Ns ...);
                      constexpr int arr[]{Ns ... };
                      XX<Ns> xx;
                      return Ns + 1;//"jim -- \n";
                   })( (Ns + 100)...)...);
  auto M = L.operator()<1, 2, 3>();
  M();
}
//*/
//*
{
  auto L =  [=]<int ... Ns>()
              [=]<class ... OTs>(OTs ... ots)
              //[=]<class ... ITs>(ITs ... its)
                  variadic_print([=]() mutable {
                    variadic_print(ots);
                    //variadic_print(ots...);
                    variadic_print(Ns);
                    //variadic_print(its);
                    return 1;
                  }()...);
  auto M = L.operator()<1, 2>(); //('A', 'B');
  //auto N = M(2, 3);
  //auto O = N.operator()<1, 2>();
}
//*/

  return 0;
}

int run = main();

}


namespace fvg67854 {

template<class T> int variadic_print(T t) {
  print("\n", t);
  return 1;
}

template<class T, class ... VFTs> int variadic_print(T t, VFTs ... vfts) { 
  print("\n", t);
  variadic_print(vfts ...);
  return sizeof ...(vfts); 
}

template<class T> int variadic_call(T t) {
  t();
  return 1;
}

template<class T, class ... VFTs> int variadic_call(T t, VFTs ... vfts) { 
  t();
  variadic_call(vfts ...);
  return sizeof ...(vfts); 
}


template<int ... IPacks> struct int_pack { };
int main() {
  
  auto L = []<int ... Is>(int_pack<Is...>)
              []() 
                []<int ...Js>(int_pack<Js...>)
                  []()
                      variadic_call([]() {
                        print("\nIs = ", Is);
                        variadic_call([]() {
                          print("\nJs = ", Js);
                        }..., ([]()
                                ([]<class ... Ts>(Ts ... ts) { 
                                      print("\nJs + 10=", Js + 10); 
                                      print("\n"); 
                                      variadic_print(ts...); })(Js...)) ...);
                      }...);
                      
  int_pack<1, 3, 5> ip;
  int_pack<2, 4, 6> jp;
  auto M = L(ip);
  auto N = M();
  auto O = N(jp);
  auto P = O();
  {
//*
  auto L =  []<class ... Ts>(Ts ... args)
              [args...]()
                variadic_print(([args]() args)()...);
                
  auto M = L.operator()<char, int>('A', 5, 3.14);
  print("\n---------------\n");
  auto N = M();
  print("\n---------------\n");
  
//*/
}

{
//*
  auto L =  []<class ... Ts>(Ts ... args)
              [args...]()
                variadic_call([=]() 
                                variadic_print( 
                                    ([args](Ts ... args2) 
                                          args + variadic_print(args2...))(args...) ...) );
                
  auto M = L.operator()<char, int>('A', 5, 3.14);
  print("\n+++++++++++++++++++++++\n");
  auto N = M();
  print("\n+++++++++++++++++++++++\n");
  
//*/
}
  return 0;
}

int run = main();

}

namespace fvg756 {
template<class T> int variadic_foo(T t) {
  print("\n", t);
  return 1;
}

template<class T, class ... VFTs> int variadic_foo(T t, VFTs ... vfts) { 
  print("\n", t);
  variadic_foo(vfts ...);
  return sizeof ...(vfts); 
}
  
int main() {


{
//*
  auto L =  [=]<int ... Ns>()
                //[=]<class ... InnerInnerTs>(InnerInnerTs ... InnerInnerArgs)                
                    variadic_foo(([=]<class ... IIITs>(IIITs ... iiits) {
                      print("\nIndex = ", Ns);
                      //Ns;
                      variadic_foo(iiits...);
                      return Ns + 1;//"jim -- \n";
                   })( (Ns + 100)...)...);
  auto M = L.operator()<1, 2, 3>();
//*/
}

{
//*
  print("\n\n\n");
  auto L =  [=]<class ... Ts>(Ts ... Ns)
                //[=]<class ... InnerInnerTs>(InnerInnerTs ... InnerInnerArgs)                
                    variadic_foo(([=]<class ... IIITs>(IIITs ... iiits) {
                      print("\nIndex = ", Ns);
                      //Ns;
                      variadic_foo(Ns ...);
                      variadic_foo(iiits ...);
                      return variadic_foo( (Ns + iiits) ...);//return iits;//"jim -- \n";
                   })( (Ns + 10)...)...);
  auto M = L(1, 2); //L.operator()<1, 2>();
//*/
}  
  //auto P = M('A');
  //auto Q = P("abc", "def", "efg");
  //print("\n\n");
  return 0;
}

int run = main();
}


namespace fv4787randXX2xd {

template<class T> int variadic_foo(T t) {
  print("\n", t);
  return 1;
}

template<class T, class ... VFTs> int variadic_foo(T t, VFTs ... vfts) { 
  print("\n", t);
  variadic_foo(vfts ...);
  //print(vfts ... );
  return sizeof ...(vfts); 
}
  
int main() {
  auto L = []<class ... OuterTs>(OuterTs ... OuterArgs)
              [=]<class ... InnerTs>(InnerTs ... InnerArgs)
                 [=]<int ... Ns>()
                    [=]<int ... Ms>()                   
                       [=]<class ... InnerInnerTs>(InnerInnerTs ... InnerInnerArgs)
                    
                    variadic_foo(([=]<class ... IIITs>(IIITs ... iiits) {
                      print("\nInnerArgs = ", InnerArgs);
                      print("\nIndex = ", Ns);
                      print("\nOuterArgs = ", OuterArgs);
                      //print("\nInnerInnerArgs = ", InnerInnerArgs);
                      
                      print("\nvariadic_foo(IIA...) = ", variadic_foo(InnerInnerArgs ... ));
                      print("\nvariadic_foo(Ns...) = ", variadic_foo(Ns ... ));
                      
                      //OuterArgs;
                      //InnerArgs;
                      return "jim -- \n";
                   })(Ns...)...);
  auto M = L('A', 3.14);
  auto N = M(5, true);
  //N(4, 5);
  //M("head = ", 3, " tail\n");
  auto O = N.operator()<1, 2>();
  auto P = O.operator()<4, 5, 6>();
  auto Q = P("abc", "def", "efg");
  print("\n\n");
  return 0;
}
int run = main();
}



namespace fv4787randXX2 {

template<class T> int variadic_foo(T t) {
  print("\n", t);
  return 1;
}

template<class T, class ... VFTs> int variadic_foo(T t, VFTs ... vfts) { 
  print("\n", t);
  variadic_foo(vfts ...);
  //print(vfts ... );
  return sizeof ...(vfts); 
}
  
int main() {
  auto L = []<class ... OuterTs>(OuterTs ... OuterArgs)
              [=]<class ... InnerTs>(InnerTs ... InnerArgs)
                 [=]<int ... Ns>() 
                   [=]<class ... InnerInnerTs>(InnerInnerTs ... InnerInnerArgs)
                    
                    variadic_foo(([=]() {
                      print("\nInnerArgs = ", InnerArgs);
                      print("\nIndex = ", Ns);
                      print("\nOuterArgs = ", OuterArgs);
                      //print("\nInnerInnerArgs = ", InnerInnerArgs);
                      
                      print("\nvariadic_foo(IIA...) = ", variadic_foo(InnerInnerArgs ... ));
                      print("\nvariadic_foo(Ns...) = ", variadic_foo(Ns ... ));
                      
                      //OuterArgs;
                      //InnerArgs;
                      return "jim -- \n";
                   })()...);
  auto M = L('A', 3.14);
  auto N = M(5, true);
  //N(4, 5);
  //M("head = ", 3, " tail\n");
  auto O = N.operator()<1, 2>();
  auto P = O("abc", "def", "efg");
  print("\n\n");
  return 0;
}
int run = main();
}


namespace fv3787rand3 {

template<class ... VFTs> int variadic_foo(VFTs ... vfts) { 
  print("\n");
  print(vfts ... );
  return sizeof ...(vfts); 
}

  
template<int ... Ns> struct IntPack {
  template<class ... MemTs> static int variadic_memfun(MemTs ... MemArgs) {
    return variadic_foo(([=]() {
      print("\nMemArgs = ", MemArgs, "\n");
      print("\nNs = ", Ns);
      print("\nvariadic_foo(MemArgs...) = ", variadic_foo(MemArgs ...), "\n");
      print("\nMemArgs[Ns] = ", MemArgs[Ns], "<--\n");
      return "hello";
    })()...);
  }  
};
  
  
int main() {
  IntPack<0, 1, 2>::variadic_memfun("123", "ABC", "XYZ");
  auto L = []<class ... OuterTs>(OuterTs ... OuterArgs)
              [=]<class ... InnerTs>(InnerTs ... InnerArgs)
                 [=]<int ... Ns>() 
                   [=]<class ... InnerInnerTs>(InnerInnerTs ... InnerInnerArgs)
                    
                    variadic_foo(([=]() {
                      print("\nInnerArgs = ", InnerArgs);
                      print("\nIndex = ", Ns);
                      print("\nOuterArgs = ", OuterArgs);
                      //print("\nInnerInnerArgs = ", InnerInnerArgs);
                      
                      print("\nvariadic_foo(IIA...) = ", variadic_foo(InnerInnerArgs ... ));
                      //OuterArgs;
                      //InnerArgs;
                      return "jim -- \n";
                   })()...);
  auto M = L('A', 3.14);
  auto N = M(5, true);
  //N(4, 5);
  //M("head = ", 3, " tail\n");
  auto O = N.operator()<1, 2>();
  auto P = O("abc", "def", "efg");
  print("\n\n");
  return 0;
}
int run = main();
}


namespace fv3787rand {
template<class ... VFTs> int variadic_foo(VFTs ... vfts) { return 5; }

int main_fv() {
  
  auto L = []<class ... OuterTs>(OuterTs ... OuterArgs)
              [=]<class ... InnerTs>(InnerTs ... InnerArgs)
                 [=]<int ... Ns>() 
                   [=]()
                    variadic_foo(([=]() {
                      print("\nInnerArgs = ", InnerArgs);
                      print("\nIndex = ", Ns);
                      print("\nOuterArgs = ", OuterArgs);
                      //OuterArgs;
                      //InnerArgs;
                      return 1;
                   })()...);
  auto M = L('A', 3.14);
  auto N = M(5, true);
  auto O = N.operator()<1, 2>();
  auto P = O();
  print("\n\n");
  return 5;
}
int run = main_fv();
}

namespace fv3787rand2 {
template<class ... VFTs> int variadic_foo(VFTs ... vfts) { return 5; }

int main_fv() {
  
  auto L = []<class ... OuterTs>(OuterTs ... OuterArgs)
              [=]<class ... InnerTs>(InnerTs ... InnerArgs)
                 [=]<int ... Ns>() 
                   [=]<class ... InnerInnerTs>(InnerInnerTs ... InnerInnerArgs)
                    variadic_foo(([=]() {
                      print("\nInnerArgs = ", InnerArgs);
                      print("\nIndex = ", Ns);
                      print("\nOuterArgs = ", OuterArgs);
                      print("variadic_foo(IIA...) = ", variadic_foo(InnerInnerArgs ... ));
                      //OuterArgs;
                      //InnerArgs;
                      return 1;
                   })()...);
  auto M = L('A', 3.14);
  auto N = M(5, true);
  auto O = N.operator()<1, 2>();
  auto P = O();
  print("\n\n");
  return 5;
}
int run = main_fv();
}


//* Test Reference Variadics
namespace fv7718 {

template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<> int variadic_foo() { return -1; }
int test_variadics() {
  auto L = []<class ... Ts>(Ts&& ... args)
     [&]()
      variadic_foo(
        ([&](Ts&& ... args22) variadic_foo(args22 ...) + args)
                                    (static_cast<Ts&&>(args)...) 
                 ...);
  auto M = L('A', 3.14);            
  auto N = M();
  return 0;
}
int T = test_variadics();
}
//*/

//* Test value variadics
namespace fv7719 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<> int variadic_foo() { return -1; }
int test_variadics() {
  auto L = []<class ... Ts>(Ts ... args)
     [=]()
      variadic_foo(
        ([=](Ts ... args22) variadic_foo(args22 ...) + args)
                                    (args...) 
                 ...);
  auto M = L('A', 3.14);            
  auto N = M();
  return 0;
}
int T = test_variadics();
}
//*/



//* Test non-type variadics
namespace fv7720 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<int N, class T> T get(T t) { return t; }
template<int N, class T> struct get_type {
  typedef T type;
};  
template<> int variadic_foo() { return -1; }
int test_variadics() {
  auto L = []<int ... Ns, class ... Ts>(Ts ... args)
     [=]()
      variadic_foo(
        ([=](typename get_type<Ns,Ts>::type ... args22) 
                    //5)(args...));
                    variadic_foo(get<Ns>(args22) ...) + args)
                                    (args...) 
                    ...);
  auto M = L.operator()<1, 2>('A', 3.14);            
  auto N = M();
  return 0;
}
int T = test_variadics();
}
//*/

//* Test non-type variadics member function
namespace fv7721 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<int N, class T> T get(T t) { return t; }
template<int N, class T> struct get_type {
  typedef T type;
};  
template<> int variadic_foo() { return -1; }

template<int ... Ns>
int test_variadics() {
  auto L = []<class ... Ts>(Ts ... args)
     [=]()
      variadic_foo(
        ([=](typename get_type<Ns,Ts>::type ... args22) 
                    //5)(args...));
                    variadic_foo(get<Ns>(args22) ...) + args)
                                    (args...) 
                    ...);
  auto M = L('A', 6.626, 111);            
  auto N = M();
  return 0;
}
int T = test_variadics<1, 2, 3>();
}
//*/

//* Test non-type variadics member function explicit partial pack
namespace fv77212 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<int N, class T> T get(T t) { return t; }
template<int N, class T> struct get_type {
  typedef T type;
};  

template<> int variadic_foo() { return -1; }
template<int ... PackNs> struct int_pack { };

template<int ... Ns>
int test_variadics(int_pack<Ns...> ps) {
  auto L = []<int ... InnerNs, class ... Ts>(Ts ... args)
     [=]()
      variadic_foo(
        ([=](typename get_type<Ns,Ts>::type ... args22) 
                    //5)(args...));
                    variadic_foo(get<Ns>(args22) ...) + args)
                                    (args...) 
                    ...);
  auto M = L('A', 6.626, 111);            
  auto N = M();
  return 0;
}
int T = test_variadics<1, 2>(int_pack<1,2,3>{});
}
//*/


//* Test non-type variadics member function explicit partial pack
namespace fv772123 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<int N, class T> T get(T t) { return t; }
template<int N, class T> struct get_type {
  typedef T type;
  template<int M> struct inner_type {
    typedef T type;
  };
};  

template<> int variadic_foo() { return -1; }
template<int ... PackNs> struct int_pack { };

template<int ... OuterNs>
int test_variadics(int_pack<OuterNs...> ps) {
 
 auto L = []<int ... InnerNs>(int_pack<InnerNs ...> ip)
      []<class ... Ts>(Ts ... args)
        [=]()
          variadic_foo(
            ([=](typename get_type<InnerNs,Ts>::template inner_type<OuterNs>::type ... args22) 
                    //5)(args...));
                    variadic_foo(get<InnerNs>(args22) ...) + args)
                                    (args...) 
                    ...);
                    
  //auto M = L(int_pack<1, 2, 3>{});
  auto M = L.template operator()<1,2>(int_pack<1, 2, 3>{});
  auto N = M('A', 6.626, 111);            
  auto O = N();
  return 0;
}
int T = test_variadics<1, 2>(int_pack<1,2,3>{});
}
//*/

//* Test non-type variadics member function explicit partial pack
namespace fv7721242 {
void print(char c) {
  printf("%c\n", c);
}

void print(double d) {
  printf("%f\n", d);
}

void print(int i) {
  printf("%d\n", i);
}
void print(unsigned int i) {
  printf("%d\n", i);
}

template<class ... FunTs> int variadic_foo(FunTs ... fts);
template<class FirstT, class ... FunTs> int variadic_foo(FirstT f, FunTs ... rest)
{ 
  print(f);
  variadic_foo(rest ... );    
  return 1000; 
}

template<int N, class T> T get(T t) { return t; }
template<int N, class T> struct get_type {
  typedef T type;
  //template<int M> struct inner_type {
  //  typedef T type;
  //};
};  

template<> int variadic_foo() { return -1; }
template<int ... PackNs> struct int_pack { };

template<int ... OuterNs>
int test_variadics(int_pack<OuterNs...> ps) {
 
 auto L = []<int ... InnerNs>(int_pack<InnerNs ...> ip)
      []<class ... Ts>(Ts ... args)
        [=]()
      //    variadic_foo(5); //sizeof...(Ts));
          variadic_foo(
            ([=](typename get_type<OuterNs,Ts>::type ... args22) 
                    //5)(args...));
                    variadic_foo(get<OuterNs>(args22) ...))
                                    (args...) 
                    );
                    
  auto M = L(int_pack<1, 2, 3>{});
  
  auto N = M('A', 6.626, 111);            
  auto O = N();
  return 0;
}
int T = test_variadics<1, 2>(int_pack<1,2,3>{});
}
//*/

//* Test non-type variadics member function explicit partial pack
namespace fv7721233 {

template<int ... PackNs> struct int_pack { };

int test_variadics() {
 
 auto L = []<int ... InnerNs>(int_pack<InnerNs ...> ip)
      //[]<class ... Ts>(Ts ... args)
        5;
        //[=]()
        //  variadic_foo(
        //    ([=](typename get_type<InnerNs,Ts>::type ... args22) 
                    //5)(args...));
        //            variadic_foo(get<InnerNs>(args22) ...) + args)
        //                            (args...) 
        //            ...);
                    
  //auto M = L(int_pack<1, 2, 3>{});
  auto M = L.operator()<1,2>(int_pack<1, 2, 3>{});
  //auto N = M('A', 6.626, 111);            
  //auto O = N();
  return 0;
}
int T = test_variadics();
}
//*/

template<int ... IPack> struct int_pack { };


namespace ns5634 {

struct FF {
  template<int ... FInts>
  int operator()(int_pack<FInts...> fp) {
    return 0;
  }
};


}
int main() {
{
//*
{
  struct FF {
    template<int ... FInts>
      int operator()(int_pack<FInts...> fp) {
        return 0;
    }
  };
  
  int_pack<1, 2, 3> ffpack;
  FF ff;
  ff.operator()<1, 2, 3>(ffpack);
}
//*/
//*
{
int_pack<1, 2, 3> llpack;
  
auto L = []<int ... LInts>(int_pack<LInts ...> ip) 0;

int ll = L.operator()<1, 2, 3>(llpack);
}
//*/
  
 // FF ff;
 // ff.operator()<1, 2, 3>(ip2);
}
  
}

#endif 