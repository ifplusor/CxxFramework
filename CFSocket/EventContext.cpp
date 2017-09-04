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
    File:       EventContext.cpp

    Contains:   Impelments object in .h file
*/

#include <CF/Net/Socket/EventContext.h>
#include <CF/Net/Socket/TCPListenerSocket.h>
#include <CF/CFState.h>

#if !__WinSock__

#include <fcntl.h>

#endif

#if MACOSXEVENTQUEUE
#include "tempcalls.h" //includes MacOS X prototypes of event Queue functions
#endif

#if DEBUG_EVENT_CONTEXT
#include <CF/Core/Utils.h>
#include <CF/Core/Time.h>
#endif

using namespace CF::Net;

#if __WinSock__
// See commentary in RequestEvent
std::atomic<unsigned int> EventContext::sUniqueID(WM_USER);
#else
std::atomic<unsigned int> EventContext::sUniqueID(1);
#endif

EventContext::EventContext(int inFileDesc, EventThread *inThread)
    : fFileDesc(inFileDesc),
      fUniqueID(0),
      fUniqueIDStr((char *) &fUniqueID, sizeof(fUniqueID)),
      fEventThread(inThread),
      fWatchEventCalled(false),
      fEventBits(0),
      fAutoCleanup(true),
      fTask(NULL) {}

void EventContext::InitNonBlocking(int inFileDesc) {
  fFileDesc = inFileDesc;

#if __WinSock__
  u_long one = 1;
  int err = ::ioctlsocket(fFileDesc, FIONBIO, &one);
#else
  int flag = ::fcntl(fFileDesc, F_GETFL, 0);
  int err = ::fcntl(fFileDesc, F_SETFL, flag | O_NONBLOCK);
#endif
  AssertV(err == 0, Core::Thread::GetErrno());
}

void EventContext::Cleanup() {
  int err = 0;

  if (fFileDesc != kInvalidFileDesc) {
    //if this object is registered in the table, unregister it now
    if (fUniqueID > 0) {
      fEventThread->fRefTable.UnRegister(&fRef);

#if !MACOSXEVENTQUEUE
      select_removeevent(fFileDesc);//The eventqueue / select shim requires this
#if __WinSock__
      err = ::closesocket(fFileDesc);
#else
      err = close(fFileDesc);
#endif

#else
      // On Linux (possibly other UNIX implementations) you MUST NOT close the
      // fd before removing the fd from the select mask, and having the select
      // function wake up to register this fact. If you close the fd first,
      // bad things may happen, like the Socket not getting unbound from the
      // port & IP addr.
      //
      // So, what we do is have the select Thread itself call close. This is
      // triggered by calling removeevent.
      err = close(fFileDesc);
#endif

      fUniqueID = 0;
      fWatchEventCalled = false;
    } else /* 未分配 id == 未注册 Ref */
#if __WinSock__
      err = ::closesocket(fFileDesc);
#else
      err = close(fFileDesc);
#endif
  }

  fFileDesc = kInvalidFileDesc;
  fUniqueID = 0;

  // we don't really care if there was an error, but it's nice to know
#if __WinSock__
  AssertV(err == 0, ::WSAGetLastError());
#else
  AssertV(err == 0, Core::Thread::GetErrno());
#endif
}

void EventContext::SnarfEventContext(EventContext &fromContext) {
  //+ show that we called watchevent
  // copy the unique id
  // set our fUniqueIDStr to the unique id
  // copy the eventreq
  // find the old event object
  // show us as the object in the fRefTable
  //      we take the OSRef from the old context, point it at our context
  //
  //TODO - this whole operation causes a race condition for Event posting
  //  way up the chain we need to disable event posting
  // or copy the posted events afer this op completes

  fromContext.fFileDesc = kInvalidFileDesc;

  fWatchEventCalled = fromContext.fWatchEventCalled;
  fUniqueID = fromContext.fUniqueID;
  fUniqueIDStr.Set((char *) &fUniqueID, sizeof(fUniqueID)),
      ::memcpy(&fEventReq, &fromContext.fEventReq, sizeof(struct eventreq));

  fRef.Set(fUniqueIDStr, this);
  fEventThread->fRefTable.Swap(&fRef);
  fEventThread->fRefTable.UnRegister(&fromContext.fRef);
}

void EventContext::RequestEvent(int theMask) {
#if DEBUG_EVENT_CONTEXT
  fModwatched = true;
#endif

  if (CFState::sState & CFState::kDisableEvent) return;

  //
  // The first Time this function gets called, we're supposed to
  // call watchevent. Each subsequent Time, call modwatch. That's
  // the way the MacOS X event Queue works.

  if (fWatchEventCalled) {
    fEventReq.er_eventbits = theMask;
#if MACOSXEVENTQUEUE
    if (modwatch(&fEventReq, theMask) != 0)
#else
    if (select_modwatch(&fEventReq, theMask) != 0)
#endif
#if __WinSock__
      AssertV(false, ::WSAGetLastError());
#else
      AssertV(false, Core::Thread::GetErrno());
#endif
  } else {
    // allocate a Unique ID for this Socket, and add it to the Ref table

    //johnson find the bug
    bool bFindValid = false;
#if __WinSock__
    //
    // Kind of a hack. On Win32, the way that we pass around the unique ID is
    // by making it the message ID of our Win32 message (see win32ev.cpp).
    // Messages must be >= WM_USER. Hence this code to restrict the numberspace
    // of our UniqueIDs.
    do {
        static unsigned int topVal = 8192;
        if (!sUniqueID.compare_exchange_weak(topVal, WM_USER))  // Fix 2466667: message IDs above a
            fUniqueID = ++sUniqueID;         // level are ignored, so wrap at 8192
        else
            fUniqueID = (PointerSizedInt) WM_USER;

        //If the fUniqueID is used, find a new one until it's free
        Ref * ref = fEventThread->fRefTable.Resolve(&fUniqueIDStr);
        if (ref != NULL) {
            fEventThread->fRefTable.Release(ref);
        } else {
            bFindValid = true; // ok, it's free
        }
    } while (!bFindValid);
#else
    do {
      static unsigned int topVal = 10000000;
      if (!sUniqueID.compare_exchange_weak(topVal, 1))  // Fix 2466667: message IDs above a
        fUniqueID = ++sUniqueID;         // level are ignored, so wrap at 8192
      else
        fUniqueID = 1;

      // If the fUniqueID is used, find a new one until it's free
      Ref *Ref = fEventThread->fRefTable.Resolve(&fUniqueIDStr);
      if (Ref != NULL) {
        fEventThread->fRefTable.Release(Ref);
      } else {
        bFindValid = true; // ok, it's free
      }

      // if id pool is empty, here will spin until some one release.
    } while (!bFindValid);
#endif

    fRef.Set(fUniqueIDStr, this);
    fEventThread->fRefTable.Register(&fRef);

    // fill out the eventreq data structure
    ::memset(&fEventReq, '\0', sizeof(fEventReq));
    fEventReq.er_type = EV_FD;
    fEventReq.er_handle = fFileDesc;
    fEventReq.er_eventbits = theMask;
    fEventReq.er_data = (void *) fUniqueID;

    fWatchEventCalled = true;
#if MACOSXEVENTQUEUE
    if (watchevent(&fEventReq, theMask) != 0)
#else
    if (select_watchevent(&fEventReq, theMask) != 0)
#endif
      //this should never fail, but if it does, cleanup.
      AssertV(false, Core::Thread::GetErrno());
  }
}

void EventThread::Entry() {
  int theErrno;
  struct eventreq theCurrentEvent;
  ::memset(&theCurrentEvent, '\0', sizeof(theCurrentEvent));

  while (true) {
    do {
      if (IsStopRequested()) return; // stop requested

      // wait for Net event
#if MACOSXEVENTQUEUE
      int theReturnValue = waitevent(&theCurrentEvent, NULL);
#else
      int theReturnValue = select_waitevent(&theCurrentEvent, NULL);
#endif

      if (CFState::sState & (CFState::kKillListener | CFState::kCleanEvent)) {
        // kill listener Socket
        if (CFState::sState & CFState::kKillListener) {
          while (true) {
            QueueElem *elem = CFState::sListenerSocket.DeQueue();
            if (elem == nullptr) break;
            TCPListenerSocket *listener = (TCPListenerSocket *)
                elem->GetEnclosingObject();
            listener->RequestEvent(EV_RM);
            listener->Signal(CF::Thread::Task::kKillEvent);
            delete elem;
          }
          CFState::sState ^= CFState::kKillListener;
          continue;
        }

        if (CFState::sState & CFState::kCleanEvent) {
          RefHashTableIter iter(fRefTable.GetHashTable());
          while (!iter.IsDone()) {
            Ref *ref = iter.GetCurrent();
            EventContext *theContext = (EventContext *) ref->GetObject();
            iter.Next();
            theContext->Cleanup();
          }
          CFState::sState ^= CFState::kCleanEvent;
          /* kCleanEvent 必 kDisableEvent，此时 select 模型再也不会产生新事件 */
          continue;
        }
      }

      // Sort of a hack. In the POSIX version of the server, waitevent can
      // return an actual POSIX error code.
      if (theReturnValue >= 0)
        theErrno = theReturnValue;
      else
        theErrno = Core::Thread::GetErrno();
    } while (theErrno == EINTR);
    AssertV(theErrno == 0, theErrno);

    // ok, there's data waiting on this Socket. Send a wakeup.
    if (theCurrentEvent.er_data != NULL) {
      // The cookie in this event is an ObjectID. Resolve that objectID into
      // a pointer.
      StrPtrLen idStr((char *) &theCurrentEvent.er_data, sizeof(PointerSizedInt));
      Ref *ref = fRefTable.Resolve(&idStr);
      if (ref != NULL) {
        EventContext *theContext = (EventContext *) ref->GetObject();
#if DEBUG_EVENT_CONTEXT
        theContext->fModwatched = false;
#endif
        theContext->ProcessEvent(theCurrentEvent.er_eventbits);
        fRefTable.Release(ref);
      }
    }

#if DEBUG_EVENT_CONTEXT
    SInt64  yieldStart = Core::Time::Milliseconds();
#endif

#if 0//defined(__linux__) && !defined(EASY_DEVICE)

#else
    this->ThreadYield();
#endif

#if DEBUG_EVENT_CONTEXT
    SInt64  yieldDur = Core::Time::Milliseconds() - yieldStart;
    static SInt64   numZeroYields;

    if (yieldDur > 1) {
      s_printf("EventThread Time in OSThread::Yield %i, numZeroYields %i\n",
               (SInt32) yieldDur,
               (SInt32) numZeroYields);
        numZeroYields = 0;
    } else
        numZeroYields++;
#endif
  }
}
