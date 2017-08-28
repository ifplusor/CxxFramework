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

  enum {
    kKillListener = 0x01 << 0x00, /* Kill TCPListenerSocket */
    kDisableEvent = 0x01 << 0x01, /* Disable RequestEvent */
    kCleanEvent = 0x01 << 0x02,   /* Cleanup all EventContext in select */
  };
  static atomic<UInt32> sState;

  static void WaitProcessState(UInt32 state) {
    sState |= state;
    while (sState & state) {
      OSThread::Sleep(1000);
    }
  }

  static OSQueue sListenerSocket;

 private:
  CFState() = default;
};

#endif //__CF_STATE_H__
