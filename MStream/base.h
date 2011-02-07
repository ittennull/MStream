#ifndef BASE_H
#define BASE_H


typedef unsigned char byte;

void noop_printf(const char*, ...);

#ifdef _DEBUG
#define dprintf printf
#else
#define dprintf noop_printf
#endif

// Including SDKDDKVer.h defines the highest available Windows platform.
#include <SDKDDKVer.h>

#endif
