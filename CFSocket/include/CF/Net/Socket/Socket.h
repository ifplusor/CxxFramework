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
    File:       Socket.h

    Contains:   Provides a simple, object oriented Socket abstraction, also
                hides the details of Socket event handling. Sockets can post
                events (such as S_DATA, S_CONNECTIONCLOSED) to Tasks.

*/

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <CF/Net/Socket/EventContext.h>

#if !__WinSock__

#include <netinet/in.h>

#endif

#ifndef DEBUG_SOCKET
#define DEBUG_SOCKET 0
#else
#undef  DEBUG_SOCKET
#define DEBUG_SOCKET 1
#endif

namespace CF {
namespace Net {

class Socket : public EventContext {
 public:

  /**
   * This class provides a global event thread, construct it.
   */
  static void Initialize() {
#if __WinSock__
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    int err = ::WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
      // Tell the user that we could not find a usable Winsock DLL.
      s_printf("WSAStartup failed with error: %d\n", err);
      exit(EXIT_FAILURE);
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
      // Tell the user that we could not find a usable WinSock DLL.
      s_printf("Could not find a usable version of Winsock.dll\n");
      WSACleanup();
      exit(EXIT_FAILURE);
    } else
      s_printf("The Winsock 2.2 dll was found okay\n");
#endif

    sEventThread = new EventThread();
  }

  static void StartThread() { sEventThread->Start(); }

  static void Release() {
    if (sEventThread != nullptr) {
      sEventThread->StopAndWaitForThread();
      delete sEventThread;
      sEventThread = nullptr;
    }

#if __WinSock__
    ::WSACleanup();
#endif
  }

  static EventThread *GetEventThread() { return sEventThread; }

  /**
   * Bind - binds the socket to the following address.
   * @return CF_FileNotOpen, CF_NoErr, or POSIX error code.
   */
  OS_Error Bind(UInt32 addr, UInt16 port, bool test = false);

  /**
   * Unbind - unbinds the socket.
   */
  void Unbind();

  void ReuseAddr();

  void NoDelay();

  void KeepAlive();

  void SetSocketBufSize(UInt32 inNewSize);

  /**
   * @return Returns an error if the socket buffer size is too big
   */
  OS_Error SetSocketRcvBufSize(UInt32 inNewSize);

  /**
   * Send
   * @return CF_FileNotOpen, CF_NoErr, or POSIX error code.
   */
  OS_Error Send(char const *inData, UInt32 inLength, UInt32 *outLengthSent);

  /**
   * Read - reads some data.
   * @return CF_FileNotOpen, CF_NoErr, or POSIX error code.
   */
  OS_Error Read(void *buffer, UInt32 length, UInt32 *rcvLen);

  /**
   * WriteV - same as Send, but takes an iovec
   * @return CF_FileNotOpen, CF_NoErr, or POSIX error code.
   */
  OS_Error WriteV(const struct iovec *iov, UInt32 numVecs, UInt32 *outLengthSent);

  // You can query for the Socket's state

  bool IsConnected() { return (bool) (fState & kConnected); }

  bool IsBound() { return (bool) (fState & kBound); }

  //If the Socket is bound, you may find out to which addr it is bound
  UInt32 GetLocalAddr() { return ntohl(fLocalAddr.sin_addr.s_addr); }

  UInt16 GetLocalPort() { return ntohs(fLocalAddr.sin_port); }

  StrPtrLen *GetLocalAddrStr();

  StrPtrLen *GetLocalPortStr();

  StrPtrLen *GetLocalDNSStr();

  enum {
    kMaxNumSockets = 4096   //UInt32
  };

  enum {
    // Pass this in on Socket constructors to specify whether the
    // Socket should be non-blocking or blocking
    kNonBlockingSocketType = 0x0001U,
    kEdgeTriggeredSocketMode = 0x0002U,
  };

 protected:

  Socket(Thread::Task *inNotifyTask, UInt32 inSocketType);

  ~Socket() override = default;

  /**
   * @return returns QTSS_NoErr, or appropriate posix error
   */
  OS_Error Open(int theType);

  UInt32 fState;

  enum {
    kPortBufSizeInBytes = 8,    //UInt32
    kMaxIPAddrSizeInBytes = 20  //UInt32
  };

#if DEBUG_SOCKET
  StrPtrLen fLocalAddrStr;
  char fLocalAddrBuffer[kMaxIPAddrSizeInBytes];
#endif

  // address information (available if bound)
  // these are always stored in network order. Conver
  struct sockaddr_in fLocalAddr;
  struct sockaddr_in fDestAddr;

  StrPtrLen *fLocalAddrStrPtr;
  StrPtrLen *fLocalDNSStrPtr;
  char fPortBuffer[kPortBufSizeInBytes];
  StrPtrLen fPortStr;

  // State flags. Be careful when changing these values, as subclasses add
  // their own
  enum {
    kBound = 0x0004,
    kConnected = 0x0008
  };

  static EventThread *sEventThread;

};

} // namespace Net
} // namespace CF

#endif // __SOCKET_H__
