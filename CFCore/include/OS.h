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
    File:       OS.h

    Contains:   OS utility functions. Memory allocation, etc.

*/

#ifndef __OS_H__
#define __OS_H__

#include <string.h>
#include <OSHeaders.h>
#include "OSTime.h"

class OS {
 public:

  /**
   * @brief set global variables about time.
   * @note call this before calling anything else
   */
  static void Initialize() {
    OSTime::Initialize();
  }

  static SInt32 Min(SInt32 a, SInt32 b) {
    return a < b ? a : b;
  }

  // Some processors (MIPS, Sparc) cannot handle non word aligned memory
  // accesses. So, we need to provide functions to safely get at non-word
  // aligned memory.
  static inline UInt32 GetUInt32FromMemory(UInt32 *inP);

  // because the OS doesn't seem to have these functions
  static SInt64 HostToNetworkSInt64(SInt64 hostOrdered);
  static SInt64 NetworkToHostSInt64(SInt64 networkOrdered);

  // Both these functions return QTSS_NoErr, QTSS_FileExists, or POSIX errorcode
  // Makes whatever directories in this path that don't exist yet
  static OS_Error RecursiveMakeDir(char *inPath);
  // Makes the directory at the end of this path
  static OS_Error MakeDir(char *inPath);

  // Discovery of how many processors are on this machine
  static UInt32 GetNumProcessors();

  // CPU Load
  static Float32 GetCurrentCPULoadPercent();

  static bool ThreadSafe();

  static void SetUser(char *user) {
    ::strncpy(sUser, user, sizeof(sUser) - 1);
    sUser[sizeof(sUser) - 1] = 0;
  }

  static void SetGroup(char *group) {
    ::strncpy(sGroup, group, sizeof(sGroup) - 1);
    sGroup[sizeof(sGroup) - 1] = 0;
  }

  static void SetPersonality(char *user, char *group) {
    SetUser(user);
    SetGroup(group);
  };

  /**
   * @brief switch user and group
   * @note only super-user can switch real user and group, others merely
   *       switch effective user and group between real user/group ID and
   *       saved set-user-ID/set-group-ID
   */
  static bool SwitchPersonality();

 private:
  static void setDivisor();

  static double sDivisor;
  static double sMicroDivisor;

  static SInt32 sMemoryErr;

  static char sUser[128];
  static char sGroup[128];
};

inline UInt32 OS::GetUInt32FromMemory(UInt32 *inP) {
#if ALLOW_NON_WORD_ALIGN_ACCESS
  return *inP;
#else
  char* tempPtr = (char*)inP;
  UInt32 temp = 0;
  ::memcpy(&temp, tempPtr, sizeof(UInt32));
  return temp;
#endif
}

#endif // __OS_H__
