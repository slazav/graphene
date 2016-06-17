#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "locklib.h"

static key_t string_to_key1(char *string) {
  /* This is stolen from tclHash.c */
  key_t result;
  int c;

  /*
   * I tried a zillion different hash functions and asked many other
   * people for advice.  Many people had their own favorite functions,
   * all different, but no-one had much idea why they were good ones.
   * I chose the one below (multiply by 9 and add new character)
   * because of the following reasons:
   *
   * 1. Multiplying by 10 is perfect for keys that are decimal strings,
   *    and multiplying by 9 is just about as good.
   * 2. Times-9 is (shift-left-3) plus (old).  This means that each
   *    character's bits hang around in the low-order bits of the
   *    hash value for ever, plus they spread fairly rapidly up to
   *    the high-order bits to fill out the hash value.  This seems
   *    works well both for decimal and non-decimal strings.
   */

  result = 0;
  while (1) {
    c = *string;
    string++;
    if (c == 0) {
      break;
    }
    result += (result<<3) + c;
  }
  if (result == IPC_PRIVATE) result++;
  return result;
}

void main() {
  printf("gpibbus1: %x\n",string_to_key1("gpibbus1"));
}
