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
    File:       OSRef.cpp

    Contains:   Implementation of OSRef class.

*/

#include <CF/Ref.h>

using namespace CF;

UInt32 RefTableUtils::HashString(StrPtrLen *inString) {
  Assert(inString != nullptr);
  Assert(inString->Ptr != nullptr);
  Assert(inString->Len > 0);
  if (inString == nullptr || inString->Len == 0 || inString->Ptr == nullptr)
    return 0;

  //make sure to convert to unsigned here, as there may be binary
  //data in this string
  UInt8 *theData = (UInt8 *) inString->Ptr;

  //divide by 4 and take the characters at quarter points in the string,
  //use those as the basis for the hash value
  UInt32 quarterLen = inString->Len >> 2;
  return (inString->Len * (theData[0] + theData[quarterLen] +
      theData[quarterLen * 2] + theData[quarterLen * 3] +
      theData[inString->Len - 1]));
}

OS_Error RefTable::Register(Ref *inRef) {
  Assert(inRef != nullptr);
  if (inRef == nullptr)
    return (OS_Error) EPERM;
#if DEBUG
    Assert(!inRef->fInATable);
#endif
  Assert(inRef->fRefCount == 0);
  Assert(inRef->fString.Ptr != nullptr);
  Assert(inRef->fString.Len != 0);

  Core::MutexLocker locker(&fMutex);
  if (inRef->fString.Ptr == nullptr || inRef->fString.Len == 0) {
    //printf("OSRefTable::Register inRef is invalid \n");
    return (OS_Error) EPERM;
  }

  // Check for a duplicate. In this function, if there is a duplicate,
  // return an error, don't resolve the duplicate
  RefKey key(&inRef->fString);
  Ref *duplicateRef = fTable.Map(&key);
  if (duplicateRef != nullptr)
    return (OS_Error) EPERM;

  // There is no duplicate, so add this Ref into the table
#if DEBUG_REF
  inRef->fInATable = true;
#endif
  fTable.Add(inRef);
  return OS_NoErr;
}

Ref *RefTable::RegisterOrResolve(Ref *inRef) {
  Assert(inRef != nullptr);
#if DEBUG_REF
  Assert(!inRef->fInATable);
#endif
  Assert(inRef->fRefCount == 0);

  Core::MutexLocker locker(&fMutex);

  // Check for a duplicate. If there is one, resolve it and return it to the caller
  Ref *duplicateRef = this->Resolve(&inRef->fString);
  if (duplicateRef != nullptr)
    return duplicateRef;

  // There is no duplicate, so add this Ref into the table
#if DEBUG_REF
  inRef->fInATable = true;
#endif
  fTable.Add(inRef);
  return nullptr;
}

void RefTable::UnRegister(Ref *ref, UInt32 refCount) {
  Assert(ref != nullptr);
  Core::MutexLocker locker(&fMutex);

  //make sure that no one else is using the object
  while (ref->fRefCount > refCount)
    ref->fCond.Wait(&fMutex);

#if DEBUG_REF
  RefKey key(&ref->fString);
  if (ref->fInATable)
      Assert(fTable.Map(&key) != nullptr);
  ref->fInATable = false;
#endif

  //ok, we now definitely have no one else using this object, so
  //remove it from the table
  fTable.Remove(ref);
}

bool RefTable::TryUnRegister(Ref *ref, UInt32 refCount) {
  Core::MutexLocker locker(&fMutex);
  if (ref->fRefCount > refCount)
    return false;

  // At this point, this is guarenteed not to block, because
  // we've already checked that the refCount is low.
  this->UnRegister(ref, refCount);
  return true;
}

Ref *RefTable::Resolve(StrPtrLen *inUniqueID) {
  Assert(inUniqueID != nullptr);
  RefKey key(inUniqueID);

  //this must be done atomically wrt the table
  Core::MutexLocker locker(&fMutex);
  Ref *ref = fTable.Map(&key);
  if (ref != nullptr) {
    ref->fRefCount++;
    Assert(ref->fRefCount > 0);
  }
  return ref;
}

void RefTable::Release(Ref *ref) {
  Assert(ref != nullptr);
  Core::MutexLocker locker(&fMutex);
  ref->fRefCount--;
  // fRefCount is a UInt32  and QTSS should never run into
  // a Ref greater than 16 * 64K, so this assert just checks to
  // be sure that we have not decremented the Ref less than zero.
  Assert(ref->fRefCount < 1048576L);
  //make sure to wakeup anyone who may be waiting for this resource to be released
  ref->fCond.Signal();
}

void RefTable::Swap(Ref *newRef) {
  Assert(newRef != nullptr);
  Core::MutexLocker locker(&fMutex);

  RefKey key(&newRef->fString);
  Ref *oldRef = fTable.Map(&key);
  if (oldRef != nullptr) {
    fTable.Remove(oldRef);
    fTable.Add(newRef);
#if DEBUG_REF
    newRef->fInATable = true;
    oldRef->fInATable = false;
    oldRef->fSwapCalled = true;
#endif
  } else Assert(0);
}

UInt32 RefTable::GetNumRefsInTable() {
  UInt64 result = fTable.GetNumEntries();
  Assert(result < kUInt32_Max);
  return (UInt32) result;
}
