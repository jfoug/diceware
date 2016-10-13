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
#include <math.h>
#include <unistd.h>

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
	const char *syms = "~!#$%^_*()-=+[]\\{}:;,.<>?/0123456789";
	if (d1 < 1 || d2 < 1 || d1 > 6 || d2 > 6)
		return ' ';
	return syms[(d1-1)*6+d2-1];
}

int debug=0;
int num_words=4;
int with_sym=0;
int with_cap=0;
double until_bits = 0.0;

void usage() {
	exit(fprintf(stderr, "usage:  diceware [options]\n\tOptions:\n\t-w=#    Number of words in phrase\n\t-s    Use symbols between words\n\t-b=#   compute longer passphrase until this many bits of entropy are achieved\n"));
}
int load_args(int argc, char **argv) {
	int c;
	int w_used=0;

	opterr = 0;
	while ((c = getopt (argc, argv, "cdsw:b:")) != -1)
	switch (c)
	{
		case 'c':
			++with_cap;
			break;
		case 'd':
			++debug;
			break;
		case 's':
			++with_sym;
			if (with_sym > 3)
				--with_sym;
			break;
		case 'w': {
			char *cp = optarg;
			if (cp && *cp == '=') ++cp;
			num_words = atoi(cp);
			w_used = 1;
			break;
		}
		case 'b': {
			char *cp = optarg;
			if (cp && *cp == '=') ++cp;
			until_bits = atof(cp);
			break;
		}
		case '?':
			if (optopt == 'w')
			  fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint (optopt))
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
			return 1;
		default:
			usage();
	}
	if (w_used && until_bits > 0.0) {
		fprintf(stderr, "Error, using -w and -b is mutually exclusive\n\n");
		usage();
	}
}
int main(int argc, char **argv) {
	int i, j, k;
	double bits=0;
	entropy E;
	int die[11];
	char dies[11], pass[512], *cpass=pass, *this_word;

	E.init = 0;
	entropy_slurp(&E);
	init_word_data();
	load_args(argc, argv);
	entropy_slurp(&E);

	if (until_bits > 0.0) {
		num_words = 10000;
	}

	for (i = 0; i < num_words; ++i) {
		for (j = 0; j < 5+(with_sym<<1); ++j) {
			die[j] = entropy_get_die(&E);
		}
		sprintf(dies, "%d%d%d%d%d\t", die[0],die[1],die[2],die[3],die[4]);
		for (j = 0; j < 7776; ++j) {
			char word[24];
			int case_bits = 0;
			if (!strncmp(dies, words[j], 6)) {
				strcpy(word, &words[j][6]);
				for (k = 0; k < with_cap; ++k) {
					if (word[k]>= 'a' && word[k] <= 'z') {
						++case_bits;  // single cap is 1 bit of entropy.
						entropy_slurp(&E);
						if (entropy_get_bit(&E))
							word[k] -= 0x20;
					}
				}
				if (until_bits==0 && with_sym && num_words == num_words-1)
					with_sym = 0;
				bits += case_bits+log(pow(6,5+(with_sym<<1)))/log(2.);
				this_word = cpass;
				cpass += sprintf(cpass, "%s ", word);
				for (k = 0; k < with_sym; ++k) {
					cpass += sprintf(cpass, "%c", symbol(die[k*2+5],die[k*2+6]));
				}
				if (with_sym)
					cpass += sprintf(cpass, " ");
				if (debug) {
					int out_len = 0;
					for (k = 0; k < 5+(with_sym<<1); ++k)
						out_len += printf ("%d ", die[k]);
					out_len += printf ("  -  %s", this_word);
					for (k = out_len; k < 40; ++k)
						printf(" ");
					printf ("(Added %0.2f bits entropy)\n", case_bits+log(pow(6,5+(with_sym<<1)))/log(2.));
				}
				break;
			}
		}
		if (until_bits > 0.0 && until_bits < bits) {
			if (!with_sym)
				break;
			with_sym = 0;
		}
		if (until_bits > 0.0 && with_sym && until_bits < bits+log(pow(6,5))/log(2.))
			with_sym = 0;
	}
	entropy_slurp(&E);
	memset(&E, 0, sizeof(E));
	memset(dies, 0, sizeof(dies));
	memset(die, 0, sizeof(die));
	printf ("passphrase is:   %s\n", pass);
	printf ("Entropy is:      %0.2f bits\n", bits);
	memset(pass, 0, sizeof(pass));
	return 0;
}
