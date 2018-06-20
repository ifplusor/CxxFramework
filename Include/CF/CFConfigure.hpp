//
// Created by james on 8/25/17.
//

#ifndef __CF_CONFIGURE_H__
#define __CF_CONFIGURE_H__

#include <CF/CFDef.h>

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

  //
  // Life cycle check points

  virtual CF_Error AfterInitBase() { return CF_NoErr; }
  virtual CF_Error AfterConfigThreads(UInt32 numThreads) { return CF_NoErr; }
  virtual CF_Error AfterConfigFramework() { return CF_NoErr; }
  virtual CF_Error StartupCustomServices() { return CF_NoErr; }
  virtual void DoIdle() { }

  //
  // User Settings

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
};

}

#endif //__CF_CONFIGURE_H__
