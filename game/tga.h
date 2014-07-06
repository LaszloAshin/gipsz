#ifndef _TGA_H
#define _TGA_H 1

typedef struct {
  unsigned width;
  unsigned height;
  unsigned bytes;
  unsigned char *data;
} tgaimage_t;

int tgaRead(tgaimage_t *im, const char *fname);
void tgaFree(tgaimage_t *im);
int tgaScale(tgaimage_t *d, tgaimage_t *s);

#endif /* _TGA_H */
