/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2008 Apple Inc.  All Rights Reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */
/*
    File:       OSMutex.h

    Contains:   Platform-independent Mutex header. The implementation of this
                object is platform-specific. Each platform must define an
                independent OSMutex.h & OSMutex.cpp file.

                This file is for Mac OS X Server only

*/

#ifndef __OS_MUTEX_H__
#define __OS_MUTEX_H__

#include <CF/Types.h>

#ifndef __Win32__

#ifndef __MinGW__
#include <sys/errno.h>
#endif

#if __PTHREADS_MUTEXES__

#if __macOS__

#ifndef _POSIX_PTHREAD_H
#include <pthread.h>
#endif // _POSIX_PTHREAD_H

#else // !__macOS__

#include <pthread.h>

#endif // !__macOS__

#include <unistd.h>

#else // !__PTHREADS_MUTEXES__
#include "mymutex.h"
#endif // !__PTHREADS_MUTEXES__

#endif // !__Win32__

namespace CF {
namespace Core {

class Cond;

class Mutex {
 public:

  Mutex();

  ~Mutex();

  inline void Lock();

  inline void Unlock();

  // Returns true on successful grab of the lock, false on failure
  inline bool TryLock();

 private:

#ifdef __Win32__
  CRITICAL_SECTION fMutex;

  DWORD       fHolder;
  UInt32      fHolderCount;

#elif !__PTHREADS_MUTEXES__
  mymutex_t fMutex;
#else
  pthread_mutex_t fMutex;
  // These two platforms don't implement pthreads recursive mutexes, so
  // we have to do it manually
  pthread_t fHolder;
  UInt32 fHolderCount;
#endif

#if __PTHREADS_MUTEXES__ || __Win32__

  void RecursiveLock();

  void RecursiveUnlock();

  bool RecursiveTryLock();

#endif

  friend class Cond;
};

class MutexLocker {
 public:

  explicit MutexLocker(Mutex *inMutexP) : fMutex(inMutexP) {
    if (fMutex != nullptr) fMutex->Lock();
  }

  ~MutexLocker() { if (fMutex != nullptr) fMutex->Unlock(); }

  void Lock() { if (fMutex != nullptr) fMutex->Lock(); }

  void Unlock() { if (fMutex != nullptr) fMutex->Unlock(); }

 private:
  Mutex *fMutex;
};

void Mutex::Lock() {
#if __PTHREADS_MUTEXES__ || __Win32__
  this->RecursiveLock();
#else
  mymutex_lock(fMutex);
#endif //!__PTHREADS__
}

void Mutex::Unlock() {
#if __PTHREADS_MUTEXES__ || __Win32__
  this->RecursiveUnlock();
#else
  mymutex_unlock(fMutex);
#endif //!__PTHREADS__
}

bool Mutex::TryLock() {
#if __PTHREADS_MUTEXES__ || __Win32__
  return this->RecursiveTryLock();
#else
  return (bool) mymutex_try_lock(fMutex);
#endif //!__PTHREADS__
}

} // namespace Core
} // namespace CF

#endif // __OS_MUTEX_H__
