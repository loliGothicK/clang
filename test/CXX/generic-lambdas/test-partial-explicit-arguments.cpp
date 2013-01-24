// RUN: %clang -c -std=c++1y %s -emit-llvm -o %s.bc
// RUN: lli %s.bc > %s.out
// RUN: FileCheck %s --input-file=%s.out

#include "fv_print.h"
using namespace fv;

template<int ... INs> struct int_pack {
  enum { value = sizeof...(INs) };
};
template<class ... Ts> struct type_pack {

};

struct X {
template<class ... Ts, int ... Ns, 
          template<class ... > class ... TTs, 
          template<int ... > class ... TTInts, 
          template<class ...> class ... TTTypes>
  int foo(TTs< TTTypes<TTInts<Ns...> ...> ...> ... tts, Ts ... ts) {
    return 0; 
  }
};


int main()
{
  {
    auto L = []<int ... Ns>(int_pack<Ns...> ip) 0;
    //CHECK: L.operator()<1, 2, 3>(int_pack<1, 2, 3, 4>{}) = 0
    DUMP(L.operator()<1, 2, 3>(int_pack<1, 2, 3, 4>{}));
  }
  {
  auto LM = []<class ... Ts, int ... Ns, 
        template<class ... > class ... TTs, 
        template<int ... > class ... TTInts, 
        template<class ...> class ... TTTypes>
          (TTs< TTTypes<TTInts<Ns...> ...> ...> ... tts) 0;
  
  typedef int_pack<1, 2, 3, 4> ipack4;
  typedef type_pack<ipack4, ipack4, ipack4> tpack_ipack4_3;
  typedef type_pack<tpack_ipack4_3, tpack_ipack4_3> ttpack;
  //CHECK: LM.operator()<char, int>( ttpack{}, ttpack{}) = 0
  DUMP(LM.operator()<char, int>( ttpack{}, ttpack{}));  
  
  }
  {
  auto LM = []<int ... Ns, 
        template<class ... > class ... TTs, 
        template<int ... > class ... TTInts, 
        template<class ...> class ... TTTypes>
          (TTs< TTTypes<TTInts<Ns...> ...> ...> ... tts) 0;
  typedef int_pack<1, 2, 3, 4> ipack4;
  typedef type_pack<ipack4, ipack4, ipack4> tpack_ipack4_3;
  typedef type_pack<tpack_ipack4_3, tpack_ipack4_3> ttpack;
  //CHECK: LM.operator()<1, 2>( ttpack{}, ttpack{})
  DUMP(LM.operator()<1, 2>( ttpack{}, ttpack{}));  
  
  }
  {
    auto X2 = []<template< int ... > class ... ipacks, int ... Ns> 
      (ipacks<Ns...> ... ips) 'c';
    
    typedef int_pack<1, 2, 3, 4, 5> ipack5;
    
    //CHECK: X2.operator()<int_pack, int_pack>(ipack5{}, ipack5{}, ipack5{}) = c
    DUMP(X2.operator()<int_pack, int_pack>(ipack5{}, ipack5{}, ipack5{}));
  
  }
  {
  auto X3 = []<class ... Ts, int ... Ns, 
        template<class ... > class ... TTs, 
        template<int ... > class ... TTInts, 
        template<class ...> class ... TTTypes>
           (TTs< TTTypes<TTInts<Ns...> ...> ...> ... tts) 0;
  typedef int_pack<1, 2, 3, 4> ipack4;
  typedef type_pack<ipack4, ipack4, ipack4> tpack_ipack4_3;
  typedef type_pack<tpack_ipack4_3, tpack_ipack4_3> ttpack;
  //CHECK: X3.operator()<char, double>( ttpack{}, ttpack{}) = 0
  DUMP(X3.operator()<char, double>( ttpack{}, ttpack{}));  
  
  }
 
  {
  auto X4 = []<class ... Ts, int ... Ns, 
        template<class ... > class ... TTs, 
        template<int ... > class ... TTInts, 
        template<class ...> class ... TTTypes>
          (Ts (*...fp)(TTs< TTTypes<TTInts<Ns...> ...> ...> ... tts)) 0;
  typedef int_pack<1, 2, 3, 4> ipack4;
  typedef type_pack<ipack4, ipack4, ipack4> tpack_ipack4_3;
  typedef type_pack<tpack_ipack4_3, tpack_ipack4_3> ttpack;
  //CHECK: X4.operator()<char, double>( (char (*)(ttpack)) 0, (double (*)(ttpack, ttpack)) 0)) = 0
  DUMP(X4.operator()<char, double>( (char (*)(ttpack)) 0, (double (*)(ttpack, ttpack)) 0));  
  
  }
  
  
  
}  
  