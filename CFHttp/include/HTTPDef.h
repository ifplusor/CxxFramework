//
// Created by james on 8/26/17.
//

#ifndef __CF_HTTP_DEF_H__
#define __CF_HTTP_DEF_H__

#include "HTTPPacket.h"

typedef CF_Error (*CF_CGIFunction) (HTTPPacket &request, HTTPPacket &response);

struct HTTPMapping {
  const char *path;
  CF_CGIFunction func;
};
typedef struct HTTPMapping HTTPMapping;

#endif //__CF_HTTP_DEF_H__
