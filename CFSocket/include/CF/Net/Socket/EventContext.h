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
/**
 * @file EventContext.h
 *
 * An event context provides the intelligence to take an event generated from
 * a UNIX file descriptor (usually EV_RE or EV_WR) and signal a Task.
 */

#ifndef __EVENT_CONTEXT_H__
#define __EVENT_CONTEXT_H__

#include <atomic>

#if MACOSXEVENTQUEUE

#ifdef AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#include <sys/ev.h>
#else
#include "CF/Net/ev.h"
#endif

#else

#include "CF/Net/ev.h"

#endif

#include <CF/Ref.h>
#include <CF/Thread/Task.h>

//enable to trace event context execution and the task associated with the context
#ifndef DEBUG_EVENT_CONTEXT
#define DEBUG_EVENT_CONTEXT 0
#else
#undef  DEBUG_EVENT_CONTENT
#define DEBUG_EVENT_CONTENT 1
#endif

namespace CF {
namespace Net {

class EventThread;

class EventContext {
 public:

  //
  // Constructor. Pass in the EventThread you would like to receive
  // events for this context, and the fd that this context applies to
  EventContext(SOCKET inFileDesc, EventThread *inThread);

  virtual ~EventContext() { if (fAutoCleanup) this->Cleanup(); }

  //
  // Cleanup. Will be called by the destructor, but can be called earlier
  void Cleanup();

  /**
   * @brief Sets inFileDesc to be non-blocking.
   *
   * @note DON'T CALL CLOSE ON THE FD ONCE THIS IS CALLED!!!!
   *
   * Once this is called, the EventContext object "owns" the file descriptor,
   * and will close it when Cleanup is called. This is necessary because of
   * some weird select() behavior.
   */
  void InitNonBlocking(SOCKET inFileDesc);

  void SetMode(bool useET) { this->fUseETMode = useET; }

  //
  // Arms this EventContext. Pass in the events you would like to receive
  virtual void RequestEvent(UInt32 theMask);

  //
  // Provide the task you would like to be notified
  void SetTask(Thread::Task *inTask) {
    fTask = inTask;
    if (DEBUG_EVENT_CONTEXT) {
      if (fTask == nullptr)
        s_printf("EventContext::SetTask context=%p task= NULL\n",
                 (void *) this);
      else
        s_printf("EventContext::SetTask context=%p task= %p name=%s\n",
                 (void *) this, (void *) fTask, fTask->fTaskName);
    }
  }

  // when the HTTP Proxy tunnels takes over a TCPSocket, we need to maintain
  // this context too
  void SnarfEventContext(EventContext &fromContext);

  // Don't cleanup this Socket automatically
  void DontAutoCleanup() { fAutoCleanup = false; }

  // Direct access to the FD is not recommended, but is needed for modules
  // that want to use the Socket classes and need to request events on the fd.
  SOCKET GetSocketFD() { return fFileDesc; }

  static const SOCKET kInvalidFileDesc = INVALID_SOCKET;

 protected:

  /**
   * @brief process network event on socket
   *
   * When an event occurs on this file descriptor, this function
   * will get called. Default behavior is to Signal the associated
   * task, but that behavior may be altered / overridden.
   *
   * Currently, we always generate a Task::kReadEvent
   */
  virtual void ProcessEvent(int /*eventBits*/) {

    if (fTask == nullptr) {
      DEBUG_LOG(DEBUG_EVENT_CONTEXT, "EventContext@%p::ProcessEvent task=NULL\n", this);
    } else {
      DEBUG_LOG(DEBUG_EVENT_CONTEXT, "EventContext@%p::ProcessEvent task=%p TaskName=%s\n",
                this, fTask, fTask->fTaskName);
    }

    if (fTask != nullptr)
      fTask->Signal(Thread::Task::kReadEvent);
  }

  SOCKET fFileDesc;

 private:
  struct eventreq fEventReq;
  bool fUseETMode; // Edge Triggered Mode

  Ref fRef; /* 引用记录，用于 event 调度 */
  PointerSizedInt fUniqueID;
  StrPtrLen fUniqueIDStr;
  EventThread *fEventThread;
  bool fWatchEventCalled;
  int fEventBits;
  bool fAutoCleanup;

  Thread::Task *fTask;

#if DEBUG_EVENT_CONTEXT
  bool fModwatched;
#endif

  static std::atomic<unsigned int> sUniqueID; // id 分配器

  friend class EventThread;
};

/**
 * @brief 基于“IO多路复用”的网络事件守护线程
 *
 * Linux 下为 epoll，Windows 下为 WSAAsyncSelect，OSX 下为 event queue
 */
class EventThread : public Core::Thread {
 public:

  EventThread() : Thread() {}
  ~EventThread() override = default;

 private:

  void Entry() override;

  RefTable fRefTable;

  friend class EventContext;
};

} // namespace Net
} // namespace CF

#endif // __EVENT_CONTEXT_H__
