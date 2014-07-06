#include <stdlib.h>
#include <string.h>
#include "mm.h"
#include "console.h"
#include "cmd.h"

#define MAX_PAGES 16
#define PAGE_SIZE 4194304
#define MAX_BLOCKS 8192

typedef struct {
  void *p;
  int size;
} block_t;

static block_t bs[MAX_BLOCKS];
static int nb;
static void *pgs[MAX_PAGES];
static int inited = 0;

static void mmCheck() {
  int i, j;
  for (i = 0; i < nb; ++i) {
    int bo = 0;
    for (j = 0; j < MAX_PAGES && pgs[j] != NULL; ++j)
      if (bs[i].p >= pgs[j] && bs[i].p + bs[i].size <= pgs[j] + PAGE_SIZE) {
        ++bo;
        break;
      }
    if (!bo) {
      cmsg(MLERR, "mmCheck: block-page boundary violation (%d)", i);
      break;
    }
    bo = 0;
    for (j = 0; j < nb; ++j)
      if (bs[i].p < bs[j].p && bs[i].p + bs[i].size > bs[j].p) {
        ++bo;
        break;
      }
    if (bo) {
      cmsg(MLERR, "mmCheck: block-block boundary violation (%d)", i);
      break;
    }
  }
}

static unsigned mmGetPos(void *p) {
  int m = 0, l = 0, r = nb - 1;
  while (l <= r) {
    m = (l + r) / 2;
    if (bs[m].p == p) {
      if (!bs[m].size && m < nb) ++m;
      return m;
    }
    if (bs[m].p > p)
      r = m - 1;
    else
      l = m + 1;
  }
  if (r < 0) r = 0;
  for (; r < nb && bs[r].p <= p; ++r);
  return r;
}

static int mmGetExactPos(void *p) {
  int m = 0, l = 0, r = nb - 1;
  while (l <= r) {
    m = (l + r) / 2;
    if (bs[m].p == p) {
      if (!bs[m].size && (++m == nb || bs[m].p != p)) m = -1;
      return m;
    }
    if (bs[m].p > p)
      r = m - 1;
    else
      l = m + 1;
  }
  return -1;
}

static int mmGetPageForPtr(void *p) {
  int i;
  for (i = 0; i < MAX_PAGES && pgs[i] != NULL; ++i)
    if (p >= pgs[i] && p < pgs[i] + PAGE_SIZE) return i;
  return -1;
}

static void mmInsert(void *p, int size) {
  if (nb == MAX_BLOCKS) {
    printf("mmInsert(): no more blocks\n");
    return;
  }
  int i = mmGetPos(p), j;
  for (j = nb; j > i; --j) bs[j] = bs[j - 1];
  bs[j].p = p;
  bs[j].size = size;
//  printf("mmInsert(%08x, %d);\n", (int)p, size);
  ++nb;
  if (size) memset(p, 0, size);
}

static void *mmPageAlloc() {
  int i;
  for (i = 0; i < MAX_PAGES && pgs[i] != NULL; ++i);
  if (i == MAX_PAGES) {
    cmsg(MLERR, "mmPageAlloc: out of pages");
    return NULL; /* no more pages */
  }
  void *p = malloc(PAGE_SIZE);
  if (p == NULL) {
    cmsg(MLERR, "malloc failed to alloc %d bytes", PAGE_SIZE);
    return NULL; /* no more memory */
  }
  for (i = 0; i < MAX_PAGES; ++i)
    if (pgs[i] == NULL || p < pgs[i]) {
      int j;
      for (j = MAX_PAGES-1; j > i; --j) pgs[j] = pgs[j - 1];
      pgs[i] = p;
      memset(p, 0, PAGE_SIZE);
      cmsg(MLDBG, "mmPageAlloc: ok");
      mmInsert(p, 0);
      return p;
    }
  /* we cant get here! */
  cmsg(MLERR, "mmPageAlloc: out of zones!");
  return NULL;
}

void *mmAlloc(int size) {
  if (!inited) return NULL;
  if (!size || size > PAGE_SIZE) {
    cmsg(MLERR, "mmAlloc is unable to alloc %d bytes", size);
    return NULL;
  }
//  printf("mmAlloc(%d): ", size);
  if (size & 7) size = (size | 7) + 1;
//  printf("mmAlloc(%d);\n", size);
  int i, j;
  if (pgs[0] == NULL) {
    mmPageAlloc();
    if (pgs[0] == NULL) return NULL;
  }
  void *p = pgs[0];
  for (i = 0; i < nb; ++i) {
    if (p + size <= bs[i].p) {
      j = mmGetPageForPtr(p);
      if (j >= 0 && p + size <= pgs[j] + PAGE_SIZE) {
        mmInsert(p, size);
        mmCheck();
//        printf("talalt egy lyukat\n");
        return p;
      }
    }
    p = bs[i].p + bs[i].size;
  }
  j = mmGetPageForPtr(p);
  if (j >= 0 && p + size <= pgs[j] + PAGE_SIZE) {
    mmInsert(p, size);
    mmCheck();
//    printf("utolso blokk\n");
    return p;
  }
  if ((p = mmPageAlloc()) != NULL) {
    mmInsert(p, size);
    mmCheck();
//    printf("uj lapon\n");
    return p;
  }
  cmsg(MLERR, "mmAlloc is unable to find free memory");
  return NULL;
}

void mmFree(void *p) {
  if (!inited) return;
//  printf("mmFree(%08x);\n", (int)p);
  int i = mmGetExactPos(p);
  if (i < 0 || !bs[i].size) {
    cmsg(MLERR, "mmFree: given pointer doesnt address an allocated zone (%08x)\n", (int)p);
    return;
  }
  memset(bs[i].p, 0, bs[i].size);
  for (; i < nb; ++i) bs[i] = bs[i + 1];
  --nb;
}

void mmInit() {
  if (inited) return;
  int i;
  for (i = 0; i < MAX_PAGES; ++i)
    pgs[i] = NULL;
  nb = 0;
  ++inited;
}

void mmDone() {
  if (!inited) return;
  inited = 0;
  int i, bo = 0;
  for (i = 0; i < nb; ++i) {
    if (!bs[i].size) continue; /* skip markers */
    if (!bo) {
      cmsg(MLDBG, "mmDone: unfreed blocks:");
      ++bo;
    }
    cmsg(MLDBG, " 0x%08x %d", (int)bs[i].p, bs[i].size);
  }
  for (i = 0; i < MAX_PAGES; ++i)
    if (pgs[i] != NULL) {
      memset(pgs[i], 0, PAGE_SIZE);
      free(pgs[i]);
      pgs[i] = NULL;
    }
  nb = 0;
}

static int cmd_meminfo(int argc, char **argv) {
  cmsg(MLINFO, "limits:");
  cmsg(MLINFO, " MAX_PAGES: %d", MAX_PAGES);
  cmsg(MLINFO, " PAGE_SIZE: %d Bytes", PAGE_SIZE);
  cmsg(MLINFO, " MAX_BLOCKS: %d", MAX_BLOCKS);
  cmsg(MLINFO, " total memory: %d Bytes", MAX_PAGES * PAGE_SIZE);
  cmsg(MLINFO, " optimal blocksize: %d Bytes", MAX_PAGES * PAGE_SIZE / MAX_BLOCKS);
  cmsg(MLINFO, "state:");
  int i, np = 0;
  for (i = 0; i < MAX_PAGES; ++i)
    if (pgs[i] != NULL) ++np;
  cmsg(MLINFO, " pages in use: %d (%d%%)", np, 100 * np / MAX_PAGES);
  cmsg(MLINFO, " blocks in use: %d (%d%%)", nb, 100 * nb / MAX_BLOCKS);
  unsigned alloc = 0;
  for (i = 0; i < nb; ++i)
    alloc += bs[i].size;
  cmsg(MLINFO, " allocated: %d Bytes (%d%%)", alloc, 100 * alloc / (MAX_PAGES * PAGE_SIZE));
  cmsg(MLINFO, " free: %d Bytes", MAX_PAGES * PAGE_SIZE - alloc);
  cmsg(MLINFO, " average blocksize: %d Bytes", alloc / nb);
  for (i = 0; i < MAX_PAGES && pgs[i] != NULL; ++i) {
    alloc = 0;
    int j;
    np = 0;
    for (j = 0; j < nb; ++j)
      if (mmGetPageForPtr(bs[j].p) == i) {
        alloc += bs[j].size;
        ++np;
      }
    cmsg(MLINFO, " page #%02d: %8d Bytes, %4d blocks (%d%%)",
      i, alloc, np, 100 * alloc / PAGE_SIZE);
  }
  return 0;
}

void mmInitCmd() {
  cmdAddCommand("meminfo", cmd_meminfo);
}

void mmShow() {
  int i;
  printf("[blocks]\n");
  for (i = 0; i < nb; ++i) printf("[%08x %x]\n", (int)bs[i].p, bs[i].size);
  printf("[end]\n");
  printf("[pages]\n");
  for (i = 0; i < MAX_PAGES && pgs[i] != NULL; ++i)
    printf("[%08x]\n", (int)pgs[i]);
  printf("[end]\n");
}

/*
int main() {
  mmInit();
  void *p1 = mmAlloc(512), *p2 = mmAlloc(512);
  printf("%08x %08x\n", (int)p1, (int)p2);
  mmFree(&p1);
  p1 = mmAlloc(513);
  printf("%08x %08x\n", (int)p1, (int)p2);
  mmShow();
  mmFree(&p1);
  mmFree(&p2);
  mmDone();
  return 0;
}
*/
