#ifndef __HTTP_SESSION_H__
#define __HTTP_SESSION_H__

#include "HTTPSessionInterface.h"
#include "HTTPRequest.h"

class HTTPSession : public HTTPSessionInterface {
 public:
  HTTPSession();
  virtual ~HTTPSession();

  //Send HTTPPacket
  CF_Error SendHTTPPacket(StrPtrLen *contentXML,
                            bool connectionClose,
                            bool decrement) override;

 private:
  SInt64 Run() override;

  // Does request prep & request cleanup, respectively

  CF_Error SetupRequest();
  void CleanupRequest();

  // test current connections handled by this object against server pref connection limit
  static inline bool OverMaxConnections(UInt32 buffer);

  CF_Error dumpRequestData();

  HTTPRequest *fRequest;
  OSMutex fReadMutex;

  enum {
    kReadingRequest = 0,
    kFilteringRequest = 1,
    kPreprocessingRequest = 2,
    kProcessingRequest = 3,
    kSendingResponse = 4,
    kCleaningUp = 5,

    kReadingFirstRequest = 6,
    kHaveCompleteMessage = 7
  };

  UInt32 fState;
};

#endif // __HTTP_SESSION_H__

