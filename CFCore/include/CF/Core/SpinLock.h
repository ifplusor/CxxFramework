//
// Created by james on 6/18/18.
//

#ifndef _EDSS2_SPINLOCK_H_
#define _EDSS2_SPINLOCK_H_

#include <atomic>

namespace CF {
namespace Core {

class SpinLock {
 public:
  SpinLock() = default;
  ~SpinLock() = default;

  void Lock() {
    bool unlatched = false;
    while(!_lock.compare_exchange_weak(unlatched, true, std::memory_order_acquire));
  }

  void Unlock() {
    _lock.store(false , std::memory_order_release);
  }

 private:
  std::atomic_bool _lock;
};

class SpinLocker {
 public:

  explicit SpinLocker(SpinLock *inLockP) : fLock(inLockP) {
      if (inLockP != nullptr) fLock->Lock();
  }

  ~SpinLocker() { if (fLock != nullptr) fLock->Unlock(); }

  void Lock() { if (fLock != nullptr) fLock->Lock(); }

  void Unlock() { if (fLock != nullptr) fLock->Unlock(); }

 private:
  SpinLock *fLock;
};

}
}
#endif // _EDSS2_SPINLOCK_H_
