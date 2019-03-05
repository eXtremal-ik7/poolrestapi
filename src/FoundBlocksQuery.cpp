#include "FoundBlocksQuery.h"
#include "p2putils/strExtras.h"
#include <jansson.h>
#include "loguru.hpp"

static const char *unitType(UnitType type)
{
  static const char *types[] = {
    "CPU",
    "GPU",
    "ASIC",
    "OTHER"
  };
  
  return types[std::min(type, UnitType_OTHER)];
}

static bool decodeAsReal(json_t *object, double *r)
{
  if (object) {
    if (json_is_integer(object)) {
      *r = json_integer_value(object);
      return true;
    } else if (json_is_real(object)) {
      *r = json_real_value(object);
      return true;
    }
  }
  
  return false;
}

template<typename T>
static inline void appendInt(const char *fieldName, T value, std::string &out, bool comma) {
  char N[32]; 
  xitoa<T>(value, N);
  out.push_back('\"');
  out.append(fieldName);
  out.append("\":");
  out.append(N);
  if (comma)
    out.push_back(',');
}

static void appendString(const char *fieldName, const char *value, std::string &out, bool comma) {
  out.push_back('\"');
  out.append(fieldName);
  out.append("\":\"");
  out.append(value);
  if (comma)
    out.append("\",");
  else
    out.push_back('\"');
}

void FoundBlocksQuery::setArg(RawData name, RawData value)
{
  if (rawCmp(name, "heightFrom") == 0) {
    _heightFrom = xatoi<decltype(_heightFrom)>(value);
  } else if (rawCmp(name, "hashFrom") == 0) {
    _hashFrom.assign((const char*)value.data, value.size);
  } else if (rawCmp(name, "count") == 0) {
    _count = xatoi<unsigned>(value);
  } else {
    std::string _name((const char*)name.data, name.size);
    LOG_F(WARNING, "foundBlocksQuery: unknown argument %s", _name.c_str());
  }
}

void FoundBlocksQuery::run(QueryContext &context)
{
  auto result = ioQueryFoundBlocks(context.node, _heightFrom, _hashFrom, _count);
  if (!result) {
    context.errorCode = 500;
    return;
  }
  
  char N[16];
  std::string &R = context.result;  
  R = "[";
  for (size_t i = 0; i < result->blocks.size(); i++) {
    auto &block = result->blocks[i];
    if (i > 0)
      R.push_back(',');       
    R.push_back('{');
    xitoa<unsigned>(block->height, N); R.append("\"height\":"); R.append(N); R.push_back(',');
    R.append("\"hash\":\""); R.append(block->hash.c_str()); R.append("\",");
    xitoa<unsigned>(block->time, N); R.append("\"time\":"); R.append(N); R.push_back(',');
    xitoa<int>(block->confirmations, N); R.append("\"confirmations\":"); R.append(N); R.push_back(',');
    xitoa<int64_t>(block->generatedCoins, N); R.append("\"generatedCoins\":"); R.append(N); R.push_back(',');
    R.append("\"foundBy\":\""); R.append(block->foundBy.c_str()); R.append("\"");
    R.push_back('}'); 
  }
  R.push_back(']');
}

void ClientInfoQuery::setArg(RawData name, RawData value)
{
  if (rawCmp(name, "userId") == 0) {
    _userId.assign((const char*)value.data, value.size);
  } else {
    std::string _name((const char*)name.data, name.size);    
    LOG_F(WARNING, "<warning> payoutsQuery: unknown argument %s", _name.c_str());
  }
}

void ClientInfoQuery::run(QueryContext &context)
{
  auto result = ioQueryClientInfo(context.node, _userId);
  if (!result) {
    context.errorCode = 500;
    return;
  }
  
  std::string &R = context.result; 
  R.push_back('{');
  auto &info = result->info;
  appendInt("balance", info->balance, R, true);
  appendInt("requested", info->requested, R, true);
  appendInt("paid", info->paid, R, true);
  appendString("name", !info->name.empty() ? info->name.c_str() : "", R, true);
  appendString("email", !info->email.empty() ? info->email.c_str() : "", R, true);
  appendInt("minimalPayout", info->minimalPayout, R, false);
  R.push_back('}');
}


void PayoutsQuery::setArg(RawData name, RawData value)
{
  if (rawCmp(name, "userId") == 0) {
    _userId.assign((const char*)value.data, value.size);
  } else if (rawCmp(name, "groupBy") == 0) {
    if (rawCmp(value, "hour") == 0)
      _groupBy = GroupByType_Hour;
    else if (rawCmp(value, "day") == 0)
      _groupBy = GroupByType_Day;
    else if (rawCmp(value, "week") == 0)
      _groupBy = GroupByType_Week;
    else if (rawCmp(value, "month") == 0)
      _groupBy = GroupByType_Month;
  } else if (rawCmp(name, "timeFrom") == 0) {
    _timeFrom = xatoi<time_t>(value);
  } else if (rawCmp(name, "count") == 0) {
    _count = xatoi<unsigned>(value);
  } else {
    std::string _name((const char*)name.data, name.size);    
    fprintf(stderr, "<warning> payoutsQuery: unknown argument %s\n", _name.c_str());
  }
}

void PayoutsQuery::run(QueryContext &context)
{
  auto result = ioQueryPayouts(context.node, _userId, _groupBy, _timeFrom, _count);
  if (!result) {
    context.errorCode = 500;
    return;
  }
  
  std::string &R = context.result;  
  R = "[";
  auto recordsNum = result->payouts.size();
  for (decltype(recordsNum) i = 0; i < recordsNum; i++) {
    auto &record = result->payouts[i];
    if (i > 0)
      R.push_back(',');     
    R.push_back('{');
    appendInt("time", record->time, R, true);
    appendString("timeLabel", record->timeLabel.c_str(), R, true);
    if (!record->txid.empty())
      appendString("txid", record->txid.c_str(), R, true);
    appendInt("value", record->value, R, false);
    R.push_back('}');    
  }
  R.push_back(']');  
}


void ClientStatsQuery::setArg(RawData name, RawData value)
{
  if (rawCmp(name, "userId") == 0) {
    _userId.assign((const char*)value.data, value.size);
  } else {
    std::string _name((const char*)name.data, name.size);    
    fprintf(stderr, "<warning> payoutsQuery: unknown argument %s\n", _name.c_str());
  }
}

void ClientStatsQuery::run(QueryContext &context)
{
  auto result = ioQueryClientStats(context.node, _userId);
  if (!result) {
    context.errorCode = 500;
    return;
  }
  
  std::string &R = context.result;  
  R.append("{\"workers\":[");
  auto workersNum = result->workers.size();
  for (decltype(workersNum) i = 0; i < workersNum; i++) {
    auto &worker = result->workers[i];
    if (i > 0)
      R.push_back(',');      
    R.push_back('{');
    appendString("name", worker->name.c_str(), R, true);    
    appendInt("power", worker->power, R, true);
    appendInt("latency", worker->latency, R, true);
    appendString("type", unitType(worker->type), R, true);
    appendInt("units", worker->units, R, true);
    appendInt("temp", worker->temp, R, false);
    R.push_back('}');
  }
  R.append("],\"total\":{");
  {
    auto &agg = result->aggregate;
    appendInt("clients", agg->clients, R, true);
    appendInt("workers", agg->workers, R, true);
    appendInt("cpus", agg->cpus, R, true);
    appendInt("gpus", agg->gpus, R, true);
    appendInt("asics", agg->asics, R, true);
    appendInt("other", agg->other, R, true);
    appendInt("averageLatency", agg->avgerageLatency, R, true);
    appendInt("power", agg->power, R, false);
  }
  R.append("}}");
}

void PoolStatsQuery::run(QueryContext &context)
{
  auto result = ioQueryPoolStats(context.node);
  if (!result) {
    context.errorCode = 500;
    return;
  }
  
  std::string &R = context.result;  
  R.push_back('{');
  auto &agg = result->aggregate;
  appendInt("clients", agg->clients, R, true);
  appendInt("workers", agg->workers, R, true);
  appendInt("cpus", agg->cpus, R, true);
  appendInt("gpus", agg->gpus, R, true);
  appendInt("asics", agg->asics, R, true);
  appendInt("other", agg->other, R, true);
  appendInt("averageLatency", agg->avgerageLatency, R, true);
  appendInt("power", agg->power, R, false);  
  R.push_back('}');
}

void SettingsQuery::setArg(RawData name, RawData value)
{
  if (rawCmp(name, "userId") == 0) {
    _userId.assign((const char*)value.data, value.size);
  } else if (rawCmp(name, "minimalPayout") == 0) {
    _threshold = xatoi<int64_t>(value);
  } else {
    std::string _name((const char*)name.data, name.size);    
    fprintf(stderr, "<warning> payoutsQuery: unknown argument %s\n", _name.c_str());
  }
}

void SettingsQuery::setBody(RawData body)
{
  std::string _body((const char*)body.data, body.size);
  json_error_t jsonError;
  json_t *r = json_loads(_body.c_str(), 0, &jsonError);
  if (!r)
    return;
  
  double value = 0.0;
  if (decodeAsReal(json_object_get(r, "minPayout"), &value)) {
    json_delete(r);
    _threshold = value * 100000000;
  } else {
    json_delete(r);
  }
}


void SettingsQuery::run(QueryContext& context)
{
  if (_userId.empty() || _threshold == 0)
    return;
  
  auto result = ioUpdateClientInfo(context.node, _userId, "", "", _threshold);
  if (!result) {
    context.errorCode = 500;
    return;
  }
  
  std::string &R = context.result;  
  R = "{}";
}

void ManualPayoutQuery::setArg(RawData name, RawData value)
{
  if (rawCmp(name, "userId") == 0) {
    _userId.assign((const char*)value.data, value.size);
  } else {
    std::string _name((const char*)name.data, name.size);    
    fprintf(stderr, "<warning> payoutsQuery: unknown argument %s\n", _name.c_str());
  }
}

void ManualPayoutQuery::run(QueryContext& context)
{
  if (_userId.empty())
    return;
  
  auto result = ioManualPayout(context.node, _userId);
  if (!result) {
    context.errorCode = 500;
    return;
  }
  
  std::string &R = context.result;  
  R = "{}";
}
