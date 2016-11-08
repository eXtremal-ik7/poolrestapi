#include "p2putils/coreTypes.h"
#include <string>

class p2pNode;
class ServiceContext;
class QueryImpl;

int rawCmp(RawData data, const char *S);

template<typename Type> Type xatoi(RawData data)
{
  int minus = 0;  
  Type lvalue = 0;
  const char *p = (const char*)data.data;
  const char *e = p + data.size;
  
  if (p < e) {
    if (*p == '-') {
      minus = 1;
      p++;
    } else if (*p == '+') {
      p++;
    }  
  }
  
  while (p < e)
    lvalue = (lvalue*10) + (*p++ - '0');
  
  return minus ? -lvalue : lvalue;
}

struct QueryContext {
  ServiceContext *serviceCtx;
  p2pNode *node;
  QueryImpl *api;
  
  
  std::string apiId;
  int pathElIdx;
  
  int errorCode;
  std::string result;
  
  QueryContext(ServiceContext *serviceCtxArg) : serviceCtx(serviceCtxArg), node(0), pathElIdx(0), errorCode(200) {}
};

class QueryImpl {
public:
  typedef QueryImpl* QueryImplPtr;
  typedef QueryImplPtr createProc();
  
  std::string result;
public:
  virtual void setArg(RawData name, RawData value) = 0;
  virtual void setHeader(RawData name, RawData value) = 0;
  virtual void setBody(RawData body) = 0;
  virtual void run(QueryContext &context) = 0;
  virtual ~QueryImpl() {}
};
