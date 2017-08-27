//
// Created by james on 8/23/17.
//

#ifndef __CXX_FRAMEWORK_H__
#define __CXX_FRAMEWORK_H__

#include "CFDef.h"
#include "CFEnv.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief give user a chance to initialize
 * @param argc - arguments count
 * @param argv - arguments vector
 */
extern CF_Error CFInit(int argc, char **argv);

extern CF_Error CFExit(CF_Error exitCode);

#ifdef __cplusplus
};
#endif

#endif // __CXX_FRAMEWORK_H__
