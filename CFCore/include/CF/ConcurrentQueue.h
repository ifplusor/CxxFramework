//
// Created by James on 2017/8/30.
//

#ifndef __CF_BLOCKING_QUEUE_H__
#define __CF_BLOCKING_QUEUE_H__

#include <CF/Queue.h>
#include <CF/Core/Thread.h>
#include <CF/Core/Cond.h>

namespace CF {

class ConcurrentQueue : public Queue {
 public:
  void EnQueue(QueueElem *elem) override {
    Core::MutexLocker theLocker(&fMutex);
    Queue::EnQueue(elem);
  }

  QueueElem *DeQueue() override {
    Core::MutexLocker theLocker(&fMutex);
    return Queue::DeQueue();
  }

  void Remove(QueueElem *elem) {
    Core::MutexLocker theLocker(&fMutex);
    Queue::Remove(elem);
  }

 protected:
  Core::Mutex fMutex;
};

/**
 * 该类用作 TaskThread 的私有成员类,实际上是利用线程的条件变量实现了一个可等
 * 待唤醒的队列操作。
 */
class BlockingQueue {
 public:
  BlockingQueue() {}

  ~BlockingQueue() {}

  QueueElem *DeQueueBlocking(Core::Thread *inCurThread,
                             SInt32 inTimeoutInMilSecs);

  QueueElem *DeQueue(); //will not block
  void EnQueue(QueueElem *obj);

  Core::Cond *GetCond() { return &fCond; }

  Queue *GetQueue() { return &fQueue; }

 private:

  Core::Cond fCond;
  Core::Mutex fMutex;
  Queue fQueue;
};

}

#endif //__CF_BLOCKING_QUEUE_H__
