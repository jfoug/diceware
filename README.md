# diceware
crypto secure diceware passphrase generation

This tool, and crypto random generation code implements a VERY secure implementation of the
diceware passphrase generator. There are numerous options which can be used.

Diceware is an algorithm for genration of provable secure passphrases.  It uses 5 dice, and
from that picks 1 of 7776 simple words.  The dice can be re-roled and additional words added.
Each 'simple' diceword word provides about 12.92 provable bits of security:  log(2,7776).

Currently, the random engine will only work on windows machines (build with Cygwin, MinGW
or VC).  This is only due to how the program builds up entropy.  The rnd.c file will be
enhanced over time, and more systems will be handled.  One goal is to get this running
as securely as it does (true random), on linux and android.  On linux, it would be easy
to simply use /dev/random   to get true random data.  Should be an easy 'change'.

This code provides extensions to diceware, which are also add provable security. Optionally
one of 36 additional symbols can be inserted between the words (the program will add the
same amount each time, 0 to 3 of them).  Each symbol provides 5.17 bits of security.

The code also allows random casing of letters.  Each random cased letter will add 1 bit
of security, but only if the word contained a letter in that possition.

-w=#   how many words are in the passphrase
-b=#   how many bits of entropy are wanted   (Note, -w and -b are mutually exclusive)
-d     show some debugging output (data as it is being created)
-c     Adds random casing to first letter (adds up to 1 bit of entropy per word)
-c -c  Adds random casing to first 2 letters of each word (up to 2 bits entropy per word)
-ccc   Adds random casing to first 3 letters (also -c -c -c does same thing)
-s     Adds a random symbol between each word.  (-s -s and -s -s -s, or -ss -sss give 2 or 3 symbols)

The provable amount of entropy (i.e. bits of security) of the passphrase are also printed with the
passphrase.

ONE big note.  You will have to remember the password.  Having a super secure password is NOT
helpful or secure if you can not remember it yourself.  Adding symbols, casing helps the security
a lot, BUT if you can not remember the passphrase, then it does not help.  The simple original
diceware algorithm by itself (with more words), may produce a 'simpler' passphrase that is
to remember in the end.

./diceware -b80   Produced
   wn waldo hurt pods vend 72 tend    Which is 90.47 bits.
   
./diceware -b80 -ss -cccc produced
  vILe #/ 40% >0 sHout -_ hilT        Which is 94.72 bits.
  
  The first was 7 'words', the 2nd was only 4. But in the end, there are also 3 two byte
  symbols and random casing ALL of which must be remembered.
  
  
  The random code.
