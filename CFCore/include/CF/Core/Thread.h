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
    File:       OSThread.h

    Contains:   A Thread abstraction

*/

#ifndef __CF_CORE_THREAD_H__
#define __CF_CORE_THREAD_H__

#include <CF/Types.h>
#include <CF/DateTranslator.h>

#ifndef __Win32__

#if __PTHREADS__

#if __solaris__ || __sgi__ || __hpux__ || __MinGW__
#include <errno.h>
#else
#include <sys/errno.h>
#endif // __solaris__ || __sgi__ || __hpux__

#include <pthread.h>

#else // !__PTHREADS__
#include <mach/mach.h>
#include <mach/cthreads.h>
#endif // !__PTHREADS__

#endif // !__Win32__

#ifndef DEBUG_THREAD
#define DEBUG_THREAD 0
#else
#undef  DEBUG_THREAD
#define DEBUG_THREAD 1
#endif

namespace CF {
namespace Core {

class Thread {

 public:

  /**
   * configure some settings about thread
   *
   * <b>note:</b> Call before calling any other OSThread function
   */
  static void Initialize();

  Thread();

  virtual ~Thread();

  //
  // Derived classes must implement their own entry function
  virtual void Entry() = 0;

  void Start();

  static void ThreadYield();

  static void Sleep(UInt32 inMsec);

  void Join();

  void SendStopRequest() { fStopRequested = true; }

  bool IsStopRequested() { return fStopRequested; }

  void StopAndWaitForThread();

  void *GetThreadData() { return fThreadData; }

  void SetThreadData(void *inThreadData) { fThreadData = inThreadData; }

  // As a convenience to higher levels, each Thread has its own date buffer
  DateBuffer *GetDateBuffer() { return &fDateBuffer; }

  static void *GetMainThreadData() { return sMainThreadData; }

  static void SetMainThreadData(void *inData) { sMainThreadData = inData; }

#if DEBUG_THREAD
  UInt32 GetNumLocksHeld() { return 0; }
  void IncrementLocksHeld() {}
  void DecrementLocksHeld() {}
#endif

#if __Linux__ || __macOS__

  static void WrapSleep(bool wrapSleep) { sWrapSleep = wrapSleep; }

#endif

#if __WinSock__
  static int TransferErrno(int winErr);
#endif

#ifdef __Win32__
  static int GetErrno();
  static DWORD GetCurrentThreadID() { return ::GetCurrentThreadId(); }
#elif __PTHREADS__
  static int GetErrno() { return errno; }
  static pthread_t GetCurrentThreadID() { return ::pthread_self(); }
#else
  static int GetErrno() { return cthread_errno(); }
  static cthread_t GetCurrentThreadID() { return cthread_self(); }
#endif

  /**
   * @brief 返回持有当前线程的 Thread 对象指针
   */
  static Thread *GetCurrent();

 private:

  // TLS for Thread object pointer
#ifdef __Win32__
  static DWORD sThreadStorageIndex;
#elif __PTHREADS__
  static pthread_key_t gMainKey;
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
  static pthread_attr_t sThreadAttr;
#endif
#endif

  bool fStopRequested;
  bool fJoined;

#ifdef __Win32__
  HANDLE fThreadID;
#elif __PTHREADS__
  pthread_t fThreadID;
#else
  UInt32 fThreadID;
#endif
  void *fThreadData;
  DateBuffer fDateBuffer;

  static void *sMainThreadData;
#ifdef __Win32__
  static unsigned int WINAPI _Entry(LPVOID inThread);
#else

  static void *_Entry(void *inThread);

#endif

#if __Linux__ || __macOS__
  static bool sWrapSleep;
#endif

};

/**
 * @brief 设置线程数据，并在对象销毁后自动设置终值
 *
 * @note 当线程为非抢占调度模型时，可用于实现自动恢复
 */
class ThreadDataSetter {
 public:

  ThreadDataSetter(void *inInitialValue, void *inFinalValue)
      : fFinalValue(inFinalValue) {
    Thread::GetCurrent()->SetThreadData(inInitialValue);
  }

  ~ThreadDataSetter() { Thread::GetCurrent()->SetThreadData(fFinalValue); }

 private:

  void *fFinalValue;
};

} // namespace Core
} // namespace CF

#endif // __CF_CORE_THREAD_H__
