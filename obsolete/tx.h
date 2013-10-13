#ifndef _TX_H
#define _TX_H 1

typedef enum {
  TXF_KEEP = 0,
  TXF_UPPERCASE = 1,
  TXF_LOWERCASE = 2,
} textfilter_t;

void txInit();
void txDone();
void txSetHeight(float h);
void txSetWidth(float h);
float txGetHeight();
void txSetColor(unsigned c);
void txPutChar(float x, float y, unsigned char ch);
void txPutStr(float x, float y, const char *p);
void txPutChar8(float x, float y, unsigned char ch);
void txPutStr8(float x, float y, const char *p);
float txGetStrWidth(const char *p);
void txSetFilter(textfilter_t filter);

#endif /* _TX_H */
