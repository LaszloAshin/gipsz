#include <stdio.h>
#include <string.h>
#include "chars-mw.h"

int main() {
  int out[6144];
  int op = 0;
  int x, y;
  memset(out, 0, sizeof(out));
  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x) {
      out[op >> 5] |= header_data[op] << (op & 31);
      ++op;
    }
  printf("static const unsigned chars[96][64] = {\n");
  for (x = 0; x < 96; ++x) {
    printf("  {\n    ");
    for (y = 0; y < 64; ++y) {
      printf("0x%08x, ", out[y*16+(x & 15)+(x >> 4)*1024]);
      if (!((y+1) & 7)) {
        printf("\n");
        if (y < 63) printf("    ");
      }
    }
    printf("  },\n");
  }
  printf("};\n");
  return 0;
}
