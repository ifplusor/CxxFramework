//
// Created by james on 8/26/17.
//

#ifndef __HTTP_DISPATCHER_H__
#define __HTTP_DISPATCHER_H__

#include <CF/Net/Http/HTTPDef.h>
#include <CF/Net/Http/HTTPPacket.h>

namespace CF::Net {

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
  // 四种类型的映射器
  enum {
    httpExactMapper = 0,
    httpWildcardMapper = 1,
    httpExtensionMapper = 2,
    httpDefaultMapper = 4
  };

  static HTTPPathMapper *BuildPathMatcher(HTTPMapping &mapping);

  static int ComparePriority(HTTPPathMapper *aMapper, HTTPPathMapper *bMapper);

  virtual ~HTTPPathMapper() = default;;

  virtual UInt32 ItsType() = 0;

  virtual bool Match(const StrPtrLen &path) = 0;

  CF_Error Mapping(HTTPPacket &request, HTTPPacket &response) {
    return fFunc(request, response);
  }

 protected:
  HTTPPathMapper(HTTPMapping mapping) : fFunc(mapping.func) {}

  CF_CGIFunction fFunc;

  static StrPtrLen sAllSuffix;
  static StrPtrLen sTypePrefix;
  static StrPtrLen sRootPath;
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

  UInt32 ItsType() override { return httpExactMapper; }

  bool Match(const StrPtrLen &path) override { return pattern.Equal(path); }

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

  UInt32 ItsType() override { return httpWildcardMapper; }

  bool Match(const StrPtrLen &path) override {
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

  UInt32 ItsType() override { return httpExtensionMapper; }

  bool Match(const StrPtrLen &path) override {
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

  UInt32 ItsType() override { return httpDefaultMapper; }

  bool Match(const StrPtrLen &path) override { return true; }
};

}

#endif // __HTTP_DISPATCHER_H__
