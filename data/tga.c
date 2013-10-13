#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tga.h"

typedef struct {
  /**
   * magic:
   *  0x00020000 if not compressed
   *  0x00030000 if grayscale
   *  0x000a0000 if rle compressed
   */
  unsigned magic : 32;
  char filler[8];
  unsigned short width : 16;
  unsigned short height : 16;
  /**
   * magic2:
   *  0x2008 - grayscale 8bit & origin at top left
   *  0x0018 - RGB & origin at bottom left
   *  0x0820 - RGBA & origin at bottom left
   *  0x2018 - RGB & origin at top left
   */
  unsigned short magic2 : 16;
} __attribute__((packed)) tgaheader_t;

int tgaRead(tgaimage_t *im, const char *fname) {
  int ret = 0;
  char Bpp = 0;
  if (im == NULL || fname == NULL) goto end2;
  FILE *f = fopen(fname, "rb");
  if (f == NULL) goto end2;
  tgaheader_t th;
  if (!fread(&th, sizeof(tgaheader_t), 1, f)) goto end1;
  if (th.magic != 0x00020000 && th.magic != 0x00030000) goto end1;
  switch (th.magic2) {
    case 0x2008:
      Bpp = 1;
      break;
    case 0x0018:
    case 0x2018:
      Bpp = 3;
      break;
    case 0x0820:
      Bpp = 4;
      break;
    default:
      goto end1;
  }
  unsigned size = th.width * th.height * Bpp;
  im->data = (char *)malloc(size);
  if (im->data == NULL) goto end1;
  if (!fread(im->data, size, 1, f)) goto end1;
  im->width = th.width;
  im->height = th.height;
  im->Bpp = Bpp;
  ++ret;
 end1:
  fclose(f);
 end2:
  return ret;
}

void tgaFree(tgaimage_t *im) {
  if (im == NULL) return;
  free(im->data);
  memset(im->data, 0, sizeof(tgaimage_t));
}
