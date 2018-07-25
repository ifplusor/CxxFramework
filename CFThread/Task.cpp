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
 * @file Task.cpp
 *
 * Implements Task class
 */

#include <CF/Thread/Task.h>
#include <CF/Core/Time.h>

using namespace CF::Thread;

std::atomic_uint Task::sShortTaskThreadPicker(0);
std::atomic_uint Task::sBlockingTaskThreadPicker(0);

CF::Core::RWMutex TaskThreadPool::sRWMutex;
static char const *sTaskStateStr = "live_"; // Alive

Task::Task()
    : fEvents(0),
      fUseThisThread(nullptr),
      fDefaultThread(nullptr),
      fWriteLock(false),
      fTimerHeapElem(),
      fTaskQueueElem(),
      pickerToUse(&Task::sShortTaskThreadPicker) {
#if DEBUG_TASK
  fInRunCount = 0;
#endif
  this->SetTaskName("unknown");

  fTaskQueueElem.SetEnclosingObject(this);
  fTimerHeapElem.SetEnclosingObject(this);
}

void Task::SetTaskName(char const *name) {
  if (name == nullptr) return;

  ::strncpy(fTaskName, sTaskStateStr, sizeof(fTaskName) - 1);
  ::strncat(fTaskName, name, sizeof(fTaskName) - strlen(fTaskName) - 1);
  fTaskName[sizeof(fTaskName) - 1] = 0; //terminate in case it is longer than fTaskName.
}

bool Task::Valid() {
  if ((this->fTaskName == nullptr) ||
      (0 != ::strncmp(sTaskStateStr, this->fTaskName, 5))) {
    DEBUG_LOG(DEBUG_TASK, "Task::Valid Found invalid task = %p\n", (void *) this);
    return false;
  }
  return true;
}

void Task::Signal(EventFlags events) {
  /* 如果该任务有指定处理的线程，则将该任务加入到指定线程的任务队列中。
   * 如果没有指定的线程，则从线程池中随机选择一个任务线程，并将该任务加入到
   * 这个任务线程的任务队列中。
   * 或者干脆就没有线程运行，那么只是打印信息退出。*/

  if (!this->Valid()) return;

  // Fancy no Mutex implementation. We atomically mask the new events into
  // the event mask. Because atomic_or returns the old state of the mask,
  // we only schedule this task once.
  /* check point!!! 激活新的 event
   * 因为对 task 的“一次”调度会连续处理期间发生的“所有”事件，
   * 已经处于 alive 状态的 task 不会被重复调度。 */
  events |= kAlive;
  DEBUG_LOG(DEBUG_TASK,
            "%lld Task@%p::Signal: before fetch_or. events=0x%X fEvents=0x%X\n",
            Core::Time::Microseconds(), this, events, fEvents.load());
  auto oldEvents = fEvents.fetch_or(events);
  DEBUG_LOG(DEBUG_TASK,
            "%lld Task@%p::Signal: after fetch_or. oldEvents=0x%X fEvents=0x%X\n",
            Core::Time::Microseconds(), this, oldEvents, fEvents.load());
  if (!(oldEvents & kAlive)) {
    if (fDefaultThread != nullptr && fUseThisThread == nullptr)
      fUseThisThread = fDefaultThread;

    if (fUseThisThread != nullptr) {
      // Task needs to be placed on a particular Thread.

      if (DEBUG_TASK) {
        if (fTaskName[0] == 0) ::strcpy(fTaskName, " _Corrupt_Task");
      }

      DEBUG_LOG(DEBUG_TASK,
                "Task@%p::Signal: EnQueue. events=0x%X TaskName=%s fUseThisThread=%p q_elem=%p\n",
                this, events, fTaskName, fUseThisThread, &fTaskQueueElem);

      DEBUG_LOG(DEBUG_TASK && TaskThreadPool::sTaskThreadArray[0] == fUseThisThread,
                "Task@%p::Signal: RTSP Thread running.\n",
                this);

      fUseThisThread->fTaskQueue.EnQueue(&fTaskQueueElem);
    } else {
      if (TaskThreadPool::sNumTaskThreads <= 0) {
        DEBUG_LOG(DEBUG_TASK,
                  "Task@%p::Signal: no task thread. events=0x%X TaskName=%s\n",
                  this, events, fTaskName);
        return;
      }

      // find a Thread to put this task on
      unsigned int theThreadIndex = pickerToUse->fetch_add(1);

      if (&Task::sShortTaskThreadPicker == pickerToUse) {
        theThreadIndex %= TaskThreadPool::sNumShortTaskThreads;

        DEBUG_LOG(DEBUG_TASK,
                  "Task@%p::Signal: EnQueue using ShortPicker. events=0x%X TaskName=%s picker[%u]=%u index=%u\n",
                  this, events, fTaskName, TaskThreadPool::sNumShortTaskThreads, Task::sShortTaskThreadPicker.load(), theThreadIndex);
      } else if (&Task::sBlockingTaskThreadPicker == pickerToUse) {
        theThreadIndex %= TaskThreadPool::sNumBlockingTaskThreads;
        theThreadIndex += TaskThreadPool::sNumShortTaskThreads;
        //don't pick from lower non-blocking (short task) threads.

        DEBUG_LOG(DEBUG_TASK,
                  "Task@%p::Signal: EnQueue using BlockingPicker. events=0x%X TaskName=%s picker[%u]=%u index=%u\n",
                  this, events, fTaskName, TaskThreadPool::sNumBlockingTaskThreads, Task::sBlockingTaskThreadPicker.load(), theThreadIndex);
      } else {
        if (DEBUG_TASK) {
          if (fTaskName[0] == 0) ::strcpy(fTaskName, " _Corrupt_Task");
        }

        DEBUG_LOG(DEBUG_TASK,
                  "Task@%p::Signal: invalid picker. events=0x%X TaskName=%s\n",
                  this, events, fTaskName);

        return;
      }

      if (DEBUG_TASK) {
        if (fTaskName[0] == 0) ::strcpy(fTaskName, " _Corrupt_Task");
      }


      DEBUG_LOG(DEBUG_TASK,
                "Task@%p::Signal: EnQueue B. Thread=%p fTaskQueue.GetLength(%" _U32BITARG_ ") q_elem=%p\n",
                this, TaskThreadPool::sTaskThreadArray[theThreadIndex],
                TaskThreadPool::sTaskThreadArray[theThreadIndex]->fTaskQueue.GetQueue()->GetLength(), &fTaskQueueElem);

      // 将任务压入 TaskThread 的就绪队列
      TaskThreadPool::sTaskThreadArray[theThreadIndex]->fTaskQueue.EnQueue(&fTaskQueueElem);

      DEBUG_LOG(DEBUG_TASK,
                "Task@%p::Signal: EnQueue A. Thread=%p fTaskQueue.GetLength(%" _U32BITARG_ ")\n",
                this, TaskThreadPool::sTaskThreadArray[theThreadIndex],
                TaskThreadPool::sTaskThreadArray[theThreadIndex]->fTaskQueue.GetQueue()->GetLength());
    }
  } else {
    DEBUG_LOG(DEBUG_TASK,
              "Task@%p::Signal: append to alive task. events=0x%X TaskName=%s\n",
              this, events, fTaskName);
  }
}

void Task::GlobalUnlock() {
  if (this->fWriteLock) {
    this->fWriteLock = false;
    TaskThreadPool::sRWMutex.Unlock();
  }
}

void Task::SetThreadPicker(std::atomic_uint *picker) {
  pickerToUse = picker;
  Assert(pickerToUse != nullptr);
  if (DEBUG_TASK) {
    if (fTaskName[0] == 0) ::strcpy(fTaskName, " _Corrupt_Task");

    if (&Task::sShortTaskThreadPicker == pickerToUse) {
      s_printf("Task::SetThreadPicker sShortTaskThreadPicker for task=%s\n", fTaskName);
    } else if (&Task::sBlockingTaskThreadPicker == pickerToUse) {
      s_printf("Task::SetThreadPicker sBlockingTaskThreadPicker for task=%s\n", fTaskName);
    } else {
      s_printf("Task::SetThreadPicker ERROR unknown picker for task=%s\n", fTaskName);
    }
  }
}

void Task::ForceSameThread() {
  fUseThisThread = (TaskThread *) Core::Thread::GetCurrent();
  Assert(fUseThisThread != nullptr);
  if (DEBUG_TASK) {
    if (fTaskName[0] == 0) ::strcpy(fTaskName, " corrupt task");
  }
  DEBUG_LOG(DEBUG_TASK,
            "Task::ForceSameThread fUseThisThread %p task %s enqueue elem=%p enclosing %p\n",
            fUseThisThread, fTaskName, &fTaskQueueElem, this);
}

/**
 * Task::Run 执行期间必须调用 GetEvents 获取已触发事件，同时清除已获得的事件标志。
 * 但同样因为 GetEvents 会清除旧事件，返回的事件必须被同时处理，否则就会丢失
 */
Task::EventFlags Task::GetEvents() {
  // Mask off every event currently in the mask except for the alive bit,
  // of course, which should remain unaffected and unreported by this call.
  EventFlags events = fEvents.fetch_and(kAlive);
  return events;
}

/**
 * 任务线程入口，由一个大循环构成
 */
void TaskThread::Entry() {
  while (true) {
    /* 等待任务的通知到达,或者因 stop 的请求而返回(目前,WaitForTask 只有在收到
       stop 请求后 才返回 NULL)。 */
    Task *theTask = this->WaitForTask();

    //
    // WaitForTask returns nullptr when it is Time to quit
    if (theTask == nullptr || !theTask->Valid())
      return;

    bool doneProcessingEvent = false;

    /* 下面也是一个循环,如果 doneProcessingEvent 为 true 则跳出循环。
     * OSMutexWriteLocker、OSMutexReadLocker 均基于 OSMutexReadWriteLocker 类,
     * 在下面的使用中这两个类构建函数会调用 sMutexRW->LockRead 或
     * sMutexRW->LockWrite 进行互斥的读写操作。 OSMutexRW 类对象 sMutexRW 为
     * TaskThreadPool 类的成员,所以是对所有的线程互斥。 */
    while (!doneProcessingEvent) {
      // If a task holds locks when it returns from its Run function,
      // that would be catastrophic and certainly lead to a deadlock
#if DEBUG_TASK
//      Assert(this->GetNumLocksHeld() == 0);
      Assert(theTask->fInRunCount == 0);
      theTask->fInRunCount++;
#endif
      theTask->fUseThisThread = nullptr; // Each invocation of Run must independently
      // request a specific Thread.
      SInt64 theTimeout = 0;

      if (theTask->fWriteLock) {
        Core::MutexWriteLocker mutexLocker(&TaskThreadPool::sRWMutex);
        DEBUG_LOG(DEBUG_TASK,
                  "TaskThread::Entry run global locked TaskName=%s CurMSec=%.3f Thread=%p task=%p\n",
                  theTask->fTaskName, Core::Time::StartTimeMilli_Float(), this, theTask);

        theTimeout = theTask->Run();
        theTask->fWriteLock = false;
      } else {
        Core::MutexReadLocker mutexLocker(&TaskThreadPool::sRWMutex);
        DEBUG_LOG(DEBUG_TASK,
                  "TaskThread::Entry run TaskName=%s CurMSec=%.3f Thread=%p task=%p\n",
                  theTask->fTaskName, Core::Time::StartTimeMilli_Float(), this, theTask);

        theTimeout = theTask->Run();
      }
#if DEBUG
      Assert(this->GetNumLocksHeld() == 0);
      theTask->fInRunCount--;
      Assert(theTask->fInRunCount == 0);
#endif
      if (theTimeout < 0) {
        /* 如果 theTimeout < 0,
         *  则说明任务结束，任务对象被销毁，doneProcessingEvent 设为 true，
         *  在后面调用 ThreadYield 后，跳出这个循环。再调用 WaitForTask，
         *  等待下个处理。 */

        if (DEBUG_TASK) {
          s_printf("TaskThread::Entry delete TaskName=%s CurMSec=%.3f Thread=%p task=%p\n",
                   theTask->fTaskName, Core::Time::StartTimeMilli_Float(), this, theTask);

          theTask->fUseThisThread = nullptr;

          if (nullptr != fHeap.Remove(&theTask->fTimerHeapElem))
            s_printf("TaskThread::Entry task still in Heap before delete\n");

          if (nullptr != theTask->fTaskQueueElem.InQueue())
            s_printf("TaskThread::Entry task still in Queue before delete\n");

          theTask->fTaskQueueElem.Remove();

          if (theTask->fEvents & ~Task::kAlive)
            s_printf("TaskThread::Entry flags still set  before delete\n");

          theTask->fEvents.fetch_sub(0); // ?

          ::strncat(theTask->fTaskName, " deleted",
                    sizeof(theTask->fTaskName) - 1);
        }

        /* check point!!! Mark as dead, then delete it.
         * 在该点，task 仍具有 alive 标记，即使其它线程调用 Signal，
         * task 也不会重复进入调度器 */
        theTask->fTaskName[0] = 'D';
        delete theTask;
        doneProcessingEvent = true;
      } else if (theTimeout == 0) {
        /* 如果 theTimeout == 0,
         *  将 theTask->fEvents 和 Task::kAlive 比较，如果返回 1，则说明该任务
         *  已经没有事件处理，反之则在 ThreadYield 返回后继续在循环里处理这个
         *  任务的事件。 */

        // We want to make sure that 100% definitely the task's Run function WILL
        // be invoked when another Thread calls Signal. We also want to make sure
        // that if an event sneaks in right as the task is returning from Run()
        // (via Signal) that the Run function will be invoked again.
        /* check point!!! Task 处理期间未激活新的 event，则撤销 alive 状态 */
        unsigned int val = Task::kAlive; // 因为 cas 失败会改变该值，所以不能声明为 static/const
        DEBUG_LOG(DEBUG_TASK,
                  "%lld TaskThread@%p::Entry: before cas, task@%p events is 0x%X\n",
                  Core::Time::Microseconds(), this, theTask, theTask->fEvents.load());
        doneProcessingEvent = theTask->fEvents.compare_exchange_weak(val, 0);
        if (doneProcessingEvent) {
          DEBUG_LOG(DEBUG_TASK,
                    "%lld TaskThread@%p::Entry: after cas succeed, task@%p events is 0x%X\n",
                    Core::Time::Microseconds(), this, theTask, theTask->fEvents.load());
        } else {
          DEBUG_LOG(DEBUG_TASK,
                    "%lld TaskThread@%p::Entry: after cas failed, task@%p events is 0x%X, oldEvents is 0x%X\n",
                    Core::Time::Microseconds(), this, theTask, theTask->fEvents.load(), val);
        }

        /* 虽然该任务目前没有事件处理，但是并不表示要销毁，所以没有 delete。 */
      } else {
        /* 如果 theTimeout > 0,
         *  则说明任务希望等待 theTimeout 时间后得到处理。*/

        if (theTimeout < kMinWaitTimeInMilSecs)
          theTimeout = kMinWaitTimeInMilSecs;

        // note that if we get here, we don't reset theTask, so it will get
        // passed into WaitForTask
        DEBUG_LOG(DEBUG_TASK,
                  "TaskThread::Entry insert TaskName=%s in timer Heap Thread=%p elem=%p task=%p timeout=%.2lf\n",
                   theTask->fTaskName, this, &theTask->fTimerHeapElem, theTask, theTimeout / 1000.0);
        theTask->fTimerHeapElem.SetValue(
            Core::Time::Milliseconds() + theTimeout);
        fHeap.Insert(&theTask->fTimerHeapElem);
        /* check point!!! 激活 kIdleEvent，保持 alive 状态 */
        theTask->fEvents.fetch_or(Task::kIdleEvent);
        doneProcessingEvent = true;
      }

#if DEBUG_TASK
      SInt64 yieldStart = Core::Time::Milliseconds();
#endif

      /* 对于 linux 系统来说，ThreadYield 实际上没有做什么工作 */
      ThreadYield();

#if DEBUG_TASK
      SInt64 yieldDur = Core::Time::Milliseconds() - yieldStart;
      static SInt64 numZeroYields;

      if (yieldDur > 1) {
        if (DEBUG_TASK)
          s_printf(
              "TaskThread::Entry Time in Yield %qd, numZeroYields %qd \n",
              yieldDur,
              numZeroYields);
        numZeroYields = 0;
      } else
        numZeroYields++;
#endif
    }

    DEBUG_LOG(DEBUG_TASK, "TaskThread@%p::Entry: task@%p is done\n", this, theTask);
  }
}

Task *TaskThread::WaitForTask() {
  /* 该函数同样由一个大循环构成。等待任务的通知到达,或者因 stop 的请求而返回。 */

  while (true) {
    SInt64 theCurrentTime = Core::Time::Milliseconds();

    /* 如果堆里有时间记录，并且这个时间<=系统当前时间（说明任务的运行时间已
     * 经到了），则返回该记录所对应的任务对象 */

    /* PeekMin 获得堆中的第一个元素（但并不取出） */
    if ((fHeap.PeekMin() != nullptr) &&
        (fHeap.PeekMin()->GetValue() <= theCurrentTime)) {
      DEBUG_LOG(DEBUG_TASK,
                "TaskThread::WaitForTask found timer-task=%s Thread=%p "
                "fHeap.CurrentHeapSize(%" _U32BITARG_ ") taskElem=%p enclose=%p\n",
                ((Task *) fHeap.PeekMin()->GetEnclosingObject())->fTaskName, this,
                fHeap.CurrentHeapSize(), fHeap.PeekMin(), fHeap.PeekMin()->GetEnclosingObject());
      return (Task *) fHeap.ExtractMin()->GetEnclosingObject();
    }

    // if there is an element waiting for a timeout, figure out how long we
    // should wait.
    SInt64 theTimeout = 0;
    if (fHeap.PeekMin() != nullptr)
      theTimeout = fHeap.PeekMin()->GetValue() - theCurrentTime;
    Assert(theTimeout >= 0);

    //
    // Make sure we can't go to sleep for some ridiculously short period of Time
    // Do not allow a timeout below 10 ms without first verifying reliable udp
    // 1-2mbit live streams.
    // Test with easydarwin.xml pref reliablUDP printfs enabled and look for
    // packet loss and check client for buffer ahead recovery.
    if (theTimeout < 10)
      theTimeout = 10;

    // wait...
    /* TaskThread 类有一个 OSQueue_Blocking 类的私有成员 fTaskQueue。
     * 等待队列里有任务插入并将其取出返回。
     * 如果返回非空,则返回该队列项所对应的任务对象。 */
    QueueElem *theElem = fTaskQueue.DeQueueBlocking(this, (SInt32) theTimeout);
    if (theElem != nullptr) {
      DEBUG_LOG(DEBUG_TASK,
                "TaskThread::WaitForTask found signal-task=%s Thread=%p "
                "fTaskQueue.GetLength(%" _U32BITARG_ ") taskElem=%p enclose=%p\n",
                ((Task*) theElem->GetEnclosingObject())->fTaskName, this,
                fTaskQueue.GetQueue()->GetLength(), theElem, theElem->GetEnclosingObject());
      return (Task *) theElem->GetEnclosingObject();
    }

    // If we are supposed to stop, return nullptr, which signals the caller to stop
    if (Core::Thread::GetCurrent()->IsStopRequested())
      return nullptr;
  }
}

TaskThread **TaskThreadPool::sTaskThreadArray = nullptr;
UInt32       TaskThreadPool::sNumTaskThreads = 0;
UInt32       TaskThreadPool::sNumShortTaskThreads = 0;
UInt32       TaskThreadPool::sNumBlockingTaskThreads = 0;

bool TaskThreadPool::CreateThreads(UInt32 numShortTaskThreads,
                                   UInt32 numBlockingThreads) {
  /*
     根据 numToAdd 参数创建 TaskThread 类对象, 并调用该类的 Start 成员函数。
     将该类对象指针保存到 sTaskThreadArray 数组。
   */

  Assert(sTaskThreadArray == nullptr);
  UInt32 numToAdd = numShortTaskThreads + numBlockingThreads;
  sTaskThreadArray = new TaskThread *[numToAdd];

  for (UInt32 x = 0; x < numToAdd; x++) {
    sTaskThreadArray[x] = new TaskThread();
    sTaskThreadArray[x]->Start();
    DEBUG_LOG(DEBUG_TASK,
              "TaskThreadPool::AddThreads sTaskThreadArray[%" _U32BITARG_ "]=%p\n",
              x, sTaskThreadArray[x]);
  }

  sNumShortTaskThreads = numShortTaskThreads;
  sNumBlockingTaskThreads = numBlockingThreads;
  sNumTaskThreads = numToAdd;

  if (0 == sNumShortTaskThreads)
    sNumShortTaskThreads = numToAdd;

  return true;
}

TaskThread *TaskThreadPool::GetThread(UInt32 index) {
  Assert(sTaskThreadArray != nullptr);
  if (index >= sNumTaskThreads) return nullptr;
  return sTaskThreadArray[index];
}

void TaskThreadPool::RemoveThreads() {
  // Tell all the threads to stop
  for (UInt32 x = 0; x < sNumTaskThreads; x++)
    sTaskThreadArray[x]->SendStopRequest();

  // Because any (or all) threads may be blocked on the Queue, cycle through
  // all the threads, signalling each one
  for (UInt32 y = 0; y < sNumTaskThreads; y++)
    sTaskThreadArray[y]->fTaskQueue.GetCond()->Signal();

  // Ok, now wait for the selected threads to terminate, deleting them and
  // removing them from the Queue.
  for (UInt32 z = 0; z < sNumTaskThreads; z++)
    delete sTaskThreadArray[z];

  delete[] sTaskThreadArray;

  sNumTaskThreads = 0;
}
