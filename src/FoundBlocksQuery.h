#include "Query.h"
#include "poolcommon/poolapi.h"
#include <string>

class FoundBlocksQuery : public QueryImpl {
private:
  int _heightFrom;
  std::string _hashFrom;
  unsigned _count;
 
  FoundBlocksQuery() : _heightFrom(-1), _count(20) {}
  
public:
  static QueryImpl *create() { return new FoundBlocksQuery; }
  void setArg(RawData name, RawData value);
  void setHeader(RawData name, RawData value) {}
  void setBody(RawData body) {}
  void run(QueryContext &context);
};

class ClientInfoQuery : public QueryImpl {
private:
  std::string _userId;
  ClientInfoQuery() {}
  
public:
  static QueryImpl *create() { return new ClientInfoQuery; }
  void setArg(RawData name, RawData value);
  void setHeader(RawData name, RawData value) {}
  void setBody(RawData body) {}
  void run(QueryContext &context);
};

class PayoutsQuery : public QueryImpl {
private:
  std::string _userId;
  GroupByType _groupBy;
  time_t _timeFrom;
  unsigned _count;
  
  PayoutsQuery() : _groupBy(GroupByType_None), _timeFrom(-1), _count(20) {}
  
public:
  static QueryImpl *create() { return new PayoutsQuery; }
  void setArg(RawData name, RawData value);
  void setHeader(RawData name, RawData value) {}
  void setBody(RawData body) {}
  void run(QueryContext &context);
};

class ClientStatsQuery : public QueryImpl {
private:
  std::string _userId;
  ClientStatsQuery() {}
  
public:
  static QueryImpl *create() { return new ClientStatsQuery; }
  void setArg(RawData name, RawData value);
  void setHeader(RawData name, RawData value) {}
  void setBody(RawData body) {}
  void run(QueryContext &context);
};

class PoolStatsQuery : public QueryImpl {
private:
  PoolStatsQuery() {}
  
public:
  static QueryImpl *create() { return new PoolStatsQuery; }
  void setArg(RawData name, RawData value) {}
  void setHeader(RawData name, RawData value) {}
  void setBody(RawData body) {}
  void run(QueryContext &context);
};



class SettingsQuery : public QueryImpl {
private:
  std::string _userId;
  int64_t _threshold;
  SettingsQuery() : _threshold(0) {}
  
public:
  static QueryImpl *create() { return new SettingsQuery; }
  void setArg(RawData name, RawData value);
  void setHeader(RawData name, RawData value) {}
  void setBody(RawData body);
  void run(QueryContext &context);
};

class ManualPayoutQuery : public QueryImpl {
private:
  std::string _userId;
  ManualPayoutQuery() {}
  
public:
  static QueryImpl *create() { return new ManualPayoutQuery; }
  void setArg(RawData name, RawData value);
  void setHeader(RawData name, RawData value) {}
  void setBody(RawData body) {}
  void run(QueryContext &context);
};
