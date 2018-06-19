//
// Created by james on 8/28/17.
//

#ifndef __CF_STATE_H__
#define __CF_STATE_H__

#include <CF/Types.h>
#include <CF/Queue.h>
#include <CF/Net/Socket/EventContext.h>

namespace CF {

class CFState {
 public:

  enum {
    kKillListener = 0x01U << 0x00U, /* Kill TCPListenerSocket */
    kDisableEvent = 0x01U << 0x01U, /* Disable RequestEvent */
    kCleanEvent   = 0x01U << 0x02U, /* Cleanup all EventContext in select */
  };
  static std::atomic<UInt32> sState;

  static void WaitProcessState(UInt32 state) {
    sState |= state;
    while (sState & state) {
      Core::Thread::Sleep(1000);
    }
  }

  static Queue sListenerSocket;

 private:
  CFState() = default;
};

}

#endif //__CF_STATE_H__
