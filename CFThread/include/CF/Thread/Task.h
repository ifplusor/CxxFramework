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
	File:       Task.h

	Contains:   Tasks are objects that can be scheduled. To schedule a task,
				you call its signal method, and pass in an event (events are
				bits and all events are defined below).

				Once Signal() is called, the task object will be scheduled.
                When it runs, its Run() function will get called. In order to
                clear the event, the derived task object must call GetEvents()
                (which returns the events that were sent).

				Calling GetEvents() implicitly "clears" the events returned.
				All events must be cleared before the Run() function returns,
				or Run() will be invoked again immediately.

*/

#ifndef __TASK_H__
#define __TASK_H__

#include <atomic>
#include <CF/Heap.h>
#include <CF/ConcurrentQueue.h>
#include <CF/Core/RWMutex.h>

#define DEBUG_TASK 0

namespace CF {
namespace Thread {

class TaskThread;

class Task {
 public:

  typedef unsigned int EventFlags;

  // EVENTS
  // here are all the events that can be sent to a task
  enum {
    // these are all of type "EventFlags"
    kKillEvent = 0x1 << 0x0,
    kIdleEvent = 0x1 << 0x1,
    kStartEvent = 0x1 << 0x2,
    kTimeoutEvent = 0x1 << 0x3,

    /* Socket events */
    kReadEvent = 0x1 << 0x4,
    kWriteEvent = 0x1 << 0x5,

    /* update event */
    kUpdateEvent = 0x1 << 0x6
  };

  // CONSTRUCTOR / DESTRUCTOR
  // You must assign priority at create Time.
  Task();

  virtual ~Task() {}

  /**
   * @return >0 invoke me after this number of MilSecs with a kIdleEvent
   * @return  0 don't reinvoke me at all.
   * @return -1 delete me
   *
   * Suggested practice is that any task should be deleted by returning true
   * from the Run function. That way, we know that the Task is not running at
   * the time it is deleted. This object provides no protection against calling
   * a method, such as Signal, at the same time the object is being deleted
   * (because it can't really), so watch those dangling references!
   */
  virtual SInt64 Run() = 0;

  // Send an event to this task.
  void Signal(EventFlags eventFlags);

  void GlobalUnlock();

  bool Valid(); // for debugging
  char fTaskName[48];

  void SetTaskName(char *name);

  void SetDefaultThread(TaskThread *defaultThread) {
    fDefaultThread = defaultThread;
  }

  void SetThreadPicker(unsigned int *picker);

  static unsigned int *GetBlockingTaskThreadPicker() {
    return &sBlockingTaskThreadPicker;
  }

 protected:

  // Only the tasks themselves may find out what events they have received
  EventFlags GetEvents();

  /**
   * A task, inside its run function, may want to ensure that the same task
   * thread is used for subsequent calls to Run(). This may be the case if the
   * task is holding a mutex between calls to run. By calling this function,
   * the task ensures that the same task thread will be used for the next call
   * to Run(). It only applies to the next call to run.
   */
  void ForceSameThread();

  SInt64 CallLocked() {
    ForceSameThread();
    fWriteLock = true;
    return (SInt64) 10; // minimum of 10 milliseconds between locks
  }

 private:

  enum {
    kAlive = 0x80000000,   // EventFlags, again
    kAliveOff = 0x7fffffff
  };

  void SetTaskThread(TaskThread *thread);

  /*
     当事件发生时，Task 进入调度队列，并设置相应的 event flag。
     Task 进入调度队列时设置 alive 标志位，执行完毕后撤销 alive 标志位。
     Task 在某一时刻，只会处于唯一调度队列。
   */
  std::atomic<EventFlags> fEvents;

  TaskThread *fUseThisThread; /* 强制执行线程 */
  TaskThread *fDefaultThread; /* 默认执行线程 */
  bool fWriteLock;

#if DEBUG_TASK
  // The whole premise of a task is that the Run function cannot be re-entered.
  // This debugging variable ensures that that is always the case
  volatile UInt32 fInRunCount;
#endif

  // This could later be optimized by using a timing wheel instead of a Heap,
  // and that way we wouldn't need both a Heap elem and a Queue elem here (just Queue elem)
  HeapElem fTimerHeapElem;
  QueueElem fTaskQueueElem;

  unsigned int *pickerToUse;

  Core::Mutex fAtomicMutex;

  // Variable used for assigning tasks to threads in a round-robin fashion
  static unsigned int sShortTaskThreadPicker; // default picker
  static unsigned int sBlockingTaskThreadPicker;

  friend class TaskThread;
};

/**
 * @brief 任务执行线程，非抢占式任务调度
 */
class TaskThread : public Core::Thread {
 public:

  // Implementation detail: all tasks get run on TaskThreads.

  TaskThread() : Thread(), fTaskThreadPoolElem() {
    fTaskThreadPoolElem.SetEnclosingObject(this);
  }

  virtual ~TaskThread() { this->StopAndWaitForThread(); }

 private:

  enum {
    kMinWaitTimeInMilSecs = 10  //UInt32
  };

  virtual void Entry();

  Task *WaitForTask();

  QueueElem fTaskThreadPoolElem;

  // use heap for time-sequence task, only in TaskThread, not concurrent.
  Heap fHeap;                     /* 时序-优先队列 */
  BlockingQueue fTaskQueue; /* 事件-触发队列 */

  friend class Task;
  friend class TaskThreadPool;
};

/**
 * @brief 任务线程池静态类，该类负责生成、删除以及维护内部的任务线程列表。
 *
 * Because task threads share a global queue of tasks to execute,
 * there can only be one pool of task threads. That is why this object
 * is static.
 */
class TaskThreadPool {
 public:

  /**
   * @brief Adds some threads to the pool
   *
   * creates the threads: takes NumShortTaskThreads + NumBLockingThreads,
   * sets num short task threads.
   */
  static bool AddThreads(UInt32 numToAdd);

  static void RemoveThreads();

  static TaskThread *GetThread(UInt32 index);

  static UInt32 GetNumThreads() { return sNumTaskThreads; }

  static void SetNumShortTaskThreads(UInt32 numToAdd) {
    sNumShortTaskThreads = numToAdd;
  }

  static void SetNumBlockingTaskThreads(UInt32 numToAdd) {
    sNumBlockingTaskThreads = numToAdd;
  }

 private:
  TaskThreadPool() = default;

  static TaskThread **sTaskThreadArray;
  static UInt32 sNumTaskThreads;
  static UInt32 sNumShortTaskThreads;
  static UInt32 sNumBlockingTaskThreads;

  static Core::RWMutex sRWMutex;

  friend class Task;
  friend class TaskThread;
};

} // namespace Task
} // namespace CF

#endif
