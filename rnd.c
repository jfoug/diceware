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

#include "rnd.h"
#include <stdio.h>
#include <stdlib.h>

typedef NTSTATUS (__stdcall *NtQuerySysInfo_fp)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NtQuerySysInfo_fp NtQuerySystemInfo;

static void entropy_init(entropy *E) {
	SHA256_CTX ctx;
	int j;

	memset(E, 0, sizeof(E));
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, E, sizeof(E));
	SHA256_Final(E->sha256_h, &ctx);
	if (!NtQuerySystemInfo) {
		HMODULE h = LoadLibrary("ntdll.dll");
		NtQuerySystemInfo = (NtQuerySysInfo_fp)GetProcAddress(h, "NtQuerySystemInformation");
	}
	E->init = 1;
	for (int j = 0; j < 10; ++j)
		entropy_slurp(E);
	for (j = 1; j < 32; ++j) {
		E->init |= (entropy_get_bit(E) << j);
		entropy_slurp(E);
	}
	E->init |= entropy_get_bit(E);
	for (j = 1; j < 10; ++j)
		entropy_slurp(E);
}

unsigned char *entropy_slurp(entropy *E) {
	ULONG sil;
	SHA256_CTX ctx;
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION asi[64]; // handle system with up to 64 CPU's
	if (E->init == 0)
		entropy_init(E);
	// grab system 'current' metrics.
	NtQuerySystemInfo(SystemPerformanceInformation, &E->perf, sizeof(E->perf), &sil);
	E->li1 = GetTickCount();
	GetCursorPos(&E->pt);
	QueryPerformanceCounter(&E->li2);
	QueryUnbiasedInterruptTime (&E->li3);
	NtQuerySystemInfo(SystemProcessorPerformanceInformation, &asi, sizeof(asi), &sil);
	memcpy(&E->pperf, asi, sizeof(E->pperf));
	// pause system an arbitrary amount of time
	Sleep(2);
	// compress/update into the sha256 buffer twice (includes all prior sha256 calls)
	SHA256_Init(&ctx); SHA256_Update(&ctx, &E, sizeof(entropy)); SHA256_Final(E->sha256_h, &ctx);
	SHA256_Init(&ctx); SHA256_Update(&ctx, &E, sizeof(entropy)); SHA256_Final(E->sha256_h, &ctx);
	// update system metrics again (proc performance first this time
	NtQuerySystemInfo(SystemProcessorPerformanceInformation, &asi, sizeof(asi), &sil);
	memcpy(&E->pperf2, asi, sizeof(E->pperf2));
	QueryPerformanceCounter(&E->li2);
	QueryUnbiasedInterruptTime (&E->li3);
	NtQuerySystemInfo(SystemPerformanceInformation, &E->perf2, sizeof(E->perf2), &sil);
	E->li1 = GetTickCount();
	GetCursorPos(&E->pt);
	// again, pause arbitrary amount of time (so that an instant call into entropy_slurp
	// will not happen without 'sleep'
	Sleep(2);
	// compress/update into the sha256 buffer.  That buffer IS our return of 32 bytes.
	SHA256_Init(&ctx); SHA256_Update(&ctx, &E, sizeof(entropy)); SHA256_Final(E->sha256_h, &ctx); 
	return E->sha256_h;
}

static unsigned char oneBits[] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};

// This function gets only 1 bit for the entire entropy hash value.
// it does this by returning 0 for even parity, 1 for odd parity
unsigned entropy_get_bit(entropy *pE) {
	unsigned i, parity = 0;
	for (i = 0; i < 32; ++i) {
		unsigned b = oneBits[pE->sha256_h[i]&0xF];
		b += oneBits[pE->sha256_h[i]>>4];
		parity += b;
	}
	return parity & 1;
}
unsigned entropy_get_die(entropy *pE) {
	// we get 16 bits of entropy. Each call to entropy_slurp, we take only
	// 1 bit (parity of the sha256 buffer.  When we get 16 bits, we divide
	// the unsigned value by 10922 and add 1.  NOTE, the number CAN be 7. 
	// if so, we simply roll again.  0xffff/10922+1 and 0xfffe/10922+1 are 7,
	// so we simply ignore those values.  But all 16 bit values from 0 to
	// 0xfffd WILL provide us with exact uniform distribution.
	unsigned short s = 0;
	unsigned i;
	for (i = 0; i < 16; ++i) {
		entropy_slurp(pE);
		s |= (entropy_get_bit(pE) << i);
	}
	s /= 10922;
	++s;
	if (s == 7)
		return entropy_get_die(pE);
	return s;
}

#if 0
HMODULE WINAPI LoadLibrary(
  _In_ LPCTSTR lpFileName
);

FARPROC WINAPI GetProcAddress(
  _In_ HMODULE hModule,
  _In_ LPCSTR  lpProcName
);

NTSTATUS WINAPI NtQuerySystemInformation(
  _In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
  _Inout_   PVOID                    SystemInformation,
  _In_      ULONG                    SystemInformationLength,
  _Out_opt_ PULONG                   ReturnLength
);

typedef struct _SYSTEM_PERFORMANCE_INFORMATION {
    BYTE Reserved1[312];
} SYSTEM_PERFORMANCE_INFORMATION;

#endif
