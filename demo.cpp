//
// Created by james on 8/27/17.
//

#include <CF/CF.h>

using namespace CF;

class MyConfig : public CFConfigure {
 public:
  HTTPMapping *GetHttpMapping() override {
    static HTTPMapping defaultHttpMapping[] = {
        {"/exit", (CF_CGIFunction) DefaultExitCGI},
        {"/", (CF_CGIFunction) DefaultCGI},
        {NULL, NULL}
    };
    return defaultHttpMapping;
  }

 private:
  static CF_Error DefaultCGI(CF::Net::HTTPPacket &request,
                             CF::Net::HTTPPacket &response) {
    ResizeableStringFormatter formatter(nullptr, 0);
    formatter.Put("test content\n");
    StrPtrLen *content = new StrPtrLen(formatter.GetAsCString(),
                                       formatter.GetCurrentOffset());
    response.SetBody(content);
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
