# simple makefile for diceware.
# note, current implentation of rnd requires windows (for entropy gathering).
# build with Cygwin (or mingw), or VisualC.

# Copyright (c) 2016, Jim Fougeron
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
 
OBJS = diceware.o rnd.o
CC = gcc

#CFLAGS = -O2

diceware: $(OBJS)
	$(CC) -o diceware $(OBJS) -lssl -lcrypto -lcurl

.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f *.o *.exe
