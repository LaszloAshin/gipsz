#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "ogl.h"
#include "console.h"

gl_t gl;

/* must be kept synched with ogl.h/gl_t */
static const struct {
  const char *ext;
  const char *func;
} extfuncs[] = {
  { "GL_ARB_multitexture", "glActiveTextureARB" },
  { "GL_ARB_multitexture", "glMultiTexCoord2fARB" },
  { "GL_ARB_texture_cube_map", NULL },
  { "GL_ARB_texture_env_combine", NULL },
  { "GL_ARB_texture_env_dot3", NULL },
  { 0, 0 }
};

static void oglDisableExt(const char *ext) {
  int i;
  void **p = reinterpret_cast<void**>(&gl);
  for (i = 0; extfuncs[i].ext != NULL; ++i, ++p)
    if (ext == extfuncs[i].ext) *p = NULL;
}

static void oglPrintExtUsage() {
  int i;
  const char *last = NULL;
  void **p = reinterpret_cast<void**>(&gl);

  cmsg(MLINFO, "detecting OpenGL extensions");
  for (i = 0; extfuncs[i].ext != NULL; ++i, ++p) {
    if (extfuncs[i].ext != last) {
      last = extfuncs[i].ext;
      if (*p != NULL) {
        cmsg(MLINFO, "using %s", last);
      } else {
        cmsg(MLWARN, "unable to find %s", last);
      }
    }
  }
}

void oglInitProcs() {
  int i;
  const char *exts = (char *)glGetString(GL_EXTENSIONS), *last = NULL;
  int lastvalid = 0;
  void **p = reinterpret_cast<void**>(&gl);
  for (i = 0; extfuncs[i].ext != NULL; ++i, ++p) {
    if (extfuncs[i].ext != last) {
      last = extfuncs[i].ext;
      const char *s = strstr(exts, last);
      lastvalid = s != NULL && s[strlen(last)] <= 32;
    }
    if (lastvalid) {
      if (extfuncs[i].func != NULL) {
        *p = SDL_GL_GetProcAddress(extfuncs[i].func);
        if (*p == NULL) {
          oglDisableExt(last);
          lastvalid = 0;
        }
      } else *p = NULL;
    } else *p = NULL;
  }
  oglPrintExtUsage();
}
