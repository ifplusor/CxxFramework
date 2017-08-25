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
#ifndef __HTTP_REQUEST_RESPONSE_H__
#define __HTTP_REQUEST_RESPONSE_H__

#include <CF.h>
#include <StringParser.h>
#include <ResizeableStringFormatter.h>
#include "HTTPProtocol.h"

class HTTPPacket {
 public:

  /**
   * @brief construct object for parse http packet header.
   * @param packetPtr - http packet data
   */
  HTTPPacket(StrPtrLen *packetPtr);

  /**
   * @brief construct object for build http packet
   * @param httpType - http packet type
   */
  HTTPPacket(HTTPType httpType = httpResponseType);

  // Destructor
  virtual ~HTTPPacket();

  HTTPType GetHTTPType() { return fHTTPType; }

  /**
   * @brief parse http request header
   * @note Should be called before accessing anything in the request header
   *
   * Calls ParseRequestLine and ParseHeaders
   */
  CF_Error Parse();

  // Basic access methods for the HTTP method, the absolute request URI,
  // the host name from URI, the relative request URI, the request file path,
  // the HTTP version, the Status code, the keep-alive tag.

  HTTPMethod GetMethod() { return fMethod; }
  HTTPVersion GetVersion() { return fVersion; }
  HTTPStatusCode GetStatusCode() { return fStatusCode; }
  bool IsRequestKeepAlive() { return fRequestKeepAlive; }

  StrPtrLen *GetRequestLine() { return &fRequestLine; }
  StrPtrLen *GetRequestAbsoluteURI() { return &fAbsoluteURI; }
  StrPtrLen *GetSchemeFromAbsoluteURI() { return &fAbsoluteURIScheme; }
  StrPtrLen *GetHostFromAbsoluteURI() { return &fHostHeader; }
  StrPtrLen *GetRequestRelativeURI() { return &fRelativeURI; }
  char *GetRequestPath() { return fRequestPath; }
  char *GetQueryString() { return fQueryString; }

  // If header field exists in the request, it will be found in the dictionary
  // and the value returned. Otherwise, NULL is returned.
  StrPtrLen *GetHeaderValue(HTTPHeader inHeader);

  // Creates a header
  bool CreateResponseHeader();
  bool CreateRequestHeader();

  void SetVersion(HTTPVersion version) { fVersion = version; }
  void SetStatusCode(HTTPStatusCode statusCode) { fStatusCode = statusCode; }

  // To append response header fields as appropriate
  void AppendResponseHeader(HTTPHeader inHeader, StrPtrLen *inValue) const;
  void AppendDateAndExpiresFields() const;
  void AppendDateField() const;
  void AppendConnectionCloseHeader() const;
  void AppendConnectionKeepAliveHeader() const;
  void AppendContentLengthHeader(UInt64 length_64bit) const;
  void AppendContentLengthHeader(UInt32 length_32bit) const;

  // Returns the completed response header by appending CRLF to the end of
  // the header fields buffer
  StrPtrLen *GetCompleteHTTPHeader() const;

  StrPtrLen *GetBody() { return fHTTPBody; }

  /**
   * @note body 的内存将交由 Packet 对象管理
   */
  void SetBody(StrPtrLen *body) {
    if (fHTTPBody != nullptr) delete[] fHTTPBody->Ptr;
    delete fHTTPBody;
    fHTTPBody = body;
  }

  //
  // Other utils

  // Parse if-modified-since header
  time_t ParseIfModSinceHeader();

 private:
  enum { kMinHeaderSizeInBytes = 512 };

  //
  // For parse

  // Gets the method, version and calls ParseURI
  CF_Error parseRequestLine(StringParser *parser);
  // Parses the URI to get absolute and relative URIs, the host name and the file path
  CF_Error parseURI(StringParser *parser);
  // Parses the headers and adds them into a dictionary
  // Also calls SetKeepAlive with the Connection header field's value if it exists
  CF_Error parseHeaders(StringParser *parser);

  // Sets fRequestKeepAlive
  void setKeepAlive(StrPtrLen *keepAliveValue);

  //
  // For construct

  // Used in initialize and CreateResponseHeader
  static void putStatusLine(StringFormatter *putStream,
                            HTTPStatusCode status,
                            HTTPVersion version);
  // Used in initialize and CreateRequestHeader
  static void putMethedLine(StringFormatter *putStream,
                            HTTPMethod method,
                            HTTPVersion version);

  // For writing into the premade headers
  StrPtrLen *getServerHeader() { return &fSvrHeader; }

  //
  // Private members

  // Complete request and response headers
  StrPtrLen fPacketHeader; // for parse
  ResizeableStringFormatter *fHTTPHeaderFormatter; // for construct
  StrPtrLen *fHTTPHeader; // for construct

  // request and repose body
  StrPtrLen *fHTTPBody;

  HTTPType fHTTPType;

  HTTPVersion fVersion;
  HTTPMethod fMethod; // request only
  HTTPStatusCode fStatusCode; // response only

  StrPtrLen fRequestLine;

  // For the URI (fAbsoluteURI and fRelativeURI are the same if the URI is of the form "/path")
  StrPtrLen fAbsoluteURI; // If it is of the form "http://foo.bar.com/path"
  StrPtrLen fRelativeURI; // If it is of the form "/path"

  // If it is an absolute URI, these fields will be filled in
  // "http://foo.bar.com/path" => fAbsoluteURIScheme = "http", fHostHeader = "foo.bar.com",
  // fRequestPath = "path"
  StrPtrLen fAbsoluteURIScheme;
  StrPtrLen fHostHeader; // If the full url is given in the request line
  char *fRequestPath; // Also contains the query string
  char *fQueryString;

  bool fRequestKeepAlive;  // Keep-alive information in the client request
  StrPtrLen fFieldValues[httpNumHeaders]; // Array of header field values parsed from the request
  StrPtrLen fSvrHeader;  // Server header set up at initialization

  static StrPtrLen sColonSpace;
  static UInt8 sURLStopConditions[];
};

#endif // __HTTP_REQUEST_RESPONSE_H__
