//
// Created by james on 8/25/17.
//

#ifndef __CF_CONFIGURE_H__
#define __CF_CONFIGURE_H__

#include <CF/CFDef.h>
#include <CF/CFEnv.h>
#include <CF/Net/Http/HTTPDef.h>

namespace CF {

/**
 * @brief Interface for configure framework.
 *
 * CxxFramework only provide this declaration with default configure, but
 * developer should give custom implement by inheritance.
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

  static CF_Error DefaultExitCGI(Net::HTTPPacket &request,
                                 Net::HTTPPacket &response) {
    ResizeableStringFormatter formatter(nullptr, 0);
    formatter.Put("server will exit\n");
    StrPtrLen *content = new StrPtrLen(formatter.GetAsCString(),
                                       formatter.GetCurrentOffset());
    response.SetBody(content);
    CFEnv::Exit(1);
    return CF_NoErr;
  }

  virtual HTTPMapping *GetHttpMapping() {
    static HTTPMapping defaultHttpMapping[] = {
        {"/exit", (CF_CGIFunction) DefaultExitCGI},
        {NULL, NULL}
    };
    return defaultHttpMapping;
  }

  virtual CF_NetAddr *GetHttpListenAddr(UInt32 *outNum) {
    static CF_NetAddr defaultHttpAddrs[] = {
        {"0.0.0.0", 8080}
    };
    *outNum = 1;
    return defaultHttpAddrs;
  }
};

}

#endif //__CF_CONFIGURE_H__
