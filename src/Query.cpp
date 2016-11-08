#include "Query.h"
#include <string.h>

int rawCmp(RawData data, const char *S)
{
  size_t length = strlen(S);
  if (data.size == length)
    return memcmp(data.data, S, length);
  return data.size < length ? -1 : 1;
}
