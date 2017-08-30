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
    File:       OSQueue.cpp

    Contains:   implements OSQueue class

*/

#include <CF/Queue.h>

using namespace CF;

Queue::Queue() : fLength(0) {
  // self-loop by sentinel
  fSentinel.fNext = &fSentinel;
  fSentinel.fPrev = &fSentinel;
}

void Queue::EnQueue(QueueElem *elem) {
  Assert(elem != nullptr);
  if (elem->fQueue == this)
    return;
  Assert(elem->fQueue == nullptr);
  elem->fNext = fSentinel.fNext;
  elem->fPrev = &fSentinel;
  elem->fQueue = this;
  fSentinel.fNext->fPrev = elem;
  fSentinel.fNext = elem;
  fLength++;
}

QueueElem *Queue::DeQueue() {
  if (fLength > 0) {
    QueueElem *elem = fSentinel.fPrev;
    Assert(fSentinel.fPrev != &fSentinel);
    elem->fPrev->fNext = &fSentinel;
    fSentinel.fPrev = elem->fPrev;
    elem->fQueue = nullptr;
    fLength--;
    return elem;
  }

  return nullptr;
}

void Queue::Remove(QueueElem *elem) {
  Assert(elem != nullptr);
  Assert(elem != &fSentinel);

  if (elem->fQueue == this) {
    elem->fNext->fPrev = elem->fPrev;
    elem->fPrev->fNext = elem->fNext;
    elem->fQueue = nullptr;
    fLength--;
  }
}

#if CF_QUEUE_TESTING
bool Queue::Test() {
  Queue theVictim;
  void *x = (void *) 1;
  QueueElem theElem1(x);
  x = (void *) 2;
  QueueElem theElem2(x);
  x = (void *) 3;
  QueueElem theElem3(x);

  if (theVictim.GetHead() != nullptr)
    return false;
  if (theVictim.GetTail() != nullptr)
    return false;

  theVictim.EnQueue(&theElem1);
  if (theVictim.GetHead() != &theElem1)
    return false;
  if (theVictim.GetTail() != &theElem1)
    return false;

  QueueElem *theElem = theVictim.DeQueue();
  if (theElem != &theElem1)
    return false;

  if (theVictim.GetHead() != nullptr)
    return false;
  if (theVictim.GetTail() != nullptr)
    return false;

  theVictim.EnQueue(&theElem1);
  theVictim.EnQueue(&theElem2);

  if (theVictim.GetHead() != &theElem1)
    return false;
  if (theVictim.GetTail() != &theElem2)
    return false;

  theElem = theVictim.DeQueue();
  if (theElem != &theElem1)
    return false;

  if (theVictim.GetHead() != &theElem2)
    return false;
  if (theVictim.GetTail() != &theElem2)
    return false;

  theElem = theVictim.DeQueue();
  if (theElem != &theElem2)
    return false;

  theVictim.EnQueue(&theElem1);
  theVictim.EnQueue(&theElem2);
  theVictim.EnQueue(&theElem3);

  if (theVictim.GetHead() != &theElem1)
    return false;
  if (theVictim.GetTail() != &theElem3)
    return false;

  theElem = theVictim.DeQueue();
  if (theElem != &theElem1)
    return false;

  if (theVictim.GetHead() != &theElem2)
    return false;
  if (theVictim.GetTail() != &theElem3)
    return false;

  theElem = theVictim.DeQueue();
  if (theElem != &theElem2)
    return false;

  if (theVictim.GetHead() != &theElem3)
    return false;
  if (theVictim.GetTail() != &theElem3)
    return false;

  theElem = theVictim.DeQueue();
  if (theElem != &theElem3)
    return false;

  theVictim.EnQueue(&theElem1);
  theVictim.EnQueue(&theElem2);
  theVictim.EnQueue(&theElem3);

  QueueIter theIterVictim(&theVictim);
  if (theIterVictim.IsDone())
    return false;
  if (theIterVictim.GetCurrent() != &theElem3)
    return false;
  theIterVictim.Next();
  if (theIterVictim.IsDone())
    return false;
  if (theIterVictim.GetCurrent() != &theElem2)
    return false;
  theIterVictim.Next();
  if (theIterVictim.IsDone())
    return false;
  if (theIterVictim.GetCurrent() != &theElem1)
    return false;
  theIterVictim.Next();
  if (!theIterVictim.IsDone())
    return false;
  if (theIterVictim.GetCurrent() != nullptr)
    return false;

  theVictim.Remove(&theElem1);

  if (theVictim.GetHead() != &theElem2)
    return false;
  if (theVictim.GetTail() != &theElem3)
    return false;

  theVictim.Remove(&theElem1);

  if (theVictim.GetHead() != &theElem2)
    return false;
  if (theVictim.GetTail() != &theElem3)
    return false;

  theVictim.Remove(&theElem3);

  if (theVictim.GetHead() != &theElem2)
    return false;
  if (theVictim.GetTail() != &theElem2)
    return false;

  return true;
}
#endif

QueueIter::QueueIter(Queue *inQueue, QueueElem *startElemP)
    : fQueueP(inQueue) {
  if (startElemP) {
    Assert(startElemP->IsMember(*inQueue));
    fCurrentElemP = startElemP;
  } else
    fCurrentElemP = nullptr;
}

void QueueIter::Next() {
  if (fCurrentElemP == fQueueP->GetTail())
    fCurrentElemP = nullptr;
  else
    fCurrentElemP = fCurrentElemP->Prev();
}

QueueElem::~QueueElem() {
  Assert(fQueue == nullptr);
}

