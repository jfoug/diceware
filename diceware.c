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

const char *wordlist_file = "beale.wordlist.asc";

char *words[7776];
void init_word_data () {
	FILE *in = fopen(wordlist_file, "rt");
	char Line[256];
	int i;

	if (!in)
		exit(fprintf(stderr, "Error, could not open wordlist file: %s\n", wordlist_file));
	fgets(Line, 256, in);
	while (!feof(in) && strncmp(Line, "11111\t", 6)) {
		fgets(Line, 256, in);
	}
	if (feof(in)) {
		fprintf(stderr, "Error 1 loading %s\n", wordlist_file);
		exit(1);
	}
	for (i = 0; i < 7776; ++i) {
		strtok(Line, "\r\n");
		words[i] = strdup(Line);
		fgets(Line, 256, in);
		if (feof(in)) {
			fprintf(stderr, "Error 2 loading %s  Line is %s\n", wordlist_file, Line);
			exit(1);
		}
		if (i < 7775 && atoi(Line) < 1 || atoi(Line) > 66666) {
			fprintf(stderr, "Error 3 loading %s  Line is %s  i=%d\n", wordlist_file, Line, i);
			exit(1);
		}
	}
	fclose(in);
}

char symbol(int d1, int d2) {
	const char *syms = "~!#$%^ *()-=+[]\\{}:;,.<>?/0123456789";
	if (d1 < 1 || d2 < 1 || d1 > 6 || d2 > 6)
		return ' ';
	return syms[(d1-1)*6+d2-1];
}

int main(int argc, char **argv) {
	unsigned int seed[16];
	int i, j;
	double d;
	char buf[512], *cp=buf;
	entropy E;
	SHA256_CTX ctx;
	int die[7];
	char dies[7];
	char pass[256], *cpass=pass;

//	printf ("Stirring the pot to create entropy.  quickly move mouse a lot to make it better\n");
	// First, init the entropy structure.
	E.init = 0;
	entropy_slurp(&E);
	// Load words
	init_word_data();
	// stir the pot one last time (after the file reading stuff.)
	entropy_slurp(&E);

//	printf("Now Rolling the Dice\n");
	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 7; ++j) {
			die[j] = entropy_get_die(&E);
//			printf ("  %d", die[j]);
		}
		sprintf(dies, "%d%d%d%d%d\t", die[0],die[1],die[2],die[3],die[4]);
		for (j = 0; j < 7776; ++j) {
			if (!strncmp(dies, words[j], 6)) {
				cpass += sprintf(cpass, "%s %c ", &words[j][6], symbol(die[5], die[6]));
//				printf ("  -  %s %c", &words[j][6], symbol(die[5], die[6]));
				break;
			}
		}
//		printf("\n");
	}
	printf ("passphrase is:   %s\n", pass);
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
