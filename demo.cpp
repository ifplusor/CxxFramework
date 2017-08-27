//
// Created by james on 8/27/17.
//

#include <CF.h>

class MyConfig : public CFConfigure {
 public:
  HTTPMapping *GetHttpMapping() override {
    static HTTPMapping defaultHttpMapping[] = {
        {"/", (CF_CGIFunction) defaultCGI},
        {"/exit", (CF_CGIFunction) exitCGI},
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

  static CF_Error exitCGI(HTTPPacket &request, HTTPPacket &response) {
    ResizeableStringFormatter formatter(nullptr, 0);
    formatter.Put("server will exit\n");
    StrPtrLen *content = new StrPtrLen(formatter.GetAsCString(),
                                       formatter.GetCurrentOffset());
    response.SetBody(content);
    CFEnv::Exit(1);
    return CF_NoErr;
  }
};

CF_Error CFInit(int argc, char **argv) {
  CFConfigure *config = new MyConfig();
  CFEnv::Register(config);
  return CF_NoErr;
}

CF_Error CFExit(CF_Error exitCode) {
  return CF_NoErr;
}
