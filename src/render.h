#ifndef _RENDER_H
#define _RENDER_H 1

#define GET_TEXTURE(t, n) (((t) >> ((n) * 10)) & 0x3ff)

extern float curfov;

void rBuildFrame();
void rInit();
void rDone();

#endif /* _RENDER_H */
