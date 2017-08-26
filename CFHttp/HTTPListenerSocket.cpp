//
// Created by james on 8/22/17.
//

#include "HTTPListenerSocket.h"
#include "HTTPSession.h"

Task *HTTPListenerSocket::GetSessionTask(TCPSocket **outSocket) {
  Assert(outSocket != nullptr);

  HTTPSession *theTask = new HTTPSession();
  *outSocket = theTask->GetSocket();  // out socket is not attached to a unix socket yet.

  if (this->OverMaxConnections(0))
    this->SlowDown();
  else
    this->RunNormal();

  return theTask;
}

bool HTTPListenerSocket::OverMaxConnections(UInt32 buffer) {
  return false;
}