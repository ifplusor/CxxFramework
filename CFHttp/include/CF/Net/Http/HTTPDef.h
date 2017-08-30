//
// Created by james on 8/26/17.
//

#ifndef __CF_HTTP_DEF_H__
#define __CF_HTTP_DEF_H__

#include <CF/Net/Http/HTTPPacket.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef CF_Error (*CF_CGIFunction) (CF::Net::HTTPPacket &request,
                                    CF::Net::HTTPPacket &response);

struct HTTPMapping {
  char *path;
  CF_CGIFunction func;
};
typedef struct HTTPMapping HTTPMapping;

#ifdef __cplusplus
};
#endif

#endif //__CF_HTTP_DEF_H__
