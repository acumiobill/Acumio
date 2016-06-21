#ifndef AcumioServer_shared_mutex_h
#define AcumioServer_shared_mutex_h
//============================================================================
// Name        : shared_mutex.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides a fast SharedMutex mechanism that allows for
//               multiple concurrent SharedLocks but only a single
//               ExclusiveLock.
//
//               This is performed via a spin-lock mechanism on an atomic
//               variable.
//
//               Usage:
//
//               class Foo {
//                public:
//                 Foo(int dummy) : mutex_, dummy_(dummy) ... {}
//                 ~Foo() {}
//                 int get_dummy();
//                 void update_dummy(int new_val);
//                   ...
//                private:
//                 acumio::transaction::SharedMutex mutex_;
//                 int dummy_;
//                  ...
//               };
//
//               int Foo::get_dummy() {
//                 SharedLock lock(mutex_);
//                 return dummy_;
//               }
//
//               void Foo::update_dummy(int new_val) {
//                 ExclusiveLock lock(mutex_);
//                 dummy_ = new_val;
//               }
//============================================================================

#include <atomic>

namespace acumio {
namespace transaction {

class SharedLock;
class ExclusiveLock;

class SharedMutex {
 public:
  SharedMutex();
  ~SharedMutex();

  // This is an emergency mechanism that will allow us to break locks
  // even in the event of thread-death of a thread that did not properly
  // release its lock.
  void break_locks();

 private:
  friend class SharedLock;
  friend class ExclusiveLock;
  void acquire();
  void release();
  void acquire_exclusive();
  void release_exclusive();
  std::atomic<int16_t> state_;
};

class SharedLock {
 public:
  SharedLock(SharedMutex& mutex);
  ~SharedLock();

  inline SharedMutex& GetMutex() { return mutex_; }
 private:
  SharedMutex& mutex_;
};

class ExclusiveLock {
 public:
  ExclusiveLock(SharedMutex& mutex);
  ~ExclusiveLock();

  inline SharedMutex& GetMutex() { return mutex_; }
 private:
  SharedMutex& mutex_;
};

} // namespace transaction
} // namespace acumio
#endif // AcumioServer_shared_mutex_h
