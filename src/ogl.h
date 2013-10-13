#ifndef _OGL_H
#define _OGL_H 1

#include <SDL/SDL_opengl.h>

/* must be kept synched with ogl.c/extfuncs */
typedef struct {
  PFNGLACTIVETEXTUREARBPROC ActiveTextureARB;
  PFNGLMULTITEXCOORD2FARBPROC MultiTexCoord2fARB;
  void *ARB_texture_cube_map;
  void *ARB_texture_env_combine;
  void *ARB_texture_env_dot3;
} gl_t;

extern gl_t gl;

void oglInitProcs();

#endif /* _OGL_H */
