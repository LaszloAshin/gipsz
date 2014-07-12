#ifndef _TEX_H
#define _TEX_H

#include <SDL/SDL_opengl.h>

void texFreeTexture(unsigned id);
int texLoadTexture(unsigned id, int force);
int texSelectTexture(int id);
int texInit();
void texFlush();
void texDone();
void texLoadReady();
int texGetNofTextures();
void texUpdate();
void texImage(int lev, int intfmt, GLsizei w, GLsizei h, GLenum fmt, void *data, int b);

#endif /* _TEX_H */
