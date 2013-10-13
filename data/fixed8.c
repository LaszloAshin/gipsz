#include <stdio.h>
#include "tga.h"

void pru(unsigned c) {
  int i;
  for (i = 0; i < 32; ++i)
    putchar(((c >> i) & 1) + '0');
  putchar('\n');
}

int main() {
  tgaimage_t im;
  if (!tgaRead(&im, "fixed8.tga")) return 1;
  printf("w=%u h=%u b=%u\n", im.width, im.height, im.Bpp);
  int i;
  for (i = 0; i < 96; ++i) {
    int y, x;
    unsigned char c[16];
    printf("  { ");
    for (y = 0; y < 16; ++y) {
      c[y] = 0;
      for (x = 0; x < 8; ++x)
        c[y] |= (im.data[y*128+x+(i & 15)*8+(i >> 4)*2048] & 1) << (7 - x);
//      pru(c);
    }
    for (y = 0; y < 16; ++y)
      printf("0x%02x, ", c[15-y]);
    printf("},\n");
  }
  tgaFree(&im);
  return 0;
}
