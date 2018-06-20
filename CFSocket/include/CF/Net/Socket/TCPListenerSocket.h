/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2008 Apple Inc.  All Rights Reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */
/*
    File:       TCPListenerSocket.h

    Contains:   A TCP listener Socket. When a new connection comes in, the listener
                attempts to assign the new connection to a Socket object and a Task
                object. Derived classes must implement a method of getting new
                Task & Socket objects

*/


#ifndef __TCP_LISTENER_SOCKET_H__
#define __TCP_LISTENER_SOCKET_H__

#include <CF/Net/Socket/TCPSocket.h>
#include <CF/Thread/IdleTask.h>

namespace CF {
namespace Net {

class TCPListenerSocket : public TCPSocket, public Thread::IdleTask {
 public:

  TCPListenerSocket()
      : TCPSocket(nullptr, Socket::kNonBlockingSocketType),
        IdleTask(),
        fAddr(0),
        fPort(0),
        fOutOfDescriptors(false),
        fSleepBetweenAccepts(false) {
    this->SetTaskName("TCPListenerSocket");
  }
  ~TCPListenerSocket() override = default;

  //
  // Send a TCPListenerObject a Kill event to delete it.

  /**
   * starts listening
   * @param addr - listening address.
   * @param port - listening port. Automatically
   * @return
   */
  OS_Error Initialize(UInt32 addr, UInt16 port);

  //You can query the listener to see if it is failing to accept
  //connections because the OS is out of descriptors.
  bool IsOutOfDescriptors() { return fOutOfDescriptors; }

  void SlowDown() { fSleepBetweenAccepts = true; }
  void RunNormal() { fSleepBetweenAccepts = false; }

  //derived object must implement a way of getting tasks & sockets to this object
  virtual Thread::Task *GetSessionTask(TCPSocket **outSocket) = 0;

  SInt64 Run() override;

 private:

  enum {
    kTimeBetweenAcceptsInMsec = 1000,   //UInt32
    kListenQueueLength = 128            //UInt32
  };

  void ProcessEvent(int eventBits) override;
  OS_Error listen(UInt32 queueLength);

  UInt32 fAddr;
  UInt16 fPort;

  bool fOutOfDescriptors;
  bool fSleepBetweenAccepts;
};

} // namespace Net
} // namespace CF

#endif // __TCP_LISTENER_SOCKET_H__

