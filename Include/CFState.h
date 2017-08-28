//
// Created by james on 8/28/17.
//

#ifndef __CF_STATE_H__
#define __CF_STATE_H__

#include "OSHeaders.h"
#include <OSQueue.h>
#include <EventContext.h>

class CFState {
 public:

  static OSQueue sListenerSocket;

  enum {
    kKillListener = 0x0001,
  };
  static atomic<UInt32> sState = 0;

  static void WaitProcessState(UInt32 state) {
    sState |= state;
    while (sState & state) {
      OSThread::Sleep(1000);
    }
  }

 private:
  CFState() = default;
};

#endif //__CF_STATE_H__
