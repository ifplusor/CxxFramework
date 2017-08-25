//
// Created by james on 8/25/17.
//

#ifndef __CF_ENV_H__
#define __CF_ENV_H__

#include "CFDef.h"
#include <StrPtrLen.h>


class CFEnv {
 public:

  static void Initialize();


  // SERVER NAME & VERSION

  static StrPtrLen &GetServerName() { return sServerNameStr; }
  static StrPtrLen &GetServerVersion() { return sServerVersionStr; }
  static StrPtrLen &GetServerPlatform() { return sServerPlatformStr; }
  static StrPtrLen &GetServerBuildDate() { return sServerBuildDateStr; }
  static StrPtrLen &GetServerHeader() { return sServerHeaderStr; }
  static StrPtrLen &GetServerBuild() { return sServerBuildStr; }
  static StrPtrLen &GetServerComment() { return sServerCommentStr; }

 private:
  CFEnv() {};

  static void makeServerHeader();

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
};

#endif //__CF_ENV_H__
