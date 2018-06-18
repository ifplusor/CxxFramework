//
// Created by James on 2017/8/30.
//

#include <CF/ConcurrentQueue.h>

using namespace CF;

CF::QueueElem *BlockingQueue::
DeQueueBlocking(Core::Thread *inCurThread, SInt32 inTimeoutInMilSecs) {
  Core::MutexLocker theLocker(&fMutex);
  /* 如果 fQueue.GetLength() == 0,则调用 fCond.Wait 即调用 pthread_cond_timedwait
   * 等待条件变量有效 */
#ifdef __Win32_
  if (fQueue.GetLength() == 0) {
      fCond.Wait(&fMutex, inTimeoutInMilSecs);
      return nullptr;
  }
#else
  if (fQueue.GetLength() == 0)
    fCond.Wait(&fMutex, inTimeoutInMilSecs);
#endif

  /*
     fCond.wait 返回或者 fQueue.GetLength() != 0,调用 fQueue.DeQueue 返回队列
     里的任务对象。这里要注意一点,pthread_cond_timedwait 可能只是超时退出,所以
     fQueue.DeQueue 可能只是返回空指针。
   */
  QueueElem *retval = fQueue.DeQueue();
  return retval;
}

CF::QueueElem *BlockingQueue::DeQueue() {
  Core::MutexLocker theLocker(&fMutex);
  QueueElem *retval = fQueue.DeQueue();
  return retval;
}

void BlockingQueue::EnQueue(QueueElem *obj) {
  {
    Core::MutexLocker theLocker(&fMutex);
    // 调用 OSQueue 类成员 fQueue.EnQueue 函数
    fQueue.EnQueue(obj);
  }
  // 调用 OSCond 类成员 fCond.Signal 函数即调用 pthread_cond_signal(&fCondition);
  fCond.Signal();
}

