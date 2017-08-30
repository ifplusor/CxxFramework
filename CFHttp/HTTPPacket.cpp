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

#include <CF/Net/Http/HTTPPacket.h>
#include <CF/StringTranslator.h>
#include <CF/Core/DateTranslator.h>
#include <CF/Core/Thread.h>
#include <CF/CFEnv.h>

namespace CF::Net {

StrPtrLen HTTPPacket::sColonSpace(": ", 2);
static bool sFalse = false;
static bool sTrue = true;
static StrPtrLen sCloseString("close", 5);
static StrPtrLen sAllString("*", 1);
static StrPtrLen sKeepAliveString("keep-alive", 10);
static StrPtrLen sDefaultRealm("CxxFramework Server", 19);

UInt8 HTTPPacket::sURLStopConditions[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //0-9      '\t' is a stop condition
        1, 0, 0, 1, 0, 0, 0, 0, 0, 0, //10-19    '\r' & '\n' are stop conditions
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
        0, 0, 1, 0, 0, 0, 0, 0, 0, 0, //30-39    ' '
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //40-49
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //50-59
        0, 0, 0, 1, 0, 0, 0, 0, 0, 0, //60-69    '?'
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //70-79
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //80-89
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //90-99
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //100-109
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //110-119
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //120-129
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //130-139
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //140-149
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //150-159
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //160-169
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //170-179
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //180-189
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //190-199
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //200-209
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //210-219
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //220-229
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //230-239
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //240-249
        0, 0, 0, 0, 0, 0              //250-255
    };

// Constructor for parse a packet header
HTTPPacket::HTTPPacket(StrPtrLen *packetPtr)
    : fSvrHeader(CFEnv::GetServerHeader()),
      fPacketHeader(*packetPtr), // 浅拷贝
      fMethod(httpIllegalMethod),
      fVersion(httpIllegalVersion),
//      fRequestLine(),
//      fAbsoluteURI(),
//      fRelativeURI(),
//      fAbsoluteURIScheme(),
//      fHostHeader(),
      fRequestPath(nullptr),
      fQueryString(nullptr),
      fStatusCode(httpOK),
      fRequestKeepAlive(false), // Default value when there is no version string
      fHTTPHeader(nullptr),
      fHTTPHeaderFormatter(nullptr),
      fHTTPBody(nullptr),
      fHTTPType(httpIllegalType) { // 未解析情况下为httpIllegalType

}

// Constructor for creating a new packet
HTTPPacket::HTTPPacket(HTTPType httpType)
    : fSvrHeader(CFEnv::GetServerHeader()),
//      fPacketHeader(),
      fMethod(httpIllegalMethod),
      fVersion(httpIllegalVersion),
//      fRequestLine(),
//      fAbsoluteURI(),
//      fRelativeURI(),
//      fAbsoluteURIScheme(),
//      fHostHeader(),
      fRequestPath(nullptr),
      fQueryString(nullptr),
      fStatusCode(httpOK),
      fRequestKeepAlive(false), // Default value when there is no version string
      fHTTPHeader(nullptr),
      fHTTPHeaderFormatter(nullptr),
      fHTTPBody(nullptr),
      fHTTPType(httpType) {

  // We require the response but we allocate memory only when we call
  // CreateResponseHeader
}

// Destructor
HTTPPacket::~HTTPPacket() {
  // delete nullptr is no effect.

  if (fHTTPHeader != nullptr) {
    delete fHTTPHeader->Ptr;
  }
  delete fHTTPHeader;
  delete fHTTPHeaderFormatter;
  delete[] fRequestPath;
  delete[] fQueryString;

  if (fHTTPBody != nullptr) {
    delete fHTTPBody->Ptr;
  }
  delete fHTTPBody;
}

// Parses the request
CF_Error HTTPPacket::Parse() {
  Assert(fPacketHeader.Ptr != NULL);
  StringParser parser(&fPacketHeader);

  // Store the request line (used for logging)
  // (ex: GET /index.html HTTP/1.0)
  StringParser requestLineParser(&fPacketHeader);
  requestLineParser.ConsumeUntil(&fRequestLine, StringParser::sEOLMask);

  // Parse request line returns an error if there is an error in the
  // request URI or the formatting of the request line.
  // If the method or version are not found, they are set
  // to httpIllegalMethod or httpIllegalVersion respectively,
  // and QTSS_NoErr is returned.
  CF_Error err;
  do {
    err = parseRequestLine(&parser);
    if (err != CF_NoErr)
      return err;
  } while (fHTTPType == httpIllegalType);

  // Parse headers and set values of headers into fFieldValues array
  err = parseHeaders(&parser);
  if (err != CF_NoErr)
    return err;

  return CF_NoErr;
}

CF_Error HTTPPacket::parseRequestLine(StringParser *parser) {
  // Get the method - If the method is not one of the defined methods
  // then it doesn't return an error but sets fMethod to httpIllegalMethod
  StrPtrLen theParsedData;
  parser->ConsumeWord(&theParsedData);
  fMethod = HTTPProtocol::GetMethod(&theParsedData);

  // 还有可能是 HTTP Response 类型
  if ((fMethod == httpIllegalMethod) && theParsedData.Equal("HTTP")) {
    parser->ConsumeUntilWhitespace(); // 过滤掉HTTP/1.1
    parser->ConsumeUntilDigit(NULL);
    UInt32 statusCode = parser->ConsumeInteger(NULL);
    if (statusCode != 0) {
      fHTTPType = httpResponseType;

      parser->ConsumeWhitespace();
      parser->ConsumeUntilWhitespace(NULL);
      // Go past the end of line
      if (!parser->ExpectEOL()) {
        fStatusCode = httpBadRequest;
        return CF_BadArgument;   // Request line is not properly formatted!
      }
      return CF_NoErr;
    }
  } else if (fMethod == httpIllegalMethod) {
    if (!parser->ExpectEOL()) {
      fStatusCode = httpBadRequest;
      return CF_BadArgument;     // Request line is not properly formatted!
    }
    return CF_NoErr;
  }

  // Consume whitespace
  parser->ConsumeWhitespace();

  fHTTPType = httpRequestType;

  // Parse the URI - If it fails returns an error after setting
  // the fStatusCode to the appropriate error code
  CF_Error err = parseURI(parser);
  if (err != CF_NoErr)
    return err;

  // Consume whitespace
  parser->ConsumeWhitespace();

  // If there is a version, consume the version string
  StrPtrLen versionStr;
  parser->ConsumeUntil(&versionStr, StringParser::sEOLMask);
  // Check the version
  if (versionStr.Len > 0)
    fVersion = HTTPProtocol::GetVersion(&versionStr);

  // Go past the end of line
  if (!parser->ExpectEOL()) {
    fStatusCode = httpBadRequest;
    return CF_BadArgument;     // Request line is not properly formatted!
  }

  return CF_NoErr;
}

CF_Error HTTPPacket::parseURI(StringParser *parser) {
  // read in the complete URL into fRequestAbsURI
  parser->ConsumeUntil(&fAbsoluteURI, sURLStopConditions);

  StringParser urlParser(&fAbsoluteURI);

  // we always should have a slash before the URI
  // If not, that indicates this is a full URI
  if (fAbsoluteURI.Ptr[0] != '/') {
    // if it is a full URL, store the scheme and host name
//    urlParser.ConsumeLength(&fAbsoluteURIScheme, 7); //consume "http://"
    urlParser.ConsumeUntil(&fAbsoluteURIScheme, ':'); // consume "http"
    urlParser.ConsumeLength(nullptr, 3); // consume "://"
    urlParser.ConsumeUntil(&fHostHeader, '/');
  }

  StrPtrLen queryString;
  if (parser->GetDataRemaining() > 0) {
    if (parser->PeekFast() == '?') {
      // we've got some CGI param
      parser->ConsumeLength(&queryString, 1); // toss '?'

      // consume the rest of the line..
      parser->ConsumeUntilWhitespace(&queryString);

      if (queryString.Len) {
        if (fQueryString != NULL) {
          delete[] fQueryString;
          fQueryString = NULL;
        }

        fQueryString = new char[queryString.Len + 1];
        ::memcpy(fQueryString, queryString.Ptr, queryString.Len);
        fQueryString[queryString.Len] = '\0';
      }
    }
  }

  // whatever is in this position is the relative URI
  StrPtrLen relativeURI(urlParser.GetCurrentPosition(),
                        urlParser.GetDataReceivedLen()
                            - urlParser.GetDataParsedLen());
  // read this URI into fRequestRelURI
  fRelativeURI = relativeURI;

  // Allocate memory for fRequestPath
  UInt32 len = fRelativeURI.Len;
  len++;
  //char* relativeURIDecoded = new char[len];
  char relativeURIDecoded[512] = {0};

  SInt32 theBytesWritten =
      StringTranslator::DecodeURL(fRelativeURI.Ptr, fRelativeURI.Len,
                                  relativeURIDecoded, len);

  //if negative, an error occurred, reported as an CF_Error
  //we also need to leave room for a terminator.
  if ((theBytesWritten < 0) || (static_cast<UInt32>(theBytesWritten) == len)) {
    fStatusCode = httpBadRequest;
    return CF_BadArgument;
  }

  if (fRequestPath != NULL)
    delete[] fRequestPath;

  fRequestPath = NULL;

  fRequestPath = new char[theBytesWritten + 1];
  ::memcpy(fRequestPath, relativeURIDecoded + 1, theBytesWritten);
  //delete relativeURIDecoded;
  fRequestPath[theBytesWritten] = '\0';

  return CF_NoErr;
}

// Parses the Connection header and makes sure that request is properly terminated
CF_Error HTTPPacket::parseHeaders(StringParser *parser) {
  StrPtrLen theKeyWord;
  bool isStreamOK;

  //Repeat until we get a \r\n\r\n, which signals the end of the headers
  while ((parser->PeekFast() != '\r') && (parser->PeekFast() != '\n')) {
    //First get the header identifier

    isStreamOK = parser->GetThru(&theKeyWord, ':');
    if (!isStreamOK) {       // No colon after header!
      fStatusCode = httpBadRequest;
      return CF_BadArgument;
    }

    if (parser->PeekFast() == ' ') {        // handle space, if any
      isStreamOK = parser->Expect(' ');
      Assert(isStreamOK);
    }

    //Look up the proper header enumeration based on the header string.
    HTTPHeader theHeader = HTTPProtocol::GetHeader(&theKeyWord);

    StrPtrLen theHeaderVal;
    isStreamOK = parser->GetThruEOL(&theHeaderVal);

    if (!isStreamOK) {       // No EOL after header!
      fStatusCode = httpBadRequest;
      return CF_BadArgument;
    }

    // If this is the connection header
    if (theHeader == httpConnectionHeader) {
      // Set the keep alive boolean based on the connection header value
      setKeepAlive(&theHeaderVal);
    }

    // Have the header field and the value; Add value to the array
    // If the field is invalid (or unrecognized) just skip over gracefully
    if (theHeader != httpIllegalHeader)
      fFieldValues[theHeader] = theHeaderVal;

  }

  isStreamOK = parser->ExpectEOL();
  Assert(isStreamOK);

  return CF_NoErr;
}

void HTTPPacket::setKeepAlive(StrPtrLen *keepAliveValue) {
  if (sCloseString.EqualIgnoreCase(keepAliveValue->Ptr, keepAliveValue->Len))
    fRequestKeepAlive = sFalse;
  else {
    Assert(sKeepAliveString
               .EqualIgnoreCase(keepAliveValue->Ptr, keepAliveValue->Len));
    fRequestKeepAlive = sTrue;
  }
}

StrPtrLen *HTTPPacket::GetHeaderValue(HTTPHeader inHeader) {
  if (inHeader != httpIllegalHeader)
    return &fFieldValues[inHeader];
  return NULL;
}

void HTTPPacket::putStatusLine(StringFormatter *putStream,
                               HTTPStatusCode status,
                               HTTPVersion version) {
  putStream->Put(HTTPProtocol::GetVersionString(version));
  putStream->PutSpace();
  putStream->Put(HTTPProtocol::GetStatusCodeAsString(status));
  putStream->PutSpace();
  putStream->Put(HTTPProtocol::GetStatusCodeString(status));
  putStream->PutEOL();
}

void HTTPPacket::putMethedLine(StringFormatter *putStream,
                               HTTPMethod method,
                               HTTPVersion version) {
  putStream->Put(HTTPProtocol::GetMethodString(method));
  putStream->PutSpace();
  putStream->Put("/");
  putStream->PutSpace();
  putStream->Put(HTTPProtocol::GetVersionString(version));
  putStream->PutEOL();
}

bool HTTPPacket::CreateResponseHeader() {
  if (fHTTPType != httpResponseType) return false;

  // If we are creating a second response for the same request, make sure and
  // deallocate memory for old response and allocate fresh memory
  if (fHTTPHeaderFormatter != NULL) {
    if (fHTTPHeader->Ptr != NULL)
      delete fHTTPHeader->Ptr;
    delete fHTTPHeader;
    delete fHTTPHeaderFormatter;
  }

  // Allocate memory for the response when you first create it
  char *responseString = new char[kMinHeaderSizeInBytes];
  fHTTPHeader = new StrPtrLen(responseString, kMinHeaderSizeInBytes);
  fHTTPHeaderFormatter =
      new ResizeableStringFormatter(fHTTPHeader->Ptr, fHTTPHeader->Len);

  // make a partial header for the given version and status code
  putStatusLine(fHTTPHeaderFormatter, fStatusCode, fVersion);

  Assert(fSvrHeader.Ptr != NULL);
  AppendResponseHeader(httpServerHeader, &fSvrHeader);
  AppendDateField();

  // Access-Control-Allow-Origin: *
  AppendResponseHeader(httpAccessControlAllowOriginHeader, &sAllString);

  fHTTPHeader->Len = fHTTPHeaderFormatter->GetCurrentOffset();
  return true;
}

bool HTTPPacket::CreateRequestHeader() {
  if (fHTTPType != httpRequestType) return false;

  // If we are creating a second response for the same request, make sure and
  // deallocate memory for old response and allocate fresh memory
  if (fHTTPHeaderFormatter != NULL) {
    if (fHTTPHeader->Ptr != NULL)
      delete fHTTPHeader->Ptr;
    delete fHTTPHeader;
    delete fHTTPHeaderFormatter;
  }

  // Allocate memory for the response when you first create it
  char *responseString = new char[kMinHeaderSizeInBytes];
  fHTTPHeader = new StrPtrLen(responseString, kMinHeaderSizeInBytes);
  fHTTPHeaderFormatter =
      new ResizeableStringFormatter(fHTTPHeader->Ptr, fHTTPHeader->Len);

  //make a partial header for the given version and status code
  putMethedLine(fHTTPHeaderFormatter, fMethod, fVersion);

  Assert(fSvrHeader.Ptr != NULL);
  AppendResponseHeader(httpUserAgentHeader, &fSvrHeader);

  fHTTPHeader->Len = fHTTPHeaderFormatter->GetCurrentOffset();
  return true;
}

StrPtrLen *HTTPPacket::GetCompleteHTTPHeader() const {
  fHTTPHeaderFormatter->PutEOL();
  fHTTPHeader->Len = fHTTPHeaderFormatter->GetCurrentOffset();
  return fHTTPHeader;
}

void HTTPPacket::AppendResponseHeader(HTTPHeader inHeader,
                                      StrPtrLen *inValue) const {
  fHTTPHeaderFormatter->Put(HTTPProtocol::GetHeaderString(inHeader));
  fHTTPHeaderFormatter->Put(sColonSpace);
  fHTTPHeaderFormatter->Put(*inValue);
  fHTTPHeaderFormatter->PutEOL();
  fHTTPHeader->Len = fHTTPHeaderFormatter->GetCurrentOffset();
}

void HTTPPacket::AppendContentLengthHeader(UInt64 length_64bit) const {
  //char* contentLength = new char[256];
  char contentLength[256] = {0};
  s_sprintf(contentLength, "%" _U64BITARG_ "", length_64bit);
  StrPtrLen contentLengthPtr(contentLength);
  AppendResponseHeader(httpContentLengthHeader, &contentLengthPtr);
}

void HTTPPacket::AppendContentLengthHeader(UInt32 length_32bit) const {
  //char* contentLength = new char[256];
  char contentLength[256] = {0};
  s_sprintf(contentLength, "%"   _U32BITARG_   "", length_32bit);
  StrPtrLen contentLengthPtr(contentLength);
  AppendResponseHeader(httpContentLengthHeader, &contentLengthPtr);
}

void HTTPPacket::AppendConnectionCloseHeader() const {
  AppendResponseHeader(httpConnectionHeader, &sCloseString);
}

void HTTPPacket::AppendConnectionKeepAliveHeader() const {
  AppendResponseHeader(httpConnectionHeader, &sKeepAliveString);
}

void HTTPPacket::AppendDateAndExpiresFields() const {
  Assert(Core::Thread::GetCurrent() != NULL);
  Core::DateBuffer *theDateBuffer = Core::Thread::GetCurrent()->GetDateBuffer();
  theDateBuffer
      ->InexactUpdate(); // Update the date buffer to the current date & Time
  StrPtrLen theDate(theDateBuffer->GetDateBuffer(),
                    Core::DateBuffer::kDateBufferLen);

  // Append dates, and have this response expire immediately
  this->AppendResponseHeader(httpDateHeader, &theDate);
  this->AppendResponseHeader(httpExpiresHeader, &theDate);
}

void HTTPPacket::AppendDateField() const {
  Assert(Core::Thread::GetCurrent() != NULL);
  Core::DateBuffer *theDateBuffer = Core::Thread::GetCurrent()->GetDateBuffer();
  theDateBuffer
      ->InexactUpdate(); // Update the date buffer to the current date & Time
  StrPtrLen theDate(theDateBuffer->GetDateBuffer(),
                    Core::DateBuffer::kDateBufferLen);

  // Append date
  this->AppendResponseHeader(httpDateHeader, &theDate);
}

time_t HTTPPacket::ParseIfModSinceHeader() {
  time_t theIfModSinceDate = static_cast<time_t>(
      Core::DateTranslator::ParseDate(&fFieldValues[httpIfModifiedSinceHeader]));
  return theIfModSinceDate;
}

}
