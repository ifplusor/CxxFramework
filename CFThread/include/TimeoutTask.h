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
    File:       TimeoutTask.h

    Contains:   Just like a normal task, but can be scheduled for timeouts.
                Unlike IdleTask, which is VERY aggressive about being on time,
                but high overhead for maintaining the timing information, this
                is a low overhead, low priority timing mechanism. Timeouts may
                not happen exactly when they are supposed to, but who cares?

*/

#ifndef __TIMEOUT_TASK_H__
#define __TIMEOUT_TASK_H__

#include "IdleTask.h"

//messages to help debugging timeouts
#define TIMEOUT_DEBUGGING 1

/**
 * @brief TimeoutTask 守护线程
 *
 * TimeoutTaskThread 是 IdleTask 的派生类, IdleTask 是 Task 的派生类。
 * 本质上，TimeoutTaskThread 并不是一个线程类，而是一个基于 Task 类的
 * 任务类，它配合 TimeoutTask 类实现一个周期性运行的基础任务。
 */
class TimeoutTaskThread : public IdleTask {
 public:

  //All timeout tasks get timed out from this thread
  TimeoutTaskThread() : IdleTask(), fMutex() {
    this->SetTaskName("TimeoutTask");
  }

  virtual ~TimeoutTaskThread() {}

 private:

  // this thread runs every minute and checks for timeouts
  enum {
    kIntervalSeconds = 15   //UInt32
  };

  virtual SInt64 Run() override;

  OSMutex fMutex;
  OSQueue fQueue; /* 列表 */

  friend class TimeoutTask;
};

/**
 * @brief 超时通知
 *
 * TimeoutTask is not a derived object off of Task, to add flexibility as
 * to how this object can be utilitized
 */
class TimeoutTask {
 public:

  /**
   * @brief construct TimeoutTaskThread.
   * @note Call Initialize before using this class
   */
  static void Initialize();

  static void StopTask() {
    if (sThread != nullptr) {
      sThread->CancelTimeout();
      sThread->Signal(Task::kKillEvent);
    }
  }

  static void Release() {
    if (sThread != nullptr) {
      delete sThread;
      sThread = nullptr;
    }
  }

  // Pass in the task you'd like to send timeouts to.
  // Also pass in the timeout you'd like to use. By default,
  // the timeout is 0 (NEVER).
  TimeoutTask(Task *inTask, SInt64 inTimeoutInMilSecs = 15);

  ~TimeoutTask();

  //MODIFIERS

  // Changes the timeout time, also refreshes the timeout
  void SetTimeout(SInt64 inTimeoutInMilSecs);

  // Specified task will get a Task::kTimeoutEvent if this
  // function isn't called within the timeout period
  void RefreshTimeout();

  void SetTask(Task *inTask) { fTask = inTask; }

 private:

  Task *fTask;
  SInt64 fTimeoutAtThisTime;
  SInt64 fTimeoutInMilSecs;
  //for putting on our global queue of timeout tasks
  OSQueueElem fQueueElem;

  static TimeoutTaskThread *sThread;

  friend class TimeoutTaskThread;
};

#endif //__TIMEOUT_TASK_H__

