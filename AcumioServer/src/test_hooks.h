#ifndef AcumioServer_test_hooks_h
#define AcumioServer_test_hooks_h
//============================================================================
// Name        : test_hooks.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Classes and/or utilities to allow inserting code for
//               test purposes in as unobtrusive a manner as possible.
//============================================================================

namespace acumio {
namespace test {

// The purpose of this class is mostly to work with testing a multi-threaded
// environment where we want to conditionally insert sleep operations:
// most of the time, we expect to have this class simply be a collection
// of no-ops. However, in a test environment, we might want to override
// the behavior of the various "invoke" methods to insert sleep operations
// or wait conditions so that we can enforce an ordering of steps in the
// process.
// The hope is that by default, the optimizer will simply ignore the
// calls when they are to the base class, since the operations are all
// inlined.
template <class T>
class TestHook {
 public:
  TestHook() {}
  virtual ~TestHook() {}
  inline virtual void invoke_1(T t) const {}
  inline virtual void invoke_2(T t) const {}
  inline virtual void invoke_3(T t) const {}
  inline virtual void invoke_4(T t) const {}
  inline virtual void invoke_5(T t) const {}
  inline virtual void invoke_6(T t) const {}
  inline virtual void invoke_7(T t) const {}
  inline virtual void invoke_8(T t) const {}
  inline virtual void invoke_9(T t) const {}
  inline virtual void invoke_10(T t) const {}
  inline virtual void invoke_11(T t) const {}
  inline virtual void invoke_12(T t) const {}
  inline virtual void invoke_13(T t) const {}
  inline virtual void invoke_14(T t) const {}
  inline virtual void invoke_15(T t) const {}
  inline virtual void invoke_16(T t) const {}
  inline virtual void invoke_17(T t) const {}
  inline virtual void invoke_18(T t) const {}
  inline virtual void invoke_19(T t) const {}
  inline virtual void invoke_20(T t) const {}
};

} // namespace test
} // namespace acumio
#endif // AcumioServer_test_hooks_h
