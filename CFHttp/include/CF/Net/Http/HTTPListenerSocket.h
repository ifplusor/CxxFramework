//
// Created by james on 8/22/17.
//

#ifndef __HTTP_LISTENER_SOCKET_H__
#define __HTTP_LISTENER_SOCKET_H__

#include <CF/Net/Socket/TCPListenerSocket.h>

namespace CF::Net {

class HTTPListenerSocket : public TCPListenerSocket {
 public:

  HTTPListenerSocket() {}

  virtual ~HTTPListenerSocket() {}

  //sole job of this object is to implement this function
  virtual Thread::Task *GetSessionTask(TCPSocket **outSocket) override;

  //check whether the Listener should be idling
  bool OverMaxConnections(UInt32 buffer);

};

}

#endif // __HTTP_LISTENER_SOCKET_H__
