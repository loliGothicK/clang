// RUN: %clang_cc1 %s -emit-llvm -o - | FileCheck %s

struct basic_ios{~basic_ios(); };

template<typename _CharT> struct basic_istream : virtual public basic_ios {
  virtual ~basic_istream(){}
};

template<typename _CharT> struct basic_iostream : public basic_istream<_CharT>
{
  virtual ~basic_iostream(){}
};

basic_iostream<char> res;

int main() {
}

// basic_iostream's complete dtor calls its base dtor, then its
// virtual base's dtor.
//  CHECK: define linkonce_odr void @_ZN14basic_iostreamIcED1Ev
//  CHECK: call void @_ZN14basic_iostreamIcED2Ev
//  CHECK: call void @_ZN9basic_iosD2Ev

// basic_iostream's base dtor calls its non-virtual base dtor.
//  CHECK: define linkonce_odr void @_ZN14basic_iostreamIcED2Ev
//  CHECK: call void @_ZN13basic_istreamIcED2Ev
//  CHECK: }

// basic_istream's base dtor is a no-op.
//  CHECK: define linkonce_odr void @_ZN13basic_istreamIcED2Ev
//  CHECK-NOT: call
//  CHECK: }

// basic_iostream's deleting dtor calls its complete dtor, then
// operator delete().
//  CHECK: define linkonce_odr void @_ZN14basic_iostreamIcED0Ev
//  CHECK: call void @_ZN14basic_iostreamIcED1Ev
//  CHECK: call void @_ZdlPv

// basic_istream's complete dtor calls the base dtor,
// then its virtual base's base dtor.
//  CHECK: define linkonce_odr void @_ZN13basic_istreamIcED1Ev
//  CHECK: call void @_ZN13basic_istreamIcED2Ev
//  CHECK: call void @_ZN9basic_iosD2Ev

// basic_istream's deleting dtor calls the complete dtor, then
// operator delete().
//  CHECK: define linkonce_odr void @_ZN13basic_istreamIcED0Ev
//  CHECK: call void @_ZN13basic_istreamIcED1Ev
//  CHECK: call void @_ZdlPv
