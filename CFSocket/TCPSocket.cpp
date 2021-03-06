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
    File:       TCPSocket.cpp

    Contains:   implements TCPSocket class

*/

#include <CF/Net/Socket/TCPSocket.h>
#include <CF/Net/Socket/SocketUtils.h>

#if !__WinSock__
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#ifdef USE_NETLOG
#include <netlog.h>
#endif

using namespace CF::Net;

void TCPSocket::SnarfSocket(TCPSocket &fromSocket) {
  // take the connection away from the other Socket
  // and use it as our own.
  Assert(fFileDesc == EventContext::kInvalidFileDesc);
  this->Set(fromSocket.fFileDesc, &fromSocket.fRemoteAddr);

  // clear the old Socket so he doesn't close and the like
  struct sockaddr_in remoteaddr;

  ::memset(&remoteaddr, 0, sizeof(remoteaddr));

  fromSocket.Set(EventContext::kInvalidFileDesc, &remoteaddr);

  // get the event context too
  this->SnarfEventContext(fromSocket);

}

/*
 * 通过 getsockname 将 osSocket 描述字对应的 sockaddr 保存到
 * TCPSocket::fLocalAddr，并设置 TCPSocket::fState |= kBound | kConnected。
 * 同时将 osSocket 保存为 TCPSocket::fFileDesc
 */
void TCPSocket::Set(int inSocket, struct sockaddr_in *remoteaddr) {
  fRemoteAddr = *remoteaddr;
  fFileDesc = inSocket;

  if (inSocket != EventContext::kInvalidFileDesc) {
    // make sure to find out what IP address this connection is actually
    // occurring on. That way, we can report correct information to clients
    // asking what the connection's IP is
#if __Win32__ || __osf__ || __sgi__ || __hpux__
    int len = sizeof(fLocalAddr);
#else
    socklen_t len = sizeof(fLocalAddr);
#endif
    int err = ::getsockname(fFileDesc, (struct sockaddr *) &fLocalAddr, &len);
    AssertV(err == 0, Core::Thread::GetErrno());
    fState |= kBound;
    fState |= kConnected;
  } else {
    fState = 0;
  }
}

CF::StrPtrLen *TCPSocket::GetRemoteAddrStr() {
  if (fRemoteStr.Len == kIPAddrBufSize)
    SocketUtils::ConvertAddrToString(fRemoteAddr.sin_addr, &fRemoteStr);
  return &fRemoteStr;
}

OS_Error TCPSocket::Connect(UInt32 inRemoteAddr, UInt16 inRemotePort) {
  ::memset(&fRemoteAddr, 0, sizeof(fRemoteAddr));
  fRemoteAddr.sin_family = AF_INET;        /* host byte order */
  fRemoteAddr.sin_port = htons(inRemotePort); /* short, network byte order */
  fRemoteAddr.sin_addr.s_addr = htonl(inRemoteAddr);

  /* don't forget to error check the connect()! */
  int err =
      ::connect(fFileDesc, (sockaddr *) &fRemoteAddr, sizeof(fRemoteAddr));
  fState |= kConnected;

  if (err == -1) {
    fRemoteAddr.sin_port = 0;
    fRemoteAddr.sin_addr.s_addr = 0;
    return (OS_Error) Core::Thread::GetErrno();
  }

  return OS_NoErr;

}

