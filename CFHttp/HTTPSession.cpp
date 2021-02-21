/*
	File:       HTTPSession.cpp
	Contains:
*/

#include <CF/CFEnv.h>
#include <CF/Net/Http/HTTPSession.h>

#if __FreeBSD__ || __hpux__
#include <unistd.h>
#endif

#if __solaris__ || __Linux__ || __sgi__ || __hpux__
#endif

using namespace CF::Net;

HTTPSession::HTTPSession()
    : HTTPSessionInterface(),
      fRequest(nullptr),
      fResponse(nullptr),
      fReadMutex(),
      fState(kReadingFirstRequest) {
  this->SetTaskName("HTTPSession");
}

HTTPSession::~HTTPSession() {
  char msgStr[2048] = {0};
  s_snprintf(msgStr,
             sizeof(msgStr),
             "HTTPSession offline from ip[%s]",
             fSocket.GetRemoteAddrStr()->Ptr);

  fLiveSession = false; //used in Clean up request to remove the RTP session.
  this->CleanupRequestAndResponse();// Make sure that all our objects are deleted
}

SInt64 HTTPSession::Run() {
  EventFlags events = this->GetEvents();
  CF_Error err = CF_NoErr;

  // Some callbacks look for this struct in the Thread object
  Core::ThreadDataSetter theSetter(nullptr, nullptr);

  if (events & Thread::Task::kKillEvent)
    fLiveSession = false;

  if (events & Thread::Task::kTimeoutEvent) {
    /* Session超时,释放Session */
    return -1;
  }

  while (this->IsLiveSession()) {
    switch (fState) {
      case kReadingFirstRequest: {
        /* 读取请求报文 */

        if ((err = fInputStream.ReadRequest()) == CF_NoErr) {
          /*
             如果 RequestStream 返回 CF_NoErr，就表示已经读取了目前所到达的
             网络数据。但还不能构成一个整体报文，还要继续等待读取...
           */
          fInputSocketP->RequestEvent(EV_RE);
          return 0;
        }

        if ((err != CF_RequestArrived) && (err != E2BIG)) {
          // Any other error implies that the client has gone away.
          // At this point, we can't have 2 sockets, so we don't need to do
          // the "half closed" check. we do below
          Assert(err > 0);
          Assert(!this->IsLiveSession());
          break;
        }

        if ((err == CF_RequestArrived) || (err == E2BIG))
          fState = kHaveCompleteMessage;

        continue;
      }

      case kReadingRequest: {
        /* 读取请求报文 */

        Core::MutexLocker readMutexLocker(&fReadMutex);

        if ((err = fInputStream.ReadRequest()) == CF_NoErr) {
          fInputSocketP->RequestEvent(EV_RE);
          return 0;
        }

        if ((err != CF_RequestArrived) && (err != E2BIG)
            && (err != CF_BadArgument)) {
          // Any other error implies that the input connection has gone away.
          // We should only kill the whole session if we aren't doing HTTP.
          // (If we are doing HTTP, the POST connection can go away)
          Assert(err > 0);
          if (fOutputSocketP->IsConnected()) {
            // If we've gotten here, this must be an HTTP session with
            // a dead input connection. If that's the case, we should
            // clean up immediately so as to not have an open Socket
            // needlessly lingering around, taking up space.
            Assert(fOutputSocketP != fInputSocketP);
            Assert(!fInputSocketP->IsConnected());
            fInputSocketP->Cleanup();
            return 0;
          } else {
            Assert(!this->IsLiveSession());
            break;
          }
        }

        fState = kHaveCompleteMessage;
      }

      case kHaveCompleteMessage: {
        /* 已经读取到完整的请求报文，进行初始化 */

        Assert(fInputStream.GetRequestBuffer());

        Assert(fRequest == nullptr);
        Assert(fResponse == nullptr);
        fRequest = new HTTPPacket(fInputStream.GetRequestBuffer());
        fResponse = new HTTPPacket(httpResponseType);

        /*
           在这里，我们已经读取了一个完整的 Request，并准备进行请求的处理，
           直到响应报文发出。
           在此过程中，此 Session 的 Socket 不进行任何网络数据的读/写；
         */
        fReadMutex.Lock();
        fSessionMutex.Lock();

        fOutputStream.ResetBytesWritten();

        if (err == E2BIG) {
          fResponse->SetStatusCode(httpInternalServerError);
          fState = kSendingResponse;
          break;
        }

        // Check for a corrupt base64 error, return an error
        if (err == CF_BadArgument) {
          fResponse->SetStatusCode(httpInternalServerError);
          fState = kSendingResponse;
          break;
        }

        Assert(err == CF_RequestArrived);
        fState = kFilteringRequest;
      }

      case kFilteringRequest: {
        /* 对请求进行过滤，解析报文 */

        /* 刷新Session保活时间 */
        fTimeoutTask.RefreshTimeout();

        /* 对请求报文进行解析，读取 body */
        CF_Error theErr = SetupRequest();

        /* 当 SetupRequest 步骤未读取到完整的网络报文，需要进行等待 */
        if (theErr == CF_WouldBlock) {
          this->ForceSameThread();
          fInputSocketP->RequestEvent(EV_RE);
          // We are holding mutexes, so we need to force
          // the same Thread to be used for next Run()
          // when next run, the fState also is kFilteringRequest, so we will
          // call SetupRequest for continue read body.
          return 0;
        }

        fState = kPreprocessingRequest;
        break;
      }

      case kPreprocessingRequest: {
        /* 请求预处理过程 */

        if (fRequest->GetVersion() == httpIllegalVersion) {
          fResponse->SetStatusCode(httpHTTPVersionNotSupported);
          fState = kSendingResponse;
        }

        fState = kProcessingRequest;
        break;
      }

      case kProcessingRequest: {

        // doDispatch
        CF_Error theErr = sDispatcher->Dispatch(*fRequest, *fResponse);
        if (theErr == CF_FileNotFound) {
          fResponse->SetStatusCode(httpNotFound);
        } else if (theErr != CF_NoErr) {
          fResponse->SetStatusCode(httpInternalServerError);
        }

        fState = kSendingResponse;
      }

      case kSendingResponse: {
        /* 响应报文发送，确保完全发送 */
        Assert(fRequest != nullptr);
        Assert(fResponse != nullptr);

        /* 构造响应信息 */
        CF_Error theErr = SetupResponse();

        if (fOutputStream.GetBytesWritten() == 0) {
          fState = kCleaningUp;
          break;
        }

        /* 发送响应报文 */
        err = fOutputStream.Flush();

        if (err == EAGAIN) {
          // If we get this error, we are currently flow-controlled and should
          // wait for the Socket to become writeable again
          /* 如果收到Socket EAGAIN错误，那么我们需要等Socket再次可写的时候再调用发送 */
          fSocket.RequestEvent(EV_WR);
          this->ForceSameThread();
          // We are holding mutexes, so we need to force
          // the same Thread to be used for next Run()
          return 0;
        } else if (err != CF_NoErr) {
          // Any other error means that the client has disconnected, right?
          Assert(!this->IsLiveSession());
          break;
        }

        fState = kCleaningUp;
      }

      case kCleaningUp: {
        // Cleaning up consists of making sure we've read all the incoming Request Body
        // data off of the Socket
        if (this->GetRemainingReqBodyLen() > 0) {
          err = this->dumpRequestData();

          if (err == EAGAIN) {
            fInputSocketP->RequestEvent(EV_RE);
            this->ForceSameThread();    // We are holding mutexes, so we need to force
            // the same Thread to be used for next Run()
            return 0;
          }
        }

        /* 一次请求的读取、处理、响应过程完整，等待下一次网络报文！ */
        this->CleanupRequestAndResponse();
        fState = kReadingRequest;
      }
      default: break;
    }
  }

  /* 清空Session占用的所有资源 */
  this->CleanupRequestAndResponse();

  /* Session引用数为0，返回-1后，系统会将此Session删除 */
  if (fObjectHolders == 0)
    return -1;

  /*
     如果流程走到这里，Session实际已经无效了，应该被删除。
     但没有，因为还有其他地方引用了Session对象
   */
  return 0;
}

CF_Error HTTPSession::SendHTTPPacket(CF::StrPtrLen *contentXML,
                                     bool connectionClose,
                                     bool decrement) {
  HTTPPacket httpAck(&CFEnv::GetServerHeader());
  httpAck.CreateResponseHeader();
  if (contentXML->Len)
    httpAck.AppendContentLengthHeader(contentXML->Len);

  if (connectionClose)
    httpAck.AppendConnectionCloseHeader();

  char respHeader[2048] = {0};
  StrPtrLen *ackPtr = httpAck.GetCompleteHTTPHeader();
  strncpy(respHeader, ackPtr->Ptr, ackPtr->Len);

  HTTPResponseStream *pOutputStream = GetOutputStream();
  pOutputStream->Put(respHeader);
  if (contentXML->Len > 0)
    pOutputStream->Put(contentXML->Ptr, contentXML->Len);

  if (pOutputStream->GetBytesWritten() != 0) {
    pOutputStream->Flush();
  }

  /* 将对HTTPSession的引用减少 1 */
  if (fObjectHolders && decrement)
    DecrementObjectHolderCount();

  if (connectionClose)
    this->Signal(Thread::Task::kKillEvent);

  return CF_NoErr;
}

/*
 * 解析请求报文
 */
CF_Error HTTPSession::SetupRequest() {
  CF_Error theErr;

  /* 解析 head */
  if (fRequest->GetHTTPType() == httpIllegalType) {
    theErr = fRequest->Parse();
    if (theErr != CF_NoErr)
      return CF_BadArgument;
  }

  /* 解析 body */
  StrPtrLen *lengthPtr = fRequest->GetHeaderValue(httpContentLengthHeader);
  StringParser theContentLenParser(lengthPtr);
  theContentLenParser.ConsumeWhitespace();
  UInt32 content_length = theContentLenParser.ConsumeInteger(nullptr);

  if (content_length) {
    s_printf("HTTPSession read content-length:%d \n", content_length);
    // Check for the existence of 2 attributes in the request: a pointer to our buffer for
    // the request body, and the current offset in that buffer. If these attributes exist,
    // then we've already been here for this request. If they don't exist, add them.
    UInt32 theBufferOffset = 0;
    char *theRequestBody = nullptr;
    StrPtrLen *requestBody = fRequest->GetBody();
    if (requestBody != nullptr) {
      theRequestBody = requestBody->Ptr;
      theBufferOffset = requestBody->Len;
    } else {
      theRequestBody = new char[content_length + 1];
      memset(theRequestBody, 0, content_length + 1);
      requestBody = new StrPtrLenDel(theRequestBody, 0);
      fRequest->SetBody(requestBody);
    }

    UInt32 theLen;

    // We have our buffer and offset. Read the data.
    theErr = fInputStream.Read(theRequestBody + theBufferOffset,
                               content_length - theBufferOffset,
                               &theLen);
    Assert(theErr != CF_BadArgument);

    if (theErr == CF_RequestFailed) {
      // NEED TO RETURN HTTP ERROR RESPONSE
      return CF_RequestFailed;
    }

    // Update our offset in the buffer
    requestBody->Len = theBufferOffset + theLen;

    s_printf("Add Len:%d \n", theLen);
    if ((theErr == CF_WouldBlock) ||
        (theLen < (content_length - theBufferOffset))) {

      // The entire content body hasn't arrived yet.
      // Request a read event and wait for it.

      return CF_WouldBlock;
    }

    Assert(theErr == CF_NoErr);
  }

  s_printf("get complete http msg:%s QueryString:%s \n",
           fRequest->GetRequestPath(),
           fRequest->GetQueryString());

  return CF_NoErr;
}

CF_Error HTTPSession::SetupResponse() {
  if (fResponse->GetVersion() == httpIllegalVersion) {
    HTTPVersion requestVersion = fRequest->GetVersion();
    fResponse->SetVersion(
        requestVersion != httpIllegalVersion ? requestVersion : http11Version);
  }

  // construct response header
  fResponse->CreateResponseHeader();

  StrPtrLen *respBody = fResponse->GetBody();
  if (respBody != NULL && respBody->Len > 0)
    fResponse->AppendContentLengthHeader(respBody->Len);
  else
    fResponse->AppendContentLengthHeader((UInt32) 0);

  if (!fRequest->IsRequestKeepAlive()) {
    fResponse->AppendConnectionCloseHeader();
  }

  char respHeader[2048] = {0};
  StrPtrLen *ackPtr = fResponse->GetCompleteHTTPHeader();
  strncpy(respHeader, ackPtr->Ptr, ackPtr->Len);

  fOutputStream.Put(respHeader);

  // construct response body
  if (respBody != NULL && respBody->Len > 0)
    fOutputStream.Put(*respBody);

  return CF_NoErr;
}

void HTTPSession::CleanupRequestAndResponse() {

  if (fRequest != nullptr) {
    if (!fRequest->IsRequestKeepAlive())
      this->Signal(Thread::Task::kKillEvent);

    // nullptr out any references to the current request
    delete fRequest;
    fRequest = nullptr;
  }

  if (fResponse != nullptr) {
    delete fResponse;
    fResponse = nullptr;
  }

  fSessionMutex.Unlock();
  fReadMutex.Unlock();

  // Clear out our last value for request body length before moving onto
  // the next request
  this->SetRequestBodyLength(-1);
}

bool HTTPSession::OverMaxConnections(UInt32 buffer) {

  return false;
}

CF_Error HTTPSession::dumpRequestData() {
  char theDumpBuffer[CF_MAX_REQUEST_BUFFER_SIZE];

  CF_Error theErr = CF_NoErr;
  while (theErr == CF_NoErr)
    theErr = this->Read(theDumpBuffer, CF_MAX_REQUEST_BUFFER_SIZE, nullptr);

  return theErr;
}
