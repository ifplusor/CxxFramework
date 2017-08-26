//
// Created by james on 8/26/17.
//

#ifndef __HTTP_DISPATCHER_H__
#define __HTTP_DISPATCHER_H__

#include "HTTPDef.h"
#include "HTTPPacket.h"

class HTTPPathMapper;

class HTTPDispatcher {
 public:

  HTTPDispatcher(HTTPMapping *mapping);

  virtual ~HTTPDispatcher() { delete[] fMappers; }

  CF_Error Dispatch(HTTPPacket &request, HTTPPacket &response);

 private:
  HTTPPathMapper **fMappers;
  UInt32 fMapperNum;
};

class HTTPPathMapper {
 public:
  static HTTPPathMapper *BuildPathMatcher(HTTPMapping &mapping);

  virtual ~HTTPPathMapper() = default;;

  enum {
    httpExactMapper = 0,
    httpWildcardMapper = 1,
    httpExtensionMapper = 2,
    httpDefaultMapper = 4
  };
  virtual UInt32 Type() = 0;

  virtual bool Match(StrPtrLen &path) = 0;

  CF_Error Mapping(HTTPPacket &request, HTTPPacket &response) {
    return fFunc(request, response);
  }

 protected:
  HTTPPathMapper(HTTPMapping mapping) : fFunc(mapping.func) {}

  CF_CGIFunction fFunc;
};

class ExactPathMapper : public HTTPPathMapper {
 public:
  ExactPathMapper(HTTPMapping mapping) : HTTPPathMapper(mapping) {
    auto len = static_cast<UInt32>(strlen(mapping.path));
    char *buffer = new char[len + 1];
    memcpy(buffer, mapping.path, len);
    buffer[len] = '\0';
    pattern.Set(buffer, len);
  }
  ~ExactPathMapper() override { delete[] pattern.Ptr; }

  UInt32 Type() override { return httpExactMapper; }

  bool Match(StrPtrLen &path) override { return pattern.Equal(path); }

 private:
  StrPtrLen pattern;
};

class WildcardPathMapper : public HTTPPathMapper {
 public:
  WildcardPathMapper(HTTPMapping mapping) : HTTPPathMapper(mapping) {
    // xxxxx/*
    auto len = static_cast<UInt32>(strlen(mapping.path) - 2);
    char *buffer = new char[len + 1];
    memcpy(buffer, mapping.path, len);
    buffer[len] = '\0';
    pattern.Set(buffer, len);
  }
  ~WildcardPathMapper() override { delete[] pattern.Ptr; }

  UInt32 Type() override { return httpWildcardMapper; }

  bool Match(StrPtrLen &path) override {
    if (path.Len < pattern.Len) return false;
    StrPtrLen op(path.Ptr, pattern.Len);
    return pattern.Equal(op);
  }
 private:
  StrPtrLen pattern;
};

class ExtensionPathMapper : public HTTPPathMapper {
 public:
  ExtensionPathMapper(HTTPMapping mapping) : HTTPPathMapper(mapping) {
    // *.ext
    auto len = static_cast<UInt32>(strlen(mapping.path) - 1);
    char *buffer = new char[len + 1];
    memcpy(buffer, mapping.path + 1, len);
    buffer[len] = '\0';
    pattern.Set(buffer, len);
  }
  ~ExtensionPathMapper() override { delete[] pattern.Ptr; }

  UInt32 Type() override { return httpExtensionMapper; }

  bool Match(StrPtrLen &path) override {
    if (path.Len < pattern.Len) return false;
    StrPtrLen op(path.Ptr + (path.Len - pattern.Len), pattern.Len);
    return pattern.Equal(op);
  }
 private:
  StrPtrLen pattern;
};

class DefaultPathMapper : public HTTPPathMapper {
 public:
  DefaultPathMapper(HTTPMapping mapping) : HTTPPathMapper(mapping) {}
  ~DefaultPathMapper() override = default;

  UInt32 Type() override { return httpDefaultMapper; }

  bool Match(StrPtrLen &path) override { return true; }
};

#endif // __HTTP_DISPATCHER_H__
