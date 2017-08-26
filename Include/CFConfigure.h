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

  /**
   * @brief give user a chance to initialize
   * @param argc - arguments count
   * @param argv - arguments vector
   */
  static void Initialize(int argc, void **argv);

  static char *GetPersonalityUser();
  static char *GetPersonalityGroup();

  //
  // TaskThreadPool Settings

  static UInt32 GetShortTaskThreads();
  static UInt32 GetBlockingThreads();

  //
  // Http Server Settings

  static HTTPMapping *GetHttpMapping();

};

#endif //__CF_CONFIGURE_H__
