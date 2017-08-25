//
// Created by james on 8/25/17.
//

#include "CFConfigure.h"


char CFConfigure::defaultUser[] = "root";
char CFConfigure::defaultGroup[] = "root";

HTTPMapping CFConfigure::defaultHttpMapping[] = {
    {"/*", (CF_CGIFunction) defaultCGI},
    {NULL, NULL}
};

void CFConfigure::Initialize() {

}

CF_Error CFConfigure::defaultCGI(HTTPPacket &request, HTTPPacket &response) {
  ResizeableStringFormatter formatter(nullptr, 0);
  formatter.Put("test content\n");
  StrPtrLen *content = new StrPtrLen(formatter.GetAsCString(),
                                     formatter.GetCurrentOffset());
  response.SetBody(content);
  return CF_NoErr;
}
