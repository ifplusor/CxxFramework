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
    File:       OSQueue.h

    Contains:   implements OSQueue class

*/

#ifndef __CF_QUEUE_H__
#define __CF_QUEUE_H__

#include <CF/Types.h>

#define CF_QUEUE_TESTING 0

namespace CF {

class Queue;

class QueueElem {
 public:
  QueueElem(void *enclosingObject = nullptr)
      : fNext(nullptr), fPrev(nullptr), fQueue(nullptr),
        fEnclosingObject(enclosingObject) {}

  virtual ~QueueElem();

  bool IsMember(const Queue &queue) { return (&queue == fQueue); }

  bool IsMemberOfAnyQueue() { return fQueue != nullptr; }

  void *GetEnclosingObject() { return fEnclosingObject; }

  void SetEnclosingObject(void *obj) { fEnclosingObject = obj; }

  QueueElem *Next() { return fNext; }

  QueueElem *Prev() { return fPrev; }

  Queue *InQueue() { return fQueue; }

  inline void Remove();

 private:

  QueueElem *fNext;
  QueueElem *fPrev;
  Queue *fQueue;
  void *fEnclosingObject;

  friend class Queue;
};

/**
 * @brief 队列，使用带哨兵的链表实现
 * @note 只保存 OSQueueElem 对象指针，不管理对象内存
 */
class Queue {
 public:
  Queue();

  ~Queue() {}

  virtual void EnQueue(QueueElem *object);

  virtual QueueElem *DeQueue();

  virtual void Remove(QueueElem *object);

  QueueElem *GetHead() {
    if (fLength > 0) return fSentinel.fPrev;
    return nullptr;
  }

  QueueElem *GetTail() {
    if (fLength > 0) return fSentinel.fNext;
    return nullptr;
  }

  UInt32 GetLength() { return fLength; }

#if CF_QUEUE_TESTING
  static bool       Test();
#endif

 protected:
  QueueElem fSentinel; /* 哨兵节点 */
  UInt32 fLength;
};

class QueueIter {
 public:
  QueueIter(Queue *inQueue)
      : fQueueP(inQueue), fCurrentElemP(inQueue->GetHead()) {}

  QueueIter(Queue *inQueue, QueueElem *startElemP);

  ~QueueIter() {}

  void Reset() { fCurrentElemP = fQueueP->GetHead(); }

  QueueElem *GetCurrent() { return fCurrentElemP; }

  void Next();

  bool IsDone() { return fCurrentElemP == nullptr; }

 private:

  Queue *fQueueP;
  QueueElem *fCurrentElemP;
};

void QueueElem::Remove() {
  if (fQueue != nullptr)
    fQueue->Remove(this);
}

}

#endif // __CF_QUEUE_H__
