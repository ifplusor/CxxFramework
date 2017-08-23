/*
	File:       HTTPSession.cpp
	Contains:
*/

#include "HTTPSession.h"
#include "OSArrayObjectDeleter.h"

using namespace std;

#if __FreeBSD__ || __hpux__
#include <unistd.h>
#endif

#if __solaris__ || __linux__ || __sgi__ || __hpux__
#endif

HTTPSession::HTTPSession()
    : HTTPSessionInterface(),
      fRequest(nullptr),
      fReadMutex(),
      fState(kReadingFirstRequest) {
  this->SetTaskName("HTTPSession");
}

HTTPSession::~HTTPSession() {
  char msgStr[2048] = {0};
  qtss_snprintf(msgStr,
                sizeof(msgStr),
                "HTTPSession offline from ip[%s]",
                fSocket.GetRemoteAddrStr()->Ptr);

  fLiveSession = false; //used in Clean up request to remove the RTP session.
  this->CleanupRequest();// Make sure that all our objects are deleted
}

SInt64 HTTPSession::Run() {
  EventFlags events = this->GetEvents();
  CF_Error err = CF_NoErr;

  // Some callbacks look for this struct in the thread object
  OSThreadDataSetter theSetter(nullptr, nullptr);

  if (events & Task::kKillEvent)
    fLiveSession = false;

  if (events & Task::kTimeoutEvent) {
    // Session超时,释放Session
    return -1;
  }

  while (this->IsLiveSession()) {
    switch (fState) {
      case kReadingFirstRequest: {
        // 读取请求报文

        if ((err = fInputStream.ReadRequest()) == CF_NoErr) {
          // 如果RequestStream返回QTSS_NoErr，就表示已经读取了目前所到达的网络数据
          // 但，还不能构成一个整体报文，还要继续等待读取...
          fInputSocketP->RequestEvent(EV_RE);
          return 0;
        }

        if ((err != CF_RequestArrived) && (err != E2BIG)) {
          // Any other error implies that the client has gone away. At this point,
          // we can't have 2 sockets, so we don't need to do the "half closed" check
          // we do below
          Assert(err > 0);
          Assert(!this->IsLiveSession());
          break;
        }

        if ((err == CF_RequestArrived) || (err == E2BIG))
          fState = kHaveCompleteMessage;

        continue;
      }

      case kReadingRequest: {
        // 读取请求报文

        OSMutexLocker readMutexLocker(&fReadMutex);

        if ((err = fInputStream.ReadRequest()) == CF_NoErr) {
          // 如果RequestStream返回QTSS_NoErr，就表示已经读取了目前所到达的网络数据
          // 但，还不能构成一个整体报文，还要继续等待读取...
          fInputSocketP->RequestEvent(EV_RE);
          return 0;
        }

        if ((err != CF_RequestArrived) && (err != E2BIG)
            && (err != CF_BadArgument)) {
          //Any other error implies that the input connection has gone away.
          // We should only kill the whole session if we aren't doing HTTP.
          // (If we are doing HTTP, the POST connection can go away)
          Assert(err > 0);
          if (fOutputSocketP->IsConnected()) {
            // If we've gotten here, this must be an HTTP session with
            // a dead input connection. If that's the case, we should
            // clean up immediately so as to not have an open socket
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
        // 已经读取到完整的请求报文，进行初始化

        Assert(fInputStream.GetRequestBuffer());

        Assert(fRequest == nullptr);
        fRequest = new HTTPRequest(&HTTPSessionInterface::GetServerHeader(),
                                   fInputStream.GetRequestBuffer());

        // 在这里，我们已经读取了一个完整的Request，并准备进行请求的处理，
        // 直到响应报文发出。
        // 在此过程中，此Session的Socket不进行任何网络数据的读/写；
        fReadMutex.Lock();
        fSessionMutex.Lock();

        fOutputStream.ResetBytesWritten();

        if (err == E2BIG) {
          // 返回HTTP报文，错误码 408
          //(void)QTSSModuleUtils::SendErrorResponse(fRequest, qtssClientBadRequest, qtssMsgRequestTooLong);
          fState = kSendingResponse;
          break;
        }

        // Check for a corrupt base64 error, return an error
        if (err == CF_BadArgument) {
          // 返回HTTP报文，错误码 408
          //(void)QTSSModuleUtils::SendErrorResponse(fRequest, qtssClientBadRequest, qtssMsgBadBase64);
          fState = kSendingResponse;
          break;
        }

        Assert(err == CF_RequestArrived);
        fState = kFilteringRequest;
      }

      case kFilteringRequest: {
        // 对请求进行过滤，解析报文

        // 刷新Session保活时间
        fTimeoutTask.RefreshTimeout();

        // 对请求报文进行解析
        CF_Error theErr = SetupRequest();

        // 当SetupRequest步骤未读取到完整的网络报文，需要进行等待
        if (theErr == CF_WouldBlock) {
          this->ForceSameThread();
          fInputSocketP->RequestEvent(EV_RE);
          // We are holding mutexes, so we need to force
          // the same thread to be used for next Run()
          return 0; // 返回0表示有事件才进行通知，返回>0表示规定事件后调用Run
        }

        // 每一步都检测响应报文是否已完成，完成则直接进行回复响应
        if (fOutputStream.GetBytesWritten() > 0) {
          fState = kSendingResponse;
          break;
        }

        fState = kPreprocessingRequest;
        break;
      }

      case kPreprocessingRequest: {
        // 请求预处理过程
        fState = kProcessingRequest;
        break;
      }

      case kProcessingRequest: {

        StrPtrLen content("nothing\n", 8);
        SendHTTPPacket(&content, true, true);

        if (fOutputStream.GetBytesWritten() == 0) {
          // 响应报文还没有形成
          //QTSSModuleUtils::SendErrorResponse(fRequest, qtssServerInternal, qtssMsgNoModuleForRequest);
          fState = kCleaningUp;
          break;
        }

        fState = kSendingResponse;
      }

      case kSendingResponse: {
        // 响应报文发送，确保完全发送
        Assert(fRequest != nullptr);

        // 发送响应报文
        err = fOutputStream.Flush();

        if (err == EAGAIN) {
          // If we get this error, we are currently flow-controlled and should
          // wait for the socket to become writeable again
          // 如果收到Socket EAGAIN错误，那么我们需要等Socket再次可写的时候再调用发送
          fSocket.RequestEvent(EV_WR);
          this->ForceSameThread();
          // We are holding mutexes, so we need to force
          // the same thread to be used for next Run()
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
        // data off of the socket
        if (this->GetRemainingReqBodyLen() > 0) {
          err = this->dumpRequestData();

          if (err == EAGAIN) {
            fInputSocketP->RequestEvent(EV_RE);
            this->ForceSameThread();    // We are holding mutexes, so we need to force
            // the same thread to be used for next Run()
            return 0;
          }
        }

        // 一次请求的读取、处理、响应过程完整，等待下一次网络报文！
        this->CleanupRequest();
        fState = kReadingRequest;
      }
      default: break;
    }
  }

  // 清空Session占用的所有资源
  this->CleanupRequest();

  // Session引用数为0，返回-1后，系统会将此Session删除
  if (fObjectHolders == 0)
    return -1;

  // 如果流程走到这里，Session实际已经无效了，应该被删除，但没有，因为还有其他地方引用了Session对象
  return 0;
}

CF_Error HTTPSession::SendHTTPPacket(StrPtrLen *contentXML,
                                       bool connectionClose,
                                       bool decrement) {
  HTTPRequest httpAck(&HTTPSessionInterface::GetServerHeader(), httpResponseType);
  httpAck.CreateResponseHeader(contentXML->Len ? httpOK : httpNotImplemented);
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

  //将对HTTPSession的引用减少一
  if (fObjectHolders && decrement)
    DecrementObjectHolderCount();

  if (connectionClose)
    this->Signal(Task::kKillEvent);

  return CF_NoErr;
}

/*
 * 解析请求报文
 */
CF_Error HTTPSession::SetupRequest() {
  CF_Error theErr = fRequest->Parse();
  if (theErr != CF_NoErr)
    return CF_BadArgument;

  if (fRequest->GetRequestPath() != nullptr) {
    string sRequest(fRequest->GetRequestPath());

    if (!sRequest.empty()) {
//      boost::to_lower(sRequest);
//
//      vector<string> path;
//      if (boost::ends_with(sRequest, "/")) {
//        boost::erase_tail(sRequest, 1);
//      }
//      boost::split(path, sRequest,
//                   boost::is_any_of("/"),
//                   boost::token_compress_on);
//      if (path.size() == 3) {
//
//      }

      return CF_NoErr;
    }
  }

  // 解析 body

  StrPtrLen *lengthPtr = fRequest->GetHeaderValue(httpContentLengthHeader);
  StringParser theContentLenParser(lengthPtr);
  theContentLenParser.ConsumeWhitespace();
  UInt32 content_length = theContentLenParser.ConsumeInteger(nullptr);

  if (content_length) {
    qtss_printf("HTTPSession read content-length:%d \n", content_length);
    // Check for the existence of 2 attributes in the request: a pointer to our buffer for
    // the request body, and the current offset in that buffer. If these attributes exist,
    // then we've already been here for this request. If they don't exist, add them.
    UInt32 theBufferOffset = 0;
    char *theRequestBody = nullptr;
    UInt32 theLen = sizeof(theRequestBody);
    // 获取 easyHTTPSesContentBody
    if (theErr != CF_NoErr) {
      // First time we've been here for this request. Create a buffer for the
      // content body and shove it in the request.
      theRequestBody = new char[content_length + 1];
      memset(theRequestBody, 0, content_length + 1);
      theLen = sizeof(theRequestBody);
      // 设置 easyHTTPSesContentBody

      // Also store the offset in the buffer
      theLen = sizeof(theBufferOffset);
      // 设置 easyHTTPSesContentBodyOffset

      Assert(theErr == CF_NoErr);
    }

    theLen = sizeof(theBufferOffset);
    // 获取 easyHTTPSesContentBodyOffset

    // We have our buffer and offset. Read the data.
    //theErr = QTSS_Read(this, theRequestBody + theBufferOffset, content_length - theBufferOffset, &theLen);
    theErr = fInputStream.Read(theRequestBody + theBufferOffset,
                               content_length - theBufferOffset,
                               &theLen);
    Assert(theErr != CF_BadArgument);

    if (theErr == CF_RequestFailed) {
      OSCharArrayDeleter charArrayPathDeleter(theRequestBody);
      //
      // NEED TO RETURN HTTP ERROR RESPONSE
      return CF_RequestFailed;
    }

    qtss_printf("Add Len:%d \n", theLen);
    if ((theErr == CF_WouldBlock)
        || (theLen < (content_length - theBufferOffset))) {
      //
      // Update our offset in the buffer
      theBufferOffset += theLen;
      // 设置 easyHTTPSesContentBodyOffset

      // The entire content body hasn't arrived yet. Request a read event and wait for it.

      Assert(theErr == CF_NoErr);
      return CF_WouldBlock;
    }

    Assert(theErr == CF_NoErr);

    OSCharArrayDeleter charArrayPathDeleter(theRequestBody);
    // 处理 body

    UInt32 offset = 0;
    // 清空 easyHTTPSesContentBodyOffset

    char *content = nullptr;
    // 清空 easyHTTPSesContentBody
  }

  qtss_printf("get complete http msg:%s QueryString:%s \n",
              fRequest->GetRequestPath(),
              fRequest->GetQueryString());

  return CF_NoErr;
}

void HTTPSession::CleanupRequest() {

  fSessionMutex.Unlock();
  fReadMutex.Unlock();

  // Clear out our last value for request body length before moving onto the next request
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
