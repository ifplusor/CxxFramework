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
    File:       OSMutex.cpp

    Contains:   Platform-independent Mutex header. The implementation of this
                object is platform -specific. Each platform must define an
                independent OSMutex.h & OSMutex.cpp file.

                This file is for Mac OS X Server only

*/

#include <stdlib.h>
#include <CF/Core/Mutex.h>
#include <CF/Core/Thread.h>

using namespace CF::Core;

// Private globals
#if __PTHREADS_MUTEXES__
static pthread_mutexattr_t *sMutexAttr = NULL;
static void MutexAttrInit();

#if __solaris__
static pthread_once_t sMutexAttrInit = { PTHREAD_ONCE_INIT };
#else
static pthread_once_t sMutexAttrInit = PTHREAD_ONCE_INIT;
#endif

#endif

Mutex::Mutex() {
#ifdef __Win32__
  ::InitializeCriticalSection(&fMutex);
  fHolder = 0;
  fHolderCount = 0;
#elif __PTHREADS_MUTEXES__
  (void) pthread_once(&sMutexAttrInit, MutexAttrInit);
  (void) pthread_mutex_init(&fMutex, sMutexAttr);

  fHolder = 0;
  fHolderCount = 0;
#else
  fMutex = mymutex_alloc();
#endif
}

#if __PTHREADS_MUTEXES__
void MutexAttrInit() {
  sMutexAttr = (pthread_mutexattr_t *) malloc(sizeof(pthread_mutexattr_t));
  ::memset(sMutexAttr, 0, sizeof(pthread_mutexattr_t));
  pthread_mutexattr_init(sMutexAttr);
}
#endif

Mutex::~Mutex() {
#ifdef __Win32__
  ::DeleteCriticalSection(&fMutex);
#elif __PTHREADS_MUTEXES__
  pthread_mutex_destroy(&fMutex);
#else
  mymutex_free(fMutex);
#endif
}

#if __PTHREADS_MUTEXES__ || __Win32__
void Mutex::RecursiveLock() {
  // We already have this Mutex. Just refcount and return
  if (Thread::GetCurrentThreadID() == fHolder) {
    fHolderCount++;
    return;
  }
#ifdef __Win32__
  ::EnterCriticalSection(&fMutex);
#else
  (void) pthread_mutex_lock(&fMutex);
#endif
  Assert(fHolder == 0);
  fHolder = Thread::GetCurrentThreadID();
  fHolderCount++;
  Assert(fHolderCount == 1);
}

void Mutex::RecursiveUnlock() {
  if (Thread::GetCurrentThreadID() != fHolder)
    return;

  Assert(fHolderCount > 0);
  fHolderCount--;
  if (fHolderCount == 0) {
    fHolder = 0;
#ifdef __Win32__
    ::LeaveCriticalSection(&fMutex);
#else
    pthread_mutex_unlock(&fMutex);
#endif
  }
}

bool Mutex::RecursiveTryLock() {
  // We already have this Mutex. Just refcount and return
  if (Thread::GetCurrentThreadID() == fHolder) {
    fHolderCount++;
    return true;
  }

#ifdef __Win32__
  bool theErr = (bool)::TryEnterCriticalSection(&fMutex); // Return values of this function match our API
  if (!theErr)
      return theErr;
#else
  int theErr = pthread_mutex_trylock(&fMutex);
  if (theErr != 0) {
    Assert(theErr == EBUSY);
    return false;
  }
#endif

  Assert(fHolder == 0);
  fHolder = Thread::GetCurrentThreadID();
  fHolderCount++;
  Assert(fHolderCount == 1);
  return true;
}

#endif // __PTHREADS_MUTEXES__ || __Win32__
