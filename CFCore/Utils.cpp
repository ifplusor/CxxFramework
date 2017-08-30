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
    File:       OS.cpp

    Contains:   OS utility functions
*/

#include <sys/stat.h>
#include <errno.h>

#include <CF/Core/Utils.h>
#include <CF/Core/Thread.h>

#if (__solaris__ || __linux__ || __linuxppc__)
#include <CF/StringParser.h>
#endif

#ifdef __sgi__
#include <unistd.h>
#endif

#if __linux__

#include <unistd.h>
#include <grp.h>
#include <pwd.h>

#endif

#if (__FreeBSD__ || __MacOSX__)
#include <sys/sysctl.h>
#endif

#if __sgi__
#include <sys/systeminfo.h>
#endif



using namespace CF::Core;

double  Utils::sDivisor = 0;
double  Utils::sMicroDivisor = 0;

char Utils::sUser[128] = "";
char Utils::sGroup[128] = "";

SInt64 Utils::HostToNetworkSInt64(SInt64 hostOrdered) {
#if BIGENDIAN
  return hostOrdered;
#else
  return (SInt64) ((UInt64) (hostOrdered << 56)
      | (UInt64) (((UInt64) 0x00ff0000 << 32) & (hostOrdered << 40))
      | (UInt64) (((UInt64) 0x0000ff00 << 32) & (hostOrdered << 24))
      | (UInt64) (((UInt64) 0x000000ff << 32) & (hostOrdered << 8))
      | (UInt64) (((UInt64) 0x00ff0000 << 8) & (hostOrdered >> 8))
      | (UInt64) ((UInt64) 0x00ff0000 & (hostOrdered >> 24))
      | (UInt64) ((UInt64) 0x0000ff00 & (hostOrdered >> 40))
      | (UInt64) ((UInt64) 0x00ff & (hostOrdered >> 56)));
#endif
}

SInt64 Utils::NetworkToHostSInt64(SInt64 networkOrdered) {
#if BIGENDIAN
  return networkOrdered;
#else
  return (SInt64) ((UInt64) (networkOrdered << 56)
      | (UInt64) (((UInt64) 0x00ff0000 << 32) & (networkOrdered << 40))
      | (UInt64) (((UInt64) 0x0000ff00 << 32) & (networkOrdered << 24))
      | (UInt64) (((UInt64) 0x000000ff << 32) & (networkOrdered << 8))
      | (UInt64) (((UInt64) 0x00ff0000 << 8) & (networkOrdered >> 8))
      | (UInt64) ((UInt64) 0x00ff0000 & (networkOrdered >> 24))
      | (UInt64) ((UInt64) 0x0000ff00 & (networkOrdered >> 40))
      | (UInt64) ((UInt64) 0x00ff & (networkOrdered >> 56)));
#endif
}

OS_Error Utils::MakeDir(char *inPath) {
  struct stat theStatBuffer;
  if (::stat(inPath, &theStatBuffer) == -1) {
    //this directory doesn't exist, so let's try to create it
#if __Win32__ || __MinGW__
    if ((inPath[1] == ':') && (strlen(inPath) == 2))
        return OS_NoErr;

    if (::mkdir(inPath) == -1)
#else
    if (::mkdir(inPath, 0777) == -1)
#endif
      return (OS_Error) Thread::GetErrno();
  }
#if __Win32__ || __MinGW__
  else if (!(theStatBuffer.st_mode & _S_IFDIR)) // MSVC++ doesn't define the S_ISDIR macro
        return EEXIST; // there is a file at this point in the path!
#else
  else if (!S_ISDIR(theStatBuffer.st_mode))
    return (OS_Error) EEXIST;//there is a file at this point in the path!
#endif

  //directory exists
  return OS_NoErr;
}

OS_Error Utils::RecursiveMakeDir(char *inPath) {
  Assert(inPath != nullptr);

  //iterate through the path, replacing '/' with '\0' as we go
  char *thePathTraverser = inPath;

  //skip over the first / in the path.
  if (*thePathTraverser == kPathDelimiterChar)
    thePathTraverser++;

  while (*thePathTraverser != '\0') {
    if (*thePathTraverser == kPathDelimiterChar) {
      //we've found a filename divider. Now that we have a complete
      //filename, see if this partial path exists.

      //make the partial path into a C string
      *thePathTraverser = '\0';
      OS_Error theErr = MakeDir(inPath);
      //there is a directory here. Just continue in our traversal
      *thePathTraverser = kPathDelimiterChar;

      if (theErr != OS_NoErr)
        return theErr;
    }
    thePathTraverser++;
  }

  //need to create the last directory in the path
  return MakeDir(inPath);
}

UInt32 Utils::GetNumProcessors() {
#if (__Win32__)
  SYSTEM_INFO theSystemInfo;
  ::GetSystemInfo(&theSystemInfo);

  return (UInt32)theSystemInfo.dwNumberOfProcessors;
#endif

#if (__MacOSX__ || __FreeBSD__)
  int numCPUs = 1;
  size_t len = sizeof(numCPUs);
  int mib[2];
  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;
  (void) ::sysctl(mib, 2, &numCPUs, &len, nullptr, 0);
  if (numCPUs < 1)
      numCPUs = 1;
  return (UInt32)numCPUs;
#endif

#if(__linux__ || __linuxppc__)

  char cpuBuffer[8192] = "";
  StrPtrLen cpuInfoBuf(cpuBuffer, sizeof(cpuBuffer));
  FILE *cpuFile = ::fopen("/proc/cpuinfo", "r");
  if (cpuFile) {
    cpuInfoBuf.Len =
        ::fread(cpuInfoBuf.Ptr, sizeof(char), cpuInfoBuf.Len, cpuFile);
    ::fclose(cpuFile);
  }

  StringParser cpuInfoFileParser(&cpuInfoBuf);
  StrPtrLen line;
  StrPtrLen word;
  UInt32 numCPUs = 0;

  while (cpuInfoFileParser.GetDataRemaining() != 0) {
    cpuInfoFileParser.GetThruEOL(&line);    // Read each line
    StringParser lineParser(&line);
    lineParser.ConsumeWhitespace();         //skip over leading whitespace

    if (lineParser.GetDataRemaining() == 0) // must be an empty line
      continue;

    lineParser.ConsumeUntilWhitespace(&word);

    if (word.Equal("processor")) // found a processor as first word in line
    {
      numCPUs++;
    }
  }

  if (numCPUs == 0)
    numCPUs = 1;

  return numCPUs;
#endif

#if(__solaris__)
  {
      UInt32 numCPUs = 0;
      char linebuff[512] = "";
      StrPtrLen line(linebuff, sizeof(linebuff));
      StrPtrLen word;

      FILE *p = ::popen("uname -X", "r");
      while ((::fgets(linebuff, sizeof(linebuff - 1), p)) > 0)
      {
          StringParser lineParser(&line);
          lineParser.ConsumeWhitespace(); //skip over leading whitespace

          if (lineParser.GetDataRemaining() == 0) // must be an empty line
              continue;

          lineParser.ConsumeUntilWhitespace(&word);

          if (word.Equal("NumCPU")) // found a tag as first word in line
          {
              lineParser.GetThru(nullptr, '=');
              lineParser.ConsumeWhitespace();  //skip over leading whitespace
              lineParser.ConsumeUntilWhitespace(&word); //read the number of cpus
              if (word.Len > 0)
                  ::sscanf(word.Ptr, "%"   _U32BITARG_   "", &numCPUs);

              break;
          }
      }
      if (numCPUs == 0)
          numCPUs = 1;

      ::pclose(p);

      return numCPUs;
  }
#endif

#if(__sgi__)
  UInt32 numCPUs = 0;

  numCPUs = sysconf(_SC_NPROC_ONLN);

  return numCPUs;
#endif

  return 1;
}

bool Utils::ThreadSafe() {

#if (__MacOSX__) // check for version 7 or greater for Thread safe stdlib
  char releaseStr[32] = "";
  size_t strLen = sizeof(releaseStr);
  int mib[2];
  mib[0] = CTL_KERN;
  mib[1] = KERN_OSRELEASE;

  UInt32 majorVers = 0;
  int err = sysctl(mib, 2, releaseStr, &strLen, nullptr, 0);
  if (err == 0)
  {
      StrPtrLen rStr(releaseStr, strLen);
      char* endMajor = rStr.FindString(".");
      if (endMajor != nullptr) // truncate to an int value.
          *endMajor = 0;

      if (::strlen(releaseStr) > 0) //convert to an int
          ::sscanf(releaseStr, "%"   _U32BITARG_   "", &majorVers);
  }
  if (majorVers < 7) // less than OS X Panther 10.3
      return false; // force 1 worker Thread because < 10.3 means std c lib is not Thread safe.

#endif

  return true;
}

bool Utils::SwitchPersonality() {
#if __linux__
  if (::strlen(sGroup) > 0) {
    struct group *gr = ::getgrnam(sGroup);
    if (gr == nullptr || ::setgid(gr->gr_gid) == -1) {
      //s_printf("Thread %"   _U32BITARG_   " setgid  to group=%s FAILED \n", (UInt32) this, sGroup);
      return false;
    }

    //s_printf("Thread %"   _U32BITARG_   " setgid  to group=%s \n", (UInt32) this, sGroup);
  }

  if (::strlen(sUser) > 0) {
    struct passwd *pw = ::getpwnam(sUser);
    if (pw == nullptr || ::setuid(pw->pw_uid) == -1) {
      //s_printf("Thread %"   _U32BITARG_   " setuid  to user=%s FAILED \n", (UInt32) this, sUser);
      return false;
    }

    //s_printf("Thread %"   _U32BITARG_   " setuid  to user=%s \n", (UInt32) this, sUser);
  }
#endif

  return true;
}
