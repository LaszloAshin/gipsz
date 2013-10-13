#include <SDL/SDL_video.h>

#if !defined(OPENSTEP) && (defined(NeXT) || defined(NeXT_PDO))
#define OPENSTEP
#endif

#if defined(_WIN32) && !defined(__WIN32__) && !defined(__CYGWIN__)
#define __WIN32__
#endif

#if !defined(OPENSTEP) && (defined(__WIN32__) && !defined(__CYGWIN__))
#  if defined(_MSC_VER) && defined(BUILD_GL32) /* tag specify we're building mesa as a DLL */
#    define GLAPI __declspec(dllexport)
#  elif defined(_MSC_VER) && defined(_DLL) /* tag specifying we're building for DLL runtime support */
#    define GLAPI __declspec(dllimport)
#  else /* for use with static link lib build of Win32 edition only */
#    define GLAPI extern
#  endif /* _STATIC_MESA support */
#  define GLAPIENTRY __stdcall
#elif defined(__CYGWIN__) && defined(USE_OPENGL32) /* use native windows opengl32 */
#  define GLAPI extern
#  define GLAPIENTRY __stdcall
#else
/* non-Windows compilation */
#  define GLAPI extern
#  define GLAPIENTRY
#endif /* WIN32 / CYGWIN bracket */

/*
 * Datatypes
 */
typedef unsigned int	GLenum;
typedef unsigned char	GLboolean;
typedef unsigned int	GLbitfield;
typedef void		GLvoid;
typedef signed char	GLbyte;		/* 1-byte signed */
typedef short		GLshort;	/* 2-byte signed */
typedef int		GLint;		/* 4-byte signed */
typedef unsigned char	GLubyte;	/* 1-byte unsigned */
typedef unsigned short	GLushort;	/* 2-byte unsigned */
typedef unsigned int	GLuint;		/* 4-byte unsigned */
typedef int		GLsizei;	/* 4-byte signed */
typedef float		GLfloat;	/* single precision float */
typedef float		GLclampf;	/* single precision float in [0,1] */
typedef double		GLdouble;	/* double precision float */
typedef double		GLclampd;	/* double precision float in [0,1] */


void (GLAPIENTRY *glBegin)(GLenum mode);
void (GLAPIENTRY *glColor3b)(GLbyte red, GLbyte green, GLbyte blue);
void (GLAPIENTRY *glVertex2i)(GLint x, GLint y);
void (GLAPIENTRY *glVertex3f)(GLfloat x, GLfloat y, GLfloat z);
void (GLAPIENTRY *glEnd)(void);

typedef struct  {
  void **p;
  char *name;
} glFunc_t;

static glFunc_t glFuncs[] = {
  { &glBegin, "glBegin" },
  { &glColor3b, "glColor3b" },
  { &glVertex2i, "glVertex2i" },
  { &glVertex3f, "glVertex3f" },
  { &glEnd, "glEnd" },
  { NULL }
};

int init_opengl_api() {
  if (SDL_GL_LoadLibrary("libGL.so.1") < 0) return 0;
  unsigned i;
  for (i = 0; glFuncs[i].p != NULL; ++i) {
    void *p = SDL_GL_GetProcAddress(glFuncs[i].name);
    if (p == NULL) return 0;
    *glFuncs[i].p = p;
  }
  return !0;
}
