#ifndef _TX2_H
#define _TX2_H 1

int tx2Init();
void tx2Done();
void tx2PutStr(float x, float y, const char *s);
float tx2GetStrWidth(const char *s);
void tx2SetWidth(float w);
void tx2SetHeight(float h);
float tx2GetWidth(void);
float tx2GetHeight(void);
void tx2SetFixed(int f);

#endif /* _TX2_H */
