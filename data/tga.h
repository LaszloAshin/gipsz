#ifndef _TGA_H
#define _TGA_H 1

typedef struct {
  unsigned width;
  unsigned height;
  char Bpp;
  char *data;
} tgaimage_t;

extern int tgaRead(tgaimage_t *im, const char *fname);
extern void tgaFree(tgaimage_t *im);

#endif /* _TGA_H */
