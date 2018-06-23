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
    File:       OSRef.h

    Contains:   Class supports creating unique string IDs to object pointers.
                A grouping of an object and its string ID may be stored in an
                OSRefTable, and the associated object pointer can be looked up
                by string ID.

                Refs can only be removed from the table when no one is using
                the Ref, therefore allowing clients to arbitrate access to
                objects in a preemptive, multithreaded environment.

*/

#ifndef __CF_REF_H__
#define __CF_REF_H__

#include <CF/Types.h>
#include <CF/StrPtrLen.h>
#include <CF/HashTable.h>
#include <CF/Core/Cond.h>

#ifndef DEBUG_REF
#define DEBUG_REF 0
#else
#undef  DEBUG_REF
#define DEBUG_REF 1
#endif

namespace CF {

class RefKey;

class RefTableUtils {
 private:

  static UInt32 HashString(StrPtrLen *inString);

  friend class Ref;
  friend class RefKey;
};

/**
 * @brief 引用记录，K-V 项，引用计数器
 */
class Ref {
 public:

  Ref()
      : fObjectP(nullptr),
        fRefCount(0),
        fHashValue(0),
        fNextHashEntry(nullptr) {
#if DEBUG_REF
    fInATable = false;
    fSwapCalled = false;
#endif
  }
  Ref(const StrPtrLen &inString, void *inObjectP)
      : fRefCount(0), fNextHashEntry(nullptr) {
    Set(inString, inObjectP);
  }
  ~Ref() {}

  void Set(const StrPtrLen &inString, void *inObjectP) {
#if DEBUG_REF
    fInATable = false;
    fSwapCalled = false;
#endif
    fString = inString;
    fObjectP = inObjectP;
    fHashValue = RefTableUtils::HashString(&fString);
  }

#if DEBUG_REF
  bool IsInTable() { return fInATable; }
#endif
  void **GetObjectPtr() { return &fObjectP; }
  void *GetObject() { return fObjectP; }
  UInt32 GetRefCount() { return fRefCount; }
  StrPtrLen *GetString() { return &fString; }
 private:

  //value
  void *fObjectP;
  //key
  StrPtrLen fString;

  //refcounting
  UInt32 fRefCount;
#if DEBUG_REF
  bool fInATable;
  bool fSwapCalled;
#endif
  Core::Cond fCond;//to block threads waiting for this Ref.

  UInt32 fHashValue;
  Ref *fNextHashEntry;

  friend class RefKey;
  friend class HashTable<Ref, RefKey>;
  friend class HashTableIter<Ref, RefKey>;
  friend class RefTable;

};

class RefKey {
 public:

  //CONSTRUCTOR / DESTRUCTOR:
  RefKey(StrPtrLen *inStringP)
      : fStringP(inStringP) {
    fHashValue = RefTableUtils::HashString(inStringP);
  }

  ~RefKey() {}

  //ACCESSORS:
  StrPtrLen *GetString() { return fStringP; }

 private:

  //PRIVATE ACCESSORS:
  SInt32 GetHashKey() { return fHashValue; }

  //these functions are only used by the hash table itself. This constructor
  //will break the "Set" functions.
  RefKey(Ref *elem) : fStringP(&elem->fString),
                      fHashValue(elem->fHashValue) {}

  friend int operator==(const RefKey &key1, const RefKey &key2) {
    if (key1.fStringP->Equal(*key2.fStringP))
      return true;
    return false;
  }

  //data:
  StrPtrLen *fStringP;
  UInt32 fHashValue;

  friend class HashTable<Ref, RefKey>;
};

typedef HashTable<Ref, RefKey> RefHashTable;
typedef HashTableIter<Ref, RefKey> RefHashTableIter;

/**
 * @brief 引用字典，使用哈希表存储
 *
 * @note 维护引用计数，但不管理对象内存
 */
class RefTable {
 public:

  enum {
    kDefaultTableSize = 1193 //UInt32
  };

  //tableSize doesn't indicate the max number of Refs that can be added
  //(it's unlimited), but is rather just how big to make the hash table
  explicit RefTable(UInt32 tableSize = kDefaultTableSize) : fTable(tableSize), fMutex() {}
  ~RefTable() = default;

  //Allows access to the Mutex in case you need to lock the table down
  //between operations
  Core::Mutex *GetMutex() { return &fMutex; }
  RefHashTable *GetHashTable() { return &fTable; }

  //Registers a Ref in the table. Once the Ref is in, clients may resolve
  //the Ref by using its string ID. You must setup the Ref before passing it
  //in here, ie., setup the string and object pointers
  //This function will succeed unless the string identifier is not unique,
  //in which case it will return QTSS_DupName
  //This function is atomic write this Ref table.
  OS_Error Register(Ref *ref);

  // RegisterOrResolve
  // If the ID of the input Ref is unique, this function is equivalent to
  // Register, and returns nullptr.
  // If there is a duplicate ID already in the map, this funcion
  // leave it, resolves it, and returns it.
  Ref *RegisterOrResolve(Ref *inRef);

  //This function may block. You can only remove a Ref from the table
  //when the refCount drops to the level specified. If several threads have
  //the Ref currently, the calling Thread will wait until the other threads
  //stop using the Ref (by calling Release, below)
  //This function is atomic write this Ref table.
  void UnRegister(Ref *ref, UInt32 refCount = 0);

  // Same as UnRegister, but guarenteed not to block. Will return
  // true if Ref was sucessfully unregistered, false otherwise
  bool TryUnRegister(Ref *ref, UInt32 refCount = 0);

  //Resolve. This function uses the provided key string to identify and grab
  //the Ref keyed by that string. Once the Ref is resolved, it is safe to use
  //(it cannot be removed from the Ref table) until you call Release. Because
  //of that, you MUST call release in a timely manner, and be aware of potential
  //deadlocks because you now own a resource being contended over.
  //This function is atomic write this Ref table.
  Ref *Resolve(StrPtrLen *inString);

  //Release. Release a Ref, and drops its refCount. After calling this, the
  //Ref is no longer safe to use, as it may be removed from the Ref table.
  void Release(Ref *inRef);

  // Swap. This atomically removes any existing Ref in the table with the new
  // Ref's ID, and replaces it with this new Ref. If there is no matching Ref
  // already in the table, this function does nothing.
  //
  // Be aware that this creates a situation where clients may have a Ref
  // resolved that is no longer in the table. The old Ref must STILL be
  // UnRegistered normally.
  // Once Swap completes successfully, clients that call resolve on the ID
  // will get the new OSRef object.
  void Swap(Ref *newRef);

  UInt32 GetNumRefsInTable();

 private:

  //all this object needs to do its job is an atomic HashTable
  RefHashTable fTable;
  Core::Mutex fMutex;
};

class RefReleaser {
 public:

  RefReleaser(RefTable *inTable, Ref *inRef)
      : fOSRefTable(inTable), fOSRef(inRef) {}
  ~RefReleaser() { fOSRefTable->Release(fOSRef); }

  Ref *GetRef() { return fOSRef; }

 private:

  RefTable *fOSRefTable;
  Ref *fOSRef;
};

}

#endif //__CF_REF_H__
