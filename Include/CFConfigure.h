//
// Created by james on 8/25/17.
//

#ifndef __CF_CONFIGURE_H__
#define __CF_CONFIGURE_H__

#include "CFDef.h"
#include <HTTPDef.h>

/**
 * @brief Interface for configure framework.
 *
 * CxxFramework only provide this declaration, and developer should give the
 * completed implement.
 */
class CFConfigure {
 public:
  CFConfigure() = default;
  virtual ~CFConfigure() = default;

  virtual char *GetPersonalityUser() {
    static char defaultUser[] = "root";
    return defaultUser;
  };

  virtual char *GetPersonalityGroup() {
    static char defaultGroup[] = "root";
    return defaultGroup;
  };

  //
  // TaskThreadPool Settings

  virtual UInt32 GetShortTaskThreads() { return 1; }
  virtual UInt32 GetBlockingThreads() { return 1; }

  //
  // Http Server Settings

  virtual HTTPMapping *GetHttpMapping() {
    static HTTPMapping defaultHttpMapping[] = {
        {"/", (CF_CGIFunction) defaultCGI},
        {NULL, NULL}
    };
    return defaultHttpMapping;
  }

 private:
  static CF_Error defaultCGI(HTTPPacket &request, HTTPPacket &response) {
    ResizeableStringFormatter formatter(nullptr, 0);
    formatter.Put("test content\n");
    StrPtrLen *content = new StrPtrLen(formatter.GetAsCString(),
                                       formatter.GetCurrentOffset());
    response.SetBody(content);
    return CF_NoErr;
  }
};

#endif //__CF_CONFIGURE_H__
