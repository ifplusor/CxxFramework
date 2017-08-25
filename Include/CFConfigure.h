//
// Created by james on 8/25/17.
//

#ifndef __CF_CONFIGURE_H__
#define __CF_CONFIGURE_H__

#include "CFDef.h"
#include "HTTPSessionInterface.h"

class CFConfigure {
 public:

  static void Initialize();

  static char *GetPersonalityUser() { return defaultUser; };
  static char *GetPersonalityGroup() { return defaultGroup; };

  static UInt32 GetShortTaskThreads() { return 0; }
  static UInt32 GetBlockingThreads() { return 0; }

  static HTTPMapping *GetHttpMapping() { return defaultHttpMapping; }

 protected:

  static CF_Error defaultCGI(HTTPPacket &request, HTTPPacket &response);

  static char defaultUser[];
  static char defaultGroup[];

  static HTTPMapping defaultHttpMapping[];
};

#endif //__CF_CONFIGURE_H__
