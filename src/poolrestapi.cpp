#include "asyncio/coroutine.h"
#include "asyncio/socket.h"
#include "p2p/p2p.h"
#include "p2putils/uriParse.h"
__NO_DEPRECATED_BEGIN
#include "config4cpp/Configuration.h"
__NO_DEPRECATED_END

#include "FoundBlocksQuery.h"




struct ServiceContext {
  std::map<std::string, p2pNode*> coinClientMap;
  std::map<std::string, QueryImpl::createProc*> apiMap;
};


struct listenerContext {
  asyncBase *base;
  aioObject *socket;
  ServiceContext *serviceCtx;
};

struct readerContext {
  asyncBase *base;
  aioObject *socket;
  ServiceContext *serviceCtx;
};

static inline void serializeString(xmstream &stream, const std::string &S)
{
  stream.write<uint32_t>(S.size());
  stream.write(S.data(), S.size());
}

static inline void deserializeString(xmstream &stream, std::string &S)
{
  size_t size = stream.read<uint32_t>();
  const char *data = stream.jumpOver<const char>(size);
  if (data)
    S.assign(data, size);
}


void uriParse(URIComponent *component, void *arg)
{
  QueryContext *ctx = (QueryContext*)arg;
  switch (component->type) {
    case uriCtPathElement : {
      std::string S(component->raw.data, component->raw.size);
      if (ctx->pathElIdx == 0) {
        if (S != "api")
          ctx->errorCode = 404;
      } else if (ctx->pathElIdx == 1) {
        // search client by coin
        ctx->node = ctx->serviceCtx->coinClientMap[S];
      } else {
        ctx->apiId.push_back('/');
        ctx->apiId.append(S);
      }
      
      ctx->pathElIdx++;
      break;
    }
    
    case uriCtPath : {
      // search api by id
      QueryImpl::createProc *createProc = ctx->serviceCtx->apiMap[ctx->apiId];
      if (createProc) {
        ctx->api = createProc();
      } else {
        ctx->api = 0;
        ctx->errorCode = 404;
      }
      break;
    }
    
    case uriCtQueryElement : {
      if (ctx->api) {
        RawData name = {(uint8_t*)component->raw.data, component->raw.size};
        RawData value = {(uint8_t*)component->raw2.data, component->raw2.size};
        ctx->api->setArg(name, value);
      }
      break;
    }
    
    default :
      break;
  }
}

void findHandlerByUrl(QueryContext *query, ServiceContext *serviceCtx, const std::string &method, const std::string &url)
{
  // TODO: optimize tihs

}

void mainProc(void *arg)
{
  readerContext *reader = (readerContext*)arg;
  uint64_t msgSize;
  uint8_t msgBuffer[16384];

  xmstream writeStream;
  while (true) {
    if (ioRead(reader->socket, &msgSize, 8, afWaitAll, 0) < 0) {
      fprintf(stderr, "<error> can't read message size, exiting..\n");
      break;
    }
    
    msgSize = xhton(msgSize);
    if (msgSize > sizeof(msgBuffer)) {
      fprintf(stderr, "<error> too big message\n");
      break;
    }
    
    if (ioRead(reader->socket, msgBuffer, msgSize, afWaitAll, 0) < 0) {
      fprintf(stderr, "<error> can't read message data, exiting..\n");
      break;
    }
    
    QueryContext query(reader->serviceCtx);  

    {
      std::string method;
      std::string url;
      std::string headerName;
      std::string headerValue;
      std::string body;
      
      // read method
      xmstream stream(msgBuffer, msgSize);
      deserializeString(stream, method);
      deserializeString(stream, url);
      std::string fullUrl = "http://host" + url;      // TODO: optimize uri parser
    
      query.apiId = method;    
      uriParse(fullUrl.c_str(), uriParse, &query);
      if (query.api && query.node) {
        // update headers
        unsigned headersNum = stream.read<uint32_t>();
        for (unsigned i = 0; i < headersNum; i++) {
          deserializeString(stream, headerName);
          deserializeString(stream, headerValue);
          if (stream.eof()) {
            fprintf(stderr, "<error> header decode failed\n");
            break;
          }
          
          RawData rawName = {(uint8_t*)headerName.data(), headerName.size()};
          RawData rawValue = {(uint8_t*)headerValue.data(), headerValue.size()};
          query.api->setHeader(rawName, rawValue);
        }   
        
        {
          deserializeString(stream, body);
          if (stream.eof()) {
            fprintf(stderr, "<error> body failed\n");
            break;
          }
        
          RawData rawBody = {(uint8_t*)body.data(), body.size()};
          query.api->setBody(rawBody);
        }
        
        query.api->run(query);
      }
    }
    
    writeStream.reset();
    writeStream.write<uint32_t>(query.errorCode);
    writeStream.write<uint32_t>(1);
    {
      serializeString(writeStream, "Content-Type");
      serializeString(writeStream, "application/json");
    }
    
    serializeString(writeStream, query.result);
    aioWrite(reader->socket, writeStream.data(), writeStream.sizeOf(), afNone, 0, 0, 0);
  }

  deleteAioObject(reader->socket);
}

void listenerProc(void *arg)
{
  listenerContext *ctx = (listenerContext*)arg;
  while (true) {
    HostAddress address;
    socketTy acceptSocket = ioAccept(ctx->socket, 0);
    if (acceptSocket != INVALID_SOCKET) {
      readerContext *reader = new readerContext;
      reader->base = ctx->base;
      reader->socket = newSocketIo(ctx->base, acceptSocket);      
      reader->serviceCtx = ctx->serviceCtx;
      coroutineTy *echoProc = coroutineNew(mainProc, reader, 0x10000);
      coroutineCall(echoProc);      
    }
  }
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <configuration file>\n", argv[0]);
    return 1;
  }

  initializeSocketSubsystem();
  asyncBase *base = createAsyncBase(amOSDefault);
  
  // Service context initialization
  HostAddress localAddress;
  ServiceContext serviceCtx;  
  
  config4cpp::Configuration *cfg = config4cpp::Configuration::create();
  try {
    cfg->parse(argv[1]);
    
    config4cpp::StringVector coins;
    cfg->lookupList("poolrestapi", "coins", coins);
    for (decltype(coins.length()) coinIdx = 0; coinIdx < coins.length(); coinIdx++) {
      
      std::vector<HostAddress> addresses;
      config4cpp::StringVector coinPeers;
      cfg->lookupList(coins[coinIdx], "frontends", coinPeers);
      for (decltype(coinPeers.length()) peerIdx = 0; peerIdx < coinPeers.length(); peerIdx++) {
        URI uri;
        if (!uriParse(coinPeers[peerIdx], &uri)) {
          fprintf(stderr, "<error> can't read coin %s peers from configuration file\n", coins[coinIdx]);
          return 1;
        }
      
        if (uri.schema != "p2p" || !uri.ipv4 || !uri.port) {
          fprintf(stderr, "<error> %s coin peers can be contain only p2p://xxx.xxx.xxx.xxx:port address now\n", coins[coinIdx]);
          return 1;
        }
        
        HostAddress address;
        address.family = AF_INET;
        address.ipv4 = uri.ipv4;
        address.port = xhton<uint16_t>(uri.port);
        addresses.push_back(address);
      }
      
      const char *poolAppName = cfg->lookupString(coins[coinIdx], "poolAppName");
      serviceCtx.coinClientMap[coins[coinIdx]] = p2pNode::createClient(base, &addresses[0], addresses.size(), poolAppName);
      
      {
        URI uri;
        const char *listenAddress = cfg->lookupString("poolrestapi", "listenAddress");
        if (!uriParse(listenAddress, &uri)) {
          fprintf(stderr, "<error> can't read listenAddress from configuration file\n");
          return 1;
        }
      
        if (uri.schema != "cxxrestapi" || !uri.ipv4 || !uri.port) {
          fprintf(stderr, "<error> listenAddress can be contain only cxxrestapi://xxx.xxx.xxx.xxx:port address now\n");
          return 1;
        }
        
        localAddress.family = AF_INET;
        localAddress.ipv4 = uri.ipv4;
        localAddress.port = xhton<uint16_t>(uri.port);
      }
    }
  } catch(const config4cpp::ConfigurationException& ex){
    fprintf(stderr, "<error> %s\n", ex.c_str());
    exit(1);
  }
  
  socketTy hSocket = socketCreate(AF_INET, SOCK_STREAM, IPPROTO_TCP, 1);
  socketReuseAddr(hSocket);
  if (socketBind(hSocket, &localAddress) != 0) {
    fprintf(stderr, "<error> cannot bind\n");
    exit(1);
  }

  if (socketListen(hSocket) != 0) {
    fprintf(stderr, "<error> listen error\n");
    exit(1);
  }

  // api map
  serviceCtx.apiMap["GET/foundBlocks"] = FoundBlocksQuery::create;
  serviceCtx.apiMap["GET/payouts"] = PayoutsQuery::create;
  serviceCtx.apiMap["GET/clientStats"] = ClientStatsQuery::create;
  serviceCtx.apiMap["GET/poolStats"] = PoolStatsQuery::create;
  serviceCtx.apiMap["GET/clientInfo"] = ClientInfoQuery::create;
  serviceCtx.apiMap["POST/settings"] = SettingsQuery::create;
  serviceCtx.apiMap["POST/payout"] = ManualPayoutQuery::create;
  
  aioObject *listenerSocket = newSocketIo(base, hSocket);
  
  listenerContext ctx;
  ctx.base = base;
  ctx.socket = listenerSocket;
  ctx.serviceCtx = &serviceCtx;

  coroutineCall(coroutineNew(listenerProc, &ctx, 0x10000));
  asyncLoop(base);
  return 0;
}
