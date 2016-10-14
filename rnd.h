/*
Copyright (c) 2016, Jim Fougeron
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
*/

#ifndef _RND__H
#define _RND__H

#include "openssl/sha.h"

// This is a crypto secure random method of rolling a die.  We build up entropy (lots of it)
// in our entropy stucture. Then we take all that entropy, and pack it down into 1 bit. We get
// 16 of these bits for EACH dice roll.

#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <Winioctl.h>   //DISK_GEOMETRY
#include <Wmistr.h>     //WNODE_HEADER
#include <winternl.h>

typedef struct entropy_t {
	unsigned char sha256_h[32];							// SHA256 of prior entropy structure (kinda CBC mode)
	int counter;										// simple incremental counter
	ULONG li1;											// GetTickCount()
	LARGE_INTEGER li2;									// QueryPerformanceCounter
	long long li3;										// QueryUnbiasedInterruptTime 
	SYSTEM_PERFORMANCE_INFORMATION perf;				// call to NtQuerySystemInformation(SystemPerformanceInformation)
	SYSTEM_PERFORMANCE_INFORMATION perf2;				// call to NtQuerySystemInformation(SystemPerformanceInformation)
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION pperf;		// call to NtQuerySystemInformation(SystemProcessorPerformanceInformation)
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION pperf2;	// call to NtQuerySystemInformation(SystemProcessorPerformanceInformation)
	POINT pt;											// GetCursorPos()
	int init;
} entropy;

extern unsigned char *entropy_slurp(entropy *E);
extern unsigned entropy_get_bit(entropy *pE);
extern unsigned entropy_get_die(entropy *pE);

#endif // _RND__H
