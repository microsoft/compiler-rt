/*===- PGOProfiling.c - Support library for PGO instrumentation -----------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source
|* License. See LICENSE.TXT for details.
|*
\*===----------------------------------------------------------------------===*/

#include <stdio.h>
#include <stdlib.h>

// Explicitly request that the format macros like PRIu64 be enabled if they
// haven't already been enabled.
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#if !defined(__APPLE__)
#include <inttypes.h>
#endif

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef unsigned int uint32_t;
typedef unsigned int uint64_t;
#endif

static FILE *OutputFile = NULL;

/*
 * A list of functions to write out the data.
 */
typedef void (*writeout_fn)();

struct writeout_fn_node {
  writeout_fn fn;
  struct writeout_fn_node *next;
};

static struct writeout_fn_node *writeout_fn_head = NULL;
static struct writeout_fn_node *writeout_fn_tail = NULL;

void llvm_pgo_emit(const char *MangledName, uint32_t NumCounters,
                   uint64_t *Counters) {
  uint32_t i;
  fprintf(OutputFile, "%s %u\n", MangledName, NumCounters);
  for (i = 0; i < NumCounters; ++i)
    fprintf(OutputFile, "%" PRIu64 "\n", Counters[i]);
  fprintf(OutputFile, "\n");
}

void llvm_pgo_register_writeout_function(writeout_fn fn) {
  struct writeout_fn_node *new_node = malloc(sizeof(struct writeout_fn_node));
  new_node->fn = fn;
  new_node->next = NULL;

  if (!writeout_fn_head) {
    writeout_fn_head = writeout_fn_tail = new_node;
  } else {
    writeout_fn_tail->next = new_node;
    writeout_fn_tail = new_node;
  }
}

void llvm_pgo_writeout_files() {
  OutputFile = fopen("pgo-data", "w");
  if (!OutputFile) return;

  while (writeout_fn_head) {
    struct writeout_fn_node *node = writeout_fn_head;
    writeout_fn_head = writeout_fn_head->next;
    node->fn();
    free(node);
  }

  fclose(OutputFile);
}

void llvm_pgo_init(writeout_fn wfn) {
  static int atexit_ran = 0;

  if (wfn)
    llvm_pgo_register_writeout_function(wfn);

  if (atexit_ran == 0) {
    atexit_ran = 1;
    atexit(llvm_pgo_writeout_files);
  }
}