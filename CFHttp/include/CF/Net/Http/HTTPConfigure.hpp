//
// Created by james on 10/6/17.
//

#ifndef __CF_HTTPCONFIGURE_H__
#define __CF_HTTPCONFIGURE_H__

#include <CF/CFEnv.h>
#include <CF/Net/Http/HTTPDef.h>
#include <CF/Net/Http/HTTPListenerSocket.h>
#include <CF/Net/Http/HTTPSessionInterface.h>
#include <CF/Net/Socket/SocketUtils.h>

namespace CF {
namespace Net {

class HTTPConfigure {
 public:
  HTTPConfigure() = default;
  virtual ~HTTPConfigure() = default;

  static CF_Error StartupHTTPService(HTTPConfigure *config) {
    CF_Error theErr;

    // Http server configure;
    UInt32 numHttpListens;
    CF_NetAddr *httpListenAddrs = config->GetHttpListenAddr(&numHttpListens);
    if (numHttpListens > 0) {
      HTTPSessionInterface::Initialize(config->GetHttpMapping());
      for (UInt32 i = 0; i < numHttpListens; i++) {
        auto *httpSocket = new HTTPListenerSocket();
        theErr = httpSocket->Initialize(SocketUtils::ConvertStringToAddr(httpListenAddrs[i].ip), httpListenAddrs[i].port);
        if (theErr == CF_NoErr) {
          CFEnv::AddListenerSocket(httpSocket);
          httpSocket->RequestEvent(EV_RE);
        } else {
          delete httpSocket;
        }
      }
    }

    return CF_NoErr;
  }

  //
  // Http Server Settings

  static CF_Error DefaultExitCGI(HTTPPacket &request, HTTPPacket &response) {
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
        {"127.0.0.1", 8080}
    };
    *outNum = 1;
    return defaultHttpAddrs;
  }

};

}
}

#endif //__CF_HTTPCONFIGURE_H__
