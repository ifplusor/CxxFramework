//
// Created by james on 8/22/17.
//

#include <CF/Net/Http/HTTPListenerSocket.h>
#include <CF/Net/Http/HTTPSession.h>

namespace CF {
namespace Net {

Thread::Task *HTTPListenerSocket::GetSessionTask(TCPSocket **outSocket) {
  Assert(outSocket != nullptr);

  auto *theTask = new HTTPSession();
  *outSocket = theTask->GetSocket(); // out Socket is not attached to a unix Socket yet.

  if (this->OverMaxConnections(0))
    this->SlowDown();
  else
    this->RunNormal();

  return theTask;
}

bool HTTPListenerSocket::OverMaxConnections(UInt32 buffer) {
  return false;
}

} // namespace Net
} // namespace CF
