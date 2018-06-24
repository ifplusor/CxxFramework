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
    File:       UDPSocketPool.cpp

    Contains:   Object that creates & maintains UDP Socket pairs in a pool.

*/

#include <CF/Net/Socket/UDPSocketPool.h>
#include <CF/Net/Socket/SocketUtils.h>

using namespace CF::Net;

/**
 * 获取 UDPSocket Pair
 * @param inIPAddr  local ip
 * @param inPort    local port
 * @param inSrcIPAddr  remote ip
 * @param inSrcPort    remote port
 */
UDPSocketPair *UDPSocketPool::GetUDPSocketPair(UInt32 inIPAddr, UInt16 inPort, UInt32 inSrcIPAddr, UInt16 inSrcPort) {
  // convert NAT ip to local ip
  inIPAddr = SocketUtils::ConvertToLocalAddr(inIPAddr);

  Core::MutexLocker locker(&fMutex);
  if ((inSrcIPAddr != 0) || (inSrcPort != 0)) {
    /* If we find a pair that is:
     *   a) on the right IP address,
     *   b) doesn't have this source IP & port in the demuxer already,
     * we can return this pair */
    for (QueueIter qIter(&fUDPQueue); !qIter.IsDone(); qIter.Next()) {
      auto theElem = (UDPSocketPair *) qIter.GetCurrent()->GetEnclosingObject();
      if ((theElem->fSocketA->GetLocalAddr() == inIPAddr) &&
          ((inPort == 0) || (theElem->fSocketA->GetLocalPort() == inPort))) {
        // check to make sure this source IP & port is not already in the demuxer.
        // If not, we can return this Socket pair.
        if ((theElem->fSocketB->GetDemuxer() == nullptr) ||
            ((!theElem->fSocketB->GetDemuxer()->AddrInMap(0, 0)) &&
                (!theElem->fSocketB->GetDemuxer()->AddrInMap(inSrcIPAddr, inSrcPort)))) {
          theElem->fRefCount++;
          return theElem;
        }

        // If port is specified, there is NO WAY a Socket pair can exist that matches
        // the criteria (because caller wants a specific ip & port combination)
        if (inPort != 0)
          return nullptr;
      }
    }
  }

  // if we get here, there is no pair already in the pool that matches the specified
  // criteria, so we have to create a new pair.
  return this->CreateUDPSocketPair(inIPAddr, inPort);
}

void UDPSocketPool::ReleaseUDPSocketPair(UDPSocketPair *inPair) {
  Core::MutexLocker locker(&fMutex);
  inPair->fRefCount--;
  if (inPair->fRefCount == 0) {
    fUDPQueue.Remove(&inPair->fElem);
    this->DestructUDPSocketPair(inPair);
  }
}

UDPSocketPair *UDPSocketPool::CreateUDPSocketPair(UInt32 inAddr, UInt16 inPort) {
  // try to find an open pair of ports to bind these suckers tooo
  Core::MutexLocker locker(&fMutex);
  UDPSocketPair *thePair = nullptr;
  UInt16 socketAPort = kLowestUDPPort;
  UInt16 socketBPort;

  // If port is 0, then the caller doesn't care what port # we bind this Socket to.
  // Otherwise, ONLY attempt to bind this Socket to the specified port
  if (inPort != 0) socketAPort = inPort;

  while (socketAPort < kHighestUDPPort) {
    socketBPort = static_cast<UInt16>(socketAPort + 1);  // make Socket pairs adjacent to one another

    thePair = ConstructUDPSocketPair();  // 创建一个 udp Socket pair
    Assert(thePair != nullptr);

    // check construct udp socket pair fail
    if (thePair == nullptr) return nullptr;

    // 创建数据报 Socket 端口
    if (thePair->fSocketA->Open() != OS_NoErr || thePair->fSocketB->Open() != OS_NoErr) break;

    // Set Socket options on these new sockets. 主要是设置 Socket buf size
    this->SetUDPSocketOptions(thePair);

    // 在两个 Socket 端口上执行 bind 操作,两个 port 相差 1.
    OS_Error theErr = thePair->fSocketA->Bind(inAddr, socketAPort);
    if (theErr == OS_NoErr) {
      //s_printf("fSocketA->Bind ok on port%u\n", socketAPort);
      theErr = thePair->fSocketB->Bind(inAddr, socketBPort);
      if (theErr == OS_NoErr) {
        //s_printf("fSocketB->Bind ok on port%u\n", socketBPort);
        fUDPQueue.EnQueue(&thePair->fElem);
        thePair->fRefCount++;

        return thePair;
      }
      //else s_printf("fSocketB->Bind failed on port%u\n", socketBPort);
    }
    //else s_printf("fSocketA->Bind failed on port%u\n", socketAPort);

    // If we are looking to bind to a specific port set, and we couldn't then just break here.
    if (inPort != 0) break;

    socketAPort += 2; //try a higher port pair

    this->DestructUDPSocketPair(thePair); //a bind failure
    thePair = nullptr;
  }

  // if we couldn't find a pair of sockets, make sure to clean up our mess
  if (thePair != nullptr) this->DestructUDPSocketPair(thePair);

  return nullptr;
}
