//
// Created by james on 8/25/17.
//

#include "CFConfigure.h"

static char defaultUser[] = "root";
static char defaultGroup[] = "root";

CF_Error defaultCGI(HTTPPacket &request, HTTPPacket &response) {
  ResizeableStringFormatter formatter(nullptr, 0);
  formatter.Put("test content\n");
  StrPtrLen *content = new StrPtrLen(formatter.GetAsCString(),
                                     formatter.GetCurrentOffset());
  response.SetBody(content);
  return CF_NoErr;
}

static HTTPMapping defaultHttpMapping[] = {
    {"/*", (CF_CGIFunction) defaultCGI},
    {NULL, NULL}
};

void CFConfigure::Initialize(int argc, void **argv) {

}

char *CFConfigure::GetPersonalityUser() { return defaultUser; };
char *CFConfigure::GetPersonalityGroup() { return defaultGroup; };

UInt32 CFConfigure::GetShortTaskThreads() { return 0; }
UInt32 CFConfigure::GetBlockingThreads() { return 0; }

HTTPMapping *CFConfigure::GetHttpMapping() { return defaultHttpMapping; }
