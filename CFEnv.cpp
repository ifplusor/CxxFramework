//
// Created by james on 8/25/17.
//

#include "CFEnv.h"
#include <StringFormatter.h>
#include <MyAssert.h>


// STATIC DATA

SInt32 CFEnv::sExitCode = 0;

CFConfigure *CFEnv::sConfigure = nullptr;

UInt32 CFEnv::sServerAPIVersion = CF_API_VERSION;

StrPtrLen CFEnv::sServerNameStr(PLATFORM_SERVER_TEXT_NAME);

// kVersionString from revision.h, include with -i at project level
StrPtrLen CFEnv::sServerVersionStr(kVersionString);
StrPtrLen CFEnv::sServerBuildStr(kBuildString);
StrPtrLen CFEnv::sServerCommentStr(kCommentString);

StrPtrLen CFEnv::sServerPlatformStr(kPlatformNameString);
StrPtrLen CFEnv::sServerBuildDateStr(__DATE__ ", " __TIME__);
char      CFEnv::sServerHeader[kMaxServerHeaderLen];
StrPtrLen CFEnv::sServerHeaderStr(sServerHeader, kMaxServerHeaderLen);


void CFEnv::makeServerHeader() {

  //Write out a premade server header
  StringFormatter serverFormatter(sServerHeaderStr.Ptr, kMaxServerHeaderLen);
  serverFormatter.Put(sServerNameStr);
  serverFormatter.PutChar('/');
  serverFormatter.Put(sServerVersionStr);
  serverFormatter.PutChar(' ');

  serverFormatter.PutChar('(');
  serverFormatter.Put("Build/");
  serverFormatter.Put(sServerBuildStr);
  serverFormatter.Put("; ");
  serverFormatter.Put("Platform/");
  serverFormatter.Put(sServerPlatformStr);
  serverFormatter.PutChar(';');

  if (sServerCommentStr.Len > 0) {
    serverFormatter.PutChar(' ');
    serverFormatter.Put(sServerCommentStr);
  }

  serverFormatter.PutChar(')');

  sServerHeaderStr.Len = serverFormatter.GetCurrentOffset();
  Assert(sServerHeaderStr.Len < kMaxServerHeaderLen);
}

void CFEnv::Initialize() {
  makeServerHeader();
}