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
    File:       TimeoutTask.cpp

    Contains:   Implementation of TimeoutTask

*/

#include "TimeoutTask.h"
#include <OSTime.h>

TimeoutTaskThread *TimeoutTask::sThread = nullptr;

void TimeoutTask::Initialize() {
  if (sThread == nullptr) {
    /* TimeoutTaskThread 是 IdleTask 的派生类, IdleTask 是 Task 的派生类。 */
    sThread = new TimeoutTaskThread;
    sThread->Signal(Task::kStartEvent);
  }
}

TimeoutTask::TimeoutTask(Task *inTask, SInt64 inTimeoutInMilSecs)
    : fTask(inTask), fQueueElem() {
  fQueueElem.SetEnclosingObject(this);
  this->SetTimeout(inTimeoutInMilSecs);

  Assert(sThread != nullptr); // this can happen if RunServer initializes tasks in the wrong order

  OSMutexLocker locker(&sThread->fMutex);
  sThread->fQueue.EnQueue(&fQueueElem);
}

TimeoutTask::~TimeoutTask() {
  OSMutexLocker locker(&sThread->fMutex);
  sThread->fQueue.Remove(&fQueueElem);
}

void TimeoutTask::SetTimeout(SInt64 inTimeoutInMilSecs) {
  fTimeoutInMilSecs = inTimeoutInMilSecs;
  if (inTimeoutInMilSecs == 0)
    fTimeoutAtThisTime = 0;
  else
    fTimeoutAtThisTime = OSTime::Milliseconds() + fTimeoutInMilSecs;
}

void TimeoutTask::RefreshTimeout() {
  fTimeoutAtThisTime = OSTime::Milliseconds() + fTimeoutInMilSecs;
  Assert(fTimeoutAtThisTime > 0);
}

SInt64 TimeoutTaskThread::Run() {
  EventFlags events = this->GetEvents(); // we must clear the event mask!
  if (events & Task::kKillEvent)
    return 0; // we will release later, not in TaskThread

  // ok, check for timeouts now. Go through the whole queue
  OSMutexLocker locker(&fMutex);
  SInt64 curTime = OSTime::Milliseconds();
  SInt64 intervalMilli = kIntervalSeconds * 1000; //always default to 60 seconds but adjust to smallest interval > 0
  SInt64 taskInterval = intervalMilli;

  for (OSQueueIter iter(&fQueue); !iter.IsDone(); iter.Next()) {
    TimeoutTask *theTimeoutTask =
        (TimeoutTask *) iter.GetCurrent()->GetEnclosingObject();

    // if it's time to time this task out, signal it
    if ((theTimeoutTask->fTimeoutAtThisTime > 0)
        && (curTime >= theTimeoutTask->fTimeoutAtThisTime)) {
#if TIMEOUT_DEBUGGING
      qtss_printf("TimeoutTask@%" _UPOINTERSIZEARG_ " timed out. "
                      "Curtime = %" _S64BITARG_ ", "
                      "timeout time = %" _S64BITARG_ "\n",
                  theTimeoutTask,
                  curTime,
                  theTimeoutTask->fTimeoutAtThisTime);
#endif
      if (theTimeoutTask->fTask != nullptr)
        theTimeoutTask->fTask->Signal(Task::kTimeoutEvent);
    } else {
      taskInterval = theTimeoutTask->fTimeoutAtThisTime - curTime;
      /* 更新 TimeoutTaskThread 的唤醒时间 */
      if ((taskInterval > 0) && (theTimeoutTask->fTimeoutInMilSecs > 0)
          && (intervalMilli > taskInterval))
        // set timeout to 1 second past this task's timeout
        intervalMilli = taskInterval + 1000;
#if TIMEOUT_DEBUGGING
      qtss_printf("TimeoutTask@%" _UPOINTERSIZEARG_ " not being timed out. "
                      "Curtime = %" _S64BITARG_ ". "
                      "timeout time = %" _S64BITARG_ "\n",
                  theTimeoutTask,
                  curTime,
                  theTimeoutTask->fTimeoutAtThisTime);
#endif
    }
  }

  OSThread::ThreadYield();

#if TIMEOUT_DEBUGGING
  qtss_printf("TimeoutTaskThread::Run interval seconds = %" _S32BITARG_ "\n",
              (SInt32)intervalMilli / 1000);
#endif

  /*
     注意：在 TaskThread::Entry 将 TimeoutTaskThread 项从线程的 fTaskQueue 里
     取出处理后，根据 Run 返回值，决定是否插入线程的 fHeap，而不会再次插入到
     fTaskQueue 里。如果插入 fHeap，在 TaskThread::WaitForTask 里会被得到处理。
   */
  return intervalMilli; // don't delete me!
}
