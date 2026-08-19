#ifndef NS_CONNECTION_H
#define NS_CONNECTION_H

#include "core/ns_stdtypes.h"

struct connection
{
  u8 buffer[4096];

  u32 rlen;
  u32 wlen;
};

#endif
