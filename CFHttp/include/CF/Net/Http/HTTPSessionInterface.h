/*
	File:       HTTPSessionInterface.h
	Contains:
*/

#ifndef __HTTP_SESSION_INTERFACE_H__
#define __HTTP_SESSION_INTERFACE_H__

#include <atomic>
#include <CF/CFDef.h>
#include <CF/Net/Socket/TCPSocket.h>
#include <CF/Thread/TimeoutTask.h>
#include <CF/Net/Http/HTTPRequestStream.h>
#include <CF/Net/Http/HTTPResponseStream.h>
#include <CF/Net/Http/HTTPDispatcher.h>

namespace CF::Net {

class HTTPSessionInterface : public Thread::Task {
 public:

  /**
   * @brief 初始化 http server, 创建 HTTPDispatcher
   * @param mapping - CGI 程序映射表
   */
  static void Initialize(HTTPMapping *mapping);

  static void Release() {
    delete sDispatcher;
    sDispatcher = nullptr;
  }

  HTTPSessionInterface();
  virtual ~HTTPSessionInterface();

  bool IsLiveSession() { return fSocket.IsConnected() && fLiveSession; }

  void RefreshTimeout() { fTimeoutTask.RefreshTimeout(); }

  void IncrementObjectHolderCount() { ++fObjectHolders; }
  void DecrementObjectHolderCount();

  HTTPRequestStream *GetInputStream() { return &fInputStream; }
  HTTPResponseStream *GetOutputStream() { return &fOutputStream; }
  TCPSocket *GetSocket() { return &fSocket; }
  Core::Mutex *GetSessionMutex() { return &fSessionMutex; }

  UInt32 GetSessionIndex() { return fSessionIndex; }

  void SetRequestBodyLength(SInt32 inLength) { fRequestBodyLen = inLength; }
  SInt32 GetRemainingReqBodyLen() { return fRequestBodyLen; }

  // QTSS STREAM FUNCTIONS

  // Allows non-buffered writes to the client. These will flow control.

  // THE FIRST ENTRY OF THE IOVEC MUST BE BLANK!!!
  virtual CF_Error WriteV(iovec *inVec,
                          UInt32 inNumVectors,
                          UInt32 inTotalLength,
                          UInt32 *outLenWritten);
  virtual CF_Error Write(void *inBuffer,
                         UInt32 inLength,
                         UInt32 *outLenWritten,
                         CF_WriteFlags inFlags);
  virtual CF_Error Read(void *ioBuffer, UInt32 inLength, UInt32 *outLenRead);
  virtual CF_Error RequestEvent(CF_EventType inEventMask);

  virtual CF_Error SendHTTPPacket(StrPtrLen *contentXML,
                                  bool connectionClose,
                                  bool decrement);

  enum {
    kMaxUserNameLen = 32,
    kMaxUserPasswordLen = 32
  };

 protected:
  enum {
    kFirstHTTPSessionID = 1,    //UInt32
  };

  //Each http session has a unique number that identifies it.

  char fUserNameBuf[kMaxUserNameLen];
  char fUserPasswordBuf[kMaxUserPasswordLen];

  Thread::TimeoutTask fTimeoutTask; // allows the session to be timed out

  HTTPRequestStream fInputStream;
  HTTPResponseStream fOutputStream;

  // Any RTP session sending interleaved data on this RTSP session must
  // be prevented from writing while an RTSP request is in progress
  Core::Mutex fSessionMutex;

  //+rt  Socket we get from "accept()"
  TCPSocket fSocket;
  TCPSocket *fOutputSocketP;
  TCPSocket *fInputSocketP;  // <-- usually same as fSocketP, unless we're HTTP Proxying

  void SnarfInputSocket(HTTPSessionInterface *fromHTTPSession);

  bool fLiveSession;
  std::atomic_int fObjectHolders;

  UInt32 fSessionIndex;
  UInt32 fLocalAddr;
  UInt32 fRemoteAddr;
  SInt32 fRequestBodyLen;

  UInt16 fLocalPort;
  UInt16 fRemotePort;

  bool fAuthenticated;

  static std::atomic_uint sSessionIndexCounter;

  static HTTPDispatcher *sDispatcher;

  // Dictionary support Param retrieval function
  static void *SetupParams(HTTPSessionInterface *inSession, UInt32 *outLen);
};

}

#endif // __HTTP_SESSION_INTERFACE_H__
