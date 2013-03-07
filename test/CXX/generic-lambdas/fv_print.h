#ifndef INCLUDED_CLANG_TEST_FV_PRINT_H
#define INCLUDED_CLANG_TEST_FV_PRINT_H
/*
 * This file contains a bunch of print helpers
 * since I can't use iostreams when testing clang
 * on windows - so we use printf and overloads
 */
 
#include <cstdio>

#define DUMP(...) fv::print(#__VA_ARGS__ " = ", (__VA_ARGS__), "\n")

namespace fv {
  void print(bool b) {
    printf("%s", b ? "true" : "false");
  }  
  void print(int i) {
    printf("%d", i);  
  }
  void print(char c) {
    printf("%c", c);
  }
 
   void print(double d) {
     printf("%f", d);
   }
   
   void print(const char* s) {
     printf("%s", s);
   }
   
  void print(unsigned u) {
    printf("%u", u);
  }
  
  template<class T> void print(T* p) {
    printf("%p", p);
  }
  template<class T> void print(T t) {
    printf("%s", t.to_string());
  }
  //template<class T> void print(T t) 
  template<class T> void print(T t, const char* header, const char* footer = 0) {
     if (header) printf("%s", header);
     print(t);
     if (footer) printf("%s", footer);
   }
   
   template<class T> void print(const char* header, T t, const char* footer = 0) {
     if (header) printf("%s", header);
     print(t);
     if (footer) printf("%s", footer);
   }
   void print(const char* header, const char* str) {
     print(header);
     print(str);
   }
   void print(const char* header, const char* str, const char* tail) {
     print(header);
     print(str);
     print(tail);
   } 
   
 } // end ns fv
 



#endif