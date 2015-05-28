// RUN: %clangxx_asan -O0 -mllvm -asan-instrument-allocas %s -o %t
// RUN: %run %t 2>&1
//
// XFAIL: arm-linux-gnueabi
// XFAIL: armv7l-unknown-linux-gnueabihf

// This testcase checks that allocas and VLAs inside loop are correctly unpoisoned.

#include <assert.h>
#include <alloca.h>
#include <stdint.h>
#include "sanitizer/asan_interface.h"

void *top, *bot;

__attribute__((noinline)) void foo(int len) {
  char x;
  top = &x;
  char array[len];  // NOLINT
  assert(!(reinterpret_cast<uintptr_t>(array) & 31L));
  alloca(len);
  for (int i = 0; i < 32; ++i) {
    char array[i];  // NOLINT
    bot = alloca(i);
    assert(!(reinterpret_cast<uintptr_t>(bot) & 31L));
  }
}

int main(int argc, char **argv) {
  foo(32);
  void *q = __asan_region_is_poisoned(bot, (char *)top - (char *)bot);
  assert(!q);
  return 0;
}