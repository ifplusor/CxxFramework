//
// Created by james on 8/25/17.
//

#ifndef __CF_ENV_H__
#define __CF_ENV_H__

#include <StrPtrLen.h>
#include <OSQueue.h>
#include <EventContext.h>
#include "CFDef.h"
#include "CFConfigure.h"

class CFEnv {
 public:

  /**
   * @brief initialize environment information
   * @note this function is the first executor when program start
   */
  static void Initialize();

  /**
   * configure 的内存将交由 CFEnv 管理
   */
  static void Register(CFConfigure *configure) { sConfigure = configure; }

  static CFConfigure *GetConfigure() { return sConfigure; }

  static bool WillExit() { return sExitCode != CF_NoErr; };
  static void Exit(UInt32 exitCode) { sExitCode = exitCode; }

  // SERVER NAME & VERSION
  static StrPtrLen &GetServerName() { return sServerNameStr; }
  static StrPtrLen &GetServerVersion() { return sServerVersionStr; }
  static StrPtrLen &GetServerPlatform() { return sServerPlatformStr; }
  static StrPtrLen &GetServerBuildDate() { return sServerBuildDateStr; }
  static StrPtrLen &GetServerHeader() { return sServerHeaderStr; }
  static StrPtrLen &GetServerBuild() { return sServerBuildStr; }
  static StrPtrLen &GetServerComment() { return sServerCommentStr; }

 private:
  CFEnv() = default;

  static void makeServerHeader();

  static SInt32 sExitCode;

  static CFConfigure *sConfigure;

  enum {
    kMaxServerHeaderLen = 1000
  };

  static UInt32 sServerAPIVersion;
  static StrPtrLen sServerNameStr;
  static StrPtrLen sServerVersionStr;
  static StrPtrLen sServerBuildStr;
  static StrPtrLen sServerCommentStr;
  static StrPtrLen sServerPlatformStr;
  static StrPtrLen sServerBuildDateStr;
  static char sServerHeader[kMaxServerHeaderLen];
  static StrPtrLen sServerHeaderStr;

  friend EventThread;
};

#endif //__CF_ENV_H__
