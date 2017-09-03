//
// Created by james on 8/26/17.
//

#include <CF/Net/Http/HTTPDispatcher.h>

namespace CF::Net {

StrPtrLen HTTPPathMapper::sAllSuffix("/*", 2);
StrPtrLen HTTPPathMapper::sTypePrefix("*.", 2);
StrPtrLen HTTPPathMapper::sRootPath("/", 1);

int cmp_mapper(const void *a, const void *b) {
  return HTTPPathMapper::ComparePriority(*(HTTPPathMapper **) a,
                                         *(HTTPPathMapper **) b);
}

HTTPDispatcher::HTTPDispatcher(HTTPMapping *mapping) {
  int count;
  for (count = 0; true; count++) {
    if (mapping[count].path == nullptr || mapping[count].func == nullptr) break;
  }

  fMappers = new HTTPPathMapper *[count];
  Assert(fMappers != nullptr);

  UInt32 index = 0;
  for (int i = 0; i < count; i++) {
    HTTPPathMapper *mapper = HTTPPathMapper::BuildPathMatcher(mapping[i]);
    if (mapper != nullptr)
      fMappers[index++] = mapper;
    else
      s_printf("error: construct path matcher failed, path: %s",
               mapping->path);
  }
  fMapperNum = index;

  // sort
  qsort(fMappers, fMapperNum, sizeof(HTTPPathMapper *), cmp_mapper);
}

CF_Error HTTPDispatcher::Dispatch(HTTPPacket &request, HTTPPacket &response) {

  StrPtrLen *requestPath = request.GetRequestRelativeURI();

  for (UInt32 i = 0; i < fMapperNum; i++) {
    if (fMappers[i]->Match(*requestPath))
      return fMappers[i]->Mapping(request, response);
  }

  return CF_FileNotFound;
}

HTTPPathMapper *HTTPPathMapper::BuildPathMatcher(HTTPMapping &mapping) {
  size_t len = strlen(mapping.path);

  if (len >= 2) {
    StrPtrLen lastTwoLetter(mapping.path + (len - 2), 2);
    if (sAllSuffix.Equal(lastTwoLetter)) { // 以 /* 结尾，wildcard
      return new WildcardPathMapper(mapping);
    }

    StrPtrLen firstTwoLetter(mapping.path, 2);
    if (sTypePrefix.Equal(firstTwoLetter)) { // 以 *. 开头，extension
      return new ExtensionPathMapper(mapping);
    }
  }

  if (sRootPath.Equal(mapping.path)) { // 只有 /，default
    return new DefaultPathMapper(mapping);
  }

  // 其它，exact
  return new ExactPathMapper(mapping);
}

int HTTPPathMapper::ComparePriority(HTTPPathMapper *aMapper,
                                    HTTPPathMapper *bMapper) {
  return aMapper->ItsType() - bMapper->ItsType();
}

}
