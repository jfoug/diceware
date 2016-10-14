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

#ifdef __CYGWIN__
// not sure why I have to do this, BUT it must be done??  Else SOCKET is not defined.
// if we include winsock2.h, then there are TONS of redef errors.
#include <w32api/psdk_inc/_socket_types.h>
#endif

#ifdef _MSC_VER
#define CURL_STATICLIB
#endif

#include <curl/curl.h>

typedef NTSTATUS (__stdcall *NtQuerySysInfo_fp)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NtQuerySysInfo_fp NtQuerySystemInfo;


static void dump(const unsigned char *msg, const unsigned char *cp, int len) {
	int i;
	static char *hex = "0123456789abcdef";
	if (msg)
		fprintf(stderr,"%s : ", msg);
	for (i = 0; i < len; ++i)
		fprintf (stderr, "%c%c", hex[cp[i]>>4], hex[cp[i]&0xF]);
	fprintf (stderr, "\n");
}

static size_t entropy_write_curl_data(void *ptr, size_t size, size_t nmemb, void *Entropy)
{
	entropy *E = (entropy *)Entropy;
	if (E->random_org_data_read >= sizeof(E->random_org_data))
		;
	else if (E->random_org_data_read + size*nmemb > sizeof(E->random_org_data)) {
		memcpy(&(E->random_org_data[E->random_org_data_read]), ptr, sizeof(E->random_org_data) - E->random_org_data_read);
		E->random_org_data_read = sizeof(E->random_org_data);
	} else {
		memcpy(&(E->random_org_data[E->random_org_data_read]), ptr, size*nmemb);
		E->random_org_data_read += size*nmemb;
	}
	return size*nmemb;
}

void entropy_curl_get_random_org_data(entropy *E) {
	CURL *curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if(curl)
	{
		// this url receives a 'raw' file of bytes (the format=f is a file download).
		curl_easy_setopt(curl, CURLOPT_URL, "https://www.random.org/cgi-bin/randbyte?nbytes=256&format=f");
		// Ok, tell the entropy_write_curl_data we want the entire buffer filled (256 bytes)
		E->random_org_data_read = 0;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, E);
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);  // debugging while developing.
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, entropy_write_curl_data);
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
			fprintf(stderr, "Unable to get data from random.org Curl error: %s\n", curl_easy_strerror(res));
		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	//dump("random.org data", E->random_org_data, E->random_org_data_read);
}
static void entropy_init(entropy *E) {
	SHA256_CTX ctx;
	int j;

	memset(E, 0, sizeof(entropy));
	entropy_curl_get_random_org_data(E);
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, E, sizeof(E));
	SHA256_Final(E->sha256_h, &ctx);
	if (!NtQuerySystemInfo) {
		HMODULE h = LoadLibrary("ntdll.dll");
		NtQuerySystemInfo = (NtQuerySysInfo_fp)GetProcAddress(h, "NtQuerySystemInformation");
	}
	E->init = 1;
	for (j = 0; j < 10; ++j)
		entropy_slurp(E);
	for (j = 1; j < 32; ++j) {
		E->init |= (entropy_get_bit(E) << j);
		entropy_slurp(E);
	}
	E->init |= entropy_get_bit(E);
	for (j = 1; j < 10; ++j)
		entropy_slurp(E);
	// get new data from randome_org_data, so that the data used 'long' term is
	// not the same as what we originally seeded with.
	entropy_curl_get_random_org_data(E);
	entropy_slurp(E);
}

unsigned char *entropy_slurp(entropy *E) {
	ULONG sil;
	SHA256_CTX ctx;
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION asi[64]; // handle system with up to 64 CPU's

	if (E->init == 0)
		entropy_init(E);
	// grab system 'current' metrics.
	E->counter++;
	NtQuerySystemInfo(SystemPerformanceInformation, &E->perf, sizeof(E->perf), &sil);
	E->li1 = GetTickCount();
	GetCursorPos(&E->pt);
	QueryPerformanceCounter(&E->li2);
	QueryUnbiasedInterruptTime ((PULONGLONG)&E->li3);
	NtQuerySystemInfo(SystemProcessorPerformanceInformation, &asi, sizeof(asi), &sil);
	memcpy(&E->pperf, asi, sizeof(E->pperf));
	// pause system an arbitrary amount of time
	Sleep(2);
	// compress/update into the sha256 buffer twice (includes all prior sha256 calls)
	SHA256_Init(&ctx); SHA256_Update(&ctx, E, sizeof(entropy)); SHA256_Final(E->sha256_h, &ctx);
	SHA256_Init(&ctx); SHA256_Update(&ctx, E, sizeof(entropy)); SHA256_Final(E->sha256_h, &ctx);
	// update system metrics again (proc performance first this time
	NtQuerySystemInfo(SystemProcessorPerformanceInformation, &asi, sizeof(asi), &sil);
	memcpy(&E->pperf2, asi, sizeof(E->pperf2));
	QueryPerformanceCounter(&E->li2);
	QueryUnbiasedInterruptTime ((PULONGLONG)&E->li3);
	NtQuerySystemInfo(SystemPerformanceInformation, &E->perf2, sizeof(E->perf2), &sil);
	E->li1 = GetTickCount();
	GetCursorPos(&E->pt);
	// again, pause arbitrary amount of time (so that an instant call into entropy_slurp
	// will not happen without 'sleep'
	Sleep(2);
	// compress/update into the sha256 buffer.  That buffer IS our return of 32 bytes.
	SHA256_Init(&ctx); SHA256_Update(&ctx, E, sizeof(entropy)); SHA256_Final(E->sha256_h, &ctx);
//	dump(E);
	return E->sha256_h;
}

// This function gets only 1 bit for the entire entropy hash value.
unsigned entropy_get_bit(entropy *pE) {
	uint32_t i, ch = 0;
	for (i = 0; i < 8; ++i) {
		ch ^= pE->sha256_32[i];
		_rotl(ch,3);  // works for VC and cygwin. The below line also is rotl, but slower.
//		ch = (ch<<3)|(ch>>29);
	}
	return ch > 0x7fffffff;
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
