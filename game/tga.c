#include <stdio.h>
#include <string.h>
#include "tga.h"
#include "mm.h"
#include "console.h"

/* ideas from tga.c of Gimp */
typedef struct tga_info_s {
  unsigned idLength;
  unsigned colorMapType;
  enum { TGA_TYPE_MAPPED, TGA_TYPE_COLOR, TGA_TYPE_GRAY } imageType;
  enum { TGA_COMP_NONE, TGA_COMP_RLE } imageCompression;
  unsigned colorMapIndex;
  unsigned colorMapLength;
  unsigned colorMapSize;
  unsigned xOrigin, yOrigin;
  unsigned width, height, bpp, bytes;
  unsigned alphaBits, flipHoriz, flipVert;
} tga_info_t;

static void *ReadImage(FILE *fp, tga_info_t *info, const char *fname);

int tgaRead(tgaimage_t *im, const char *fname) {
  int ret;
  FILE *fp;
  unsigned char hd[18];
  static const char tgamagic[] = "TRUEVISION-XFILE.";
  char magic[sizeof(tgamagic)];
  tga_info_t info;
  void *p;

  ret = 0;
  fp = fopen(fname, "rb");

  if (fp == NULL) {
    cmsg(MLERR, "tgaRead: unable to open file '%s'", fname);
    return ret;
  }

  if (fseek(fp, -sizeof(magic), SEEK_END) || fread(magic, sizeof(magic), 1, fp) != 1) {
    cmsg(MLERR, "tgaRead: missing tail of file '%s'", fname);
    goto exit;
  }

  if (memcmp(magic, tgamagic, sizeof(tgamagic))) {
    cmsg(MLERR, "tgaRead: invalid magic in file '%s'", fname);
    goto exit;
  }

  if (fseek(fp, 0, SEEK_SET) || fread(hd, sizeof(hd), 1, fp) != 1) {
    cmsg(MLERR, "tgaRead: missing head of file '%s'", fname);
    goto exit;
  }

  switch (hd[2] & 0x03) {
    case 0x01:
      info.imageType = TGA_TYPE_MAPPED;
      break;
    case 0x02:
      info.imageType = TGA_TYPE_COLOR;
      break;
    case 0x03:
      info.imageType = TGA_TYPE_GRAY;
      break;
    default:
      cmsg(MLERR, "unknown image type for '%s'", fname);
      goto exit;
  }
  if (hd[2] & 0x08) {
    info.imageCompression = TGA_COMP_RLE;
  } else {
    info.imageCompression = TGA_COMP_NONE;
  }

  info.idLength     = hd[0];
  info.colorMapType = hd[1];

  info.colorMapIndex  = hd[3] + hd[4] * 256;
  info.colorMapLength = hd[5] + hd[6] * 256;
  info.colorMapSize   = hd[7];

  info.xOrigin = hd[ 8] + hd[ 9] * 256;
  info.yOrigin = hd[10] + hd[11] * 256;
  info.width   = hd[12] + hd[13] * 256;
  info.height  = hd[14] + hd[15] * 256;

  info.bpp       = hd[16];
  info.bytes     = (info.bpp + 7) / 8;
  info.alphaBits = (hd[17] & 0x0f);
  info.flipHoriz = (hd[17] & 0x10) ? 1 : 0;
  info.flipVert  = (hd[17] & 0x20) ? 0 : 1;

  switch (info.imageType) {
    case TGA_TYPE_MAPPED:
      if (info.bpp != 8) {
        cmsg(MLERR, "unhandled sub-format in '%s'", fname);
        goto exit;
      }
      break;
    case TGA_TYPE_COLOR:
      if (info.bpp != 15 && info.bpp != 16 && info.bpp != 24 && info.bpp != 32) {
        cmsg(MLERR, "unhandled sub-format in '%s'", fname);
        goto exit;
      }
      break;
    case TGA_TYPE_GRAY:
      if (info.bpp != 8 && (info.alphaBits != 8 || (info.bpp != 15 && info.bpp != 16))) {
        cmsg(MLERR, "unhandled sub-format in '%s' %d %d", fname, info.bpp, info.alphaBits);
        goto exit;
      }
      break;
  }

  if (info.bytes * 8 != info.bpp && !(info.bytes == 2 && info.bpp == 15)) {
    cmsg(MLERR, "not support for tga with these parameters");
    goto exit;
  }

  if (info.imageType == TGA_TYPE_MAPPED && info.colorMapType != 1) {
    cmsg(MLERR, "indexed image has invalid color map type %d", info.colorMapType);
    goto exit;
  } else if (info.imageType != TGA_TYPE_MAPPED && info.colorMapType != 0) {
    cmsg(MLERR, "Non-indexed image has invalid color map type %d", info.colorMapType);
    goto exit;
  }

  if (info.idLength && fseek(fp, info.idLength, SEEK_CUR)) {
    cmsg(MLERR, "tgaRead: file '%s' is truncated or corrupted", fname);
  }
  p = ReadImage(fp, &info, fname);
  if (p != NULL) {
    im->width = info.width;
    im->height = info.height;
    im->bytes = info.bytes;
    im->data = p;
    ++ret;
  }
 exit:
  fclose(fp);
  return ret;
}

static void bgr2rgb(unsigned char *a, int length, int bytes) {
  int i;
  unsigned char *b, c;

  b = a + 2;
  for (i = length; i; --i) {
    c = *a;
    *a = *b;
    *b = c;
    a += bytes;
    b += bytes;
  }
}

static void upsample(unsigned char *p, int length, int alpha) {
  int destbytes, i;
  unsigned char *s, *d, s0, s1;

  destbytes = (alpha) ? 4 : 3;
  s = p + (length - 1) * 2;
  d = p + (length - 1) * destbytes;
  for (i = length; i; --i) {
    s0 = s[0], s1 = s[1];

    d[0] =  (s1 << 1) & 0xf8;
    d[0] += d[0] >> 5;

    d[1] =  (s1 << 6) + ((s0 >> 2) & 0x38);
    d[1] += d[1] >> 5;

    d[2] =  s1 << 3;
    d[2] += d[2] >> 5;

    if (alpha) d[3] = (s1 & 0x80) ? 255 : 0;

    d -= destbytes;
    s -= 2;
  }
}

static void unmap(unsigned char *p, int length, int alpha, unsigned char *cmap, int cms) {
  unsigned char *s, *d, *c;
  int i, j, destbytes;

  cms = (cms + 7) / 8;
  destbytes = cms;
  if (destbytes < 4 && alpha) destbytes = 4;
  s = p + length * (alpha ? 2 : 1) - 1;
  d = p + length * destbytes - 1;
  for (i = length; i; --i) {
    if (alpha) *d-- = *s--;
    c = cmap + (*s--) * cms;
    for (j = cms - 1; j >= 0; --j) *d-- = c[j];
  }
}

/* taken almost unchanged from tga.h of gimp */
static int rle_read(unsigned char *buf, int width, int bytes, FILE *fp) {
  static int repeat = 0;
  static int direct = 0;
  static unsigned char sample[4];
  int head;
  int x, k;

  if (buf == NULL) { /* init phase */
    repeat = 0;
    direct = 0;
    return 0;
  }
  for (x = width; x; --x) {
    if (!repeat && !direct) {
      head = getc(fp);
      if (head == EOF) {
        return head;
      } else if (head >= 128) {
        repeat = head - 127;
        if (fread(sample, bytes, 1, fp) < 1) return EOF;
      } else {
        direct = head + 1;
      }
    }
    if (repeat > 0) {
      for (k = 0; k < bytes; ++k) {
        buf[k] = sample[k];
      }
      --repeat;
    } else { /* direct > 0 */
      if (fread(buf, bytes, 1, fp) < 1) return EOF;
      --direct;
    }
    buf += bytes;
  }
  return 0;
}

static void ReadLine(FILE *fp, tga_info_t *info, unsigned char *row, unsigned char *cmap) {
  switch (info->imageCompression) {
    case TGA_COMP_NONE:
      fread(row, info->width, info->bytes, fp);
      break;
    case TGA_COMP_RLE:
      rle_read(row, info->width, info->bytes, fp);
      break;
  }
  if (info->flipHoriz) {
    /* TODO: flip line */
  }
  switch (info->imageType) {
    case TGA_TYPE_MAPPED:
      unmap(row, info->width, info->alphaBits, cmap, info->colorMapSize);
      break;
    case TGA_TYPE_COLOR:
      if (info->bpp == 15 || info->bpp == 16) {
        upsample(row, info->width, info->alphaBits);
      } else {
        bgr2rgb(row, info->width, info->bytes);
      }
      break;
    case TGA_TYPE_GRAY:
      /* gray images can go */
      break;
  }
}

static void *ReadImage(FILE *fp, tga_info_t *info, const char *fname) {
  int destbytes;
  int cmap_bytes;
  unsigned char cmap[4 * 256];
  unsigned char *img, *row;
  int i;

  if (info->colorMapType == 1) {
    cmap_bytes = (info->colorMapSize + 7) / 8;
    if (cmap_bytes > 4 || fread(cmap, info->colorMapLength * cmap_bytes, 1, fp) != 1) {
      cmsg(MLERR, "unable to read colormap from '%s'", fname);
      return NULL;
    }
    switch (info->colorMapSize) {
      case 32:
      case 24:
        bgr2rgb(cmap, info->colorMapLength, cmap_bytes);
        break;
      case 16:
      case 15:
        upsample(cmap, info->colorMapLength, 0/*info->alphaBits*/);
        info->colorMapSize = /*(info->alphaBits) ? 4 :*/ 3;
        break;
    }
  }

  switch (info->imageType) {
    case TGA_TYPE_MAPPED:
      destbytes = (info->colorMapSize + 7) / 8;
      if (info->alphaBits) destbytes += 1;
//      printf("cms %d %d %d\n", destbytes, info->colorMapSize, info->alphaBits);
      break;
    case TGA_TYPE_COLOR:
    case TGA_TYPE_GRAY:
      destbytes = info->bytes;
      break;
    default:
      cmsg(MLERR, "unknown image type for '%s'", fname);
      return NULL;
  }
  img = mmAlloc(info->width * info->height * destbytes);
  if (img == NULL) return img;

  if (info->imageCompression == TGA_COMP_RLE) rle_read(NULL, 0, 0, NULL);
  if (info->flipVert) {
    row = img + info->width * (info->height - 1) * destbytes;
    for (i = info->height; i; --i) {
      ReadLine(fp, info, row, cmap);
      row -= info->width * destbytes;
    }
  } else {
    row = img;
    for (i = info->height; i; --i) {
      ReadLine(fp, info, row, cmap);
      row += info->width * destbytes;
    }
  }

  info->bytes = destbytes;
  return img;
}

void tgaFree(tgaimage_t *im) {
  if (im == NULL) return;
  if (im->data != NULL) mmFree(im->data);
  memset(im, 0, sizeof(tgaimage_t));
  im->data = NULL;
}

/* dest size must be smaller that src - current algo reqs this */
int tgaScale(tgaimage_t *d, tgaimage_t *s) {
  if (d == NULL || s == NULL) return 0;
  if (!d->width || !d->height) return 0;
  if (!s->width || !s->height || !s->bytes) return 0;
  if (s->data == NULL) return 0;
  if (d->data != NULL) mmFree(d->data);
  unsigned size = d->width * d->height * s->bytes;
  d->data = (unsigned char *)mmAlloc(size);
  if (d->data == NULL) return 0;
  d->bytes = s->bytes;
  int x, y, x1, y1, x2, y2, bx, by;
  int dx = 65536 * s->width / d->width;
  int dy = 65536 * s->height / d->height;
  int sx, sy = dy;
  unsigned char *p = d->data;
  int c, n;
  y2 = 0;
  for (y = d->height; y; --y, sy += dy) {
    y1 = y2;
    y2 = sy / 65536;
    sx = dx;
    x2 = 0;
    for (x = d->width; x; --x, sx += dx) {
      x1 = x2;
      x2 = sx / 65536;
      for (unsigned k = 0; k < d->bytes; ++k) {
        n = c = 0;
        for (by = y1; by < y2; ++by)
          for (bx = x1; bx < x2; ++bx) {
            ++n;
            c += s->data[(by * s->width + bx) * s->bytes + k];
          }
        *p++ = c / n;
      }
    }
  }
  return !0;
}
