#include <math.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "tga.h"
#include "tx.h"
#include "main.h"
#include "con_cmd.h"
#include "console.h"
#include "tex.h"
#include "mm.h"
//#include "bsp_norm.h"
#include "input.h"

static double FOV = 90.0, curfov = 90.0;
static int bspReady = 0;
int bspIsReady() { return bspReady; }

typedef struct {
  double x, y, z;
} vec3d_t;

typedef struct {
  double x, y;
} vertex_t;

typedef struct {
  short f : 16;
  short c : 16;
  unsigned char l : 8;
  short u : 16;
  short v : 16;
  unsigned t : 32;
} __attribute__((packed)) sector_t;

struct node_s;

typedef enum {
  LF_NOTHING		= 0x0000,
  LF_TWOSIDED		= 0x0001,
  LF_TOPSTART		= 0x0002,
  LF_BOTTOMSTART	= 0x0004,
  LF_BLOCK		= 0x0008,
} lineflag_t;

typedef struct {
  unsigned a, b;
  double u1, u2, v;
  struct node_s *nn;
  lineflag_t flags;
  double nx, ny;
  unsigned t;
} line_t;

static struct {
  vertex_t *p;
  unsigned n;
} vc;

static struct {
  sector_t *p;
  unsigned n;
} sc;

typedef struct {
  double x1, y1, z1;
  double x2, y2, z2;
} bbox_t;

typedef enum {
  NF_NOTHING	= 0x00,
  NF_VISIBLE	= 0x01,
} nodeflag_t;

typedef struct node_s {
  line_t *p;
  unsigned n;
  bbox_t bb;
  sector_t *s;
  int cx, cy;
  struct node_s *l, *r, *ow;
  nodeflag_t flags;
} node_t;

typedef struct {
  unsigned dist;
  node_t *n;
} nodptr_t;

static node_t *root = NULL, *cn = NULL;
static int onfloor = 0;
static int r_clear = 1;
static int r_drawwalls = 1;
static int cheats = 0;

#define PI 3.141592654

struct {
  double x, y, z, dx, dy, dz;
  double d2x, d2y;
  double vx, vy, vz;
  double a, da;
  double b;
  vec3d_t cps[4];
} cam;

static void bspRotateCam(double a, double b) {
  a += cam.a;
  while (a >= 2 * PI) a -= 2 * PI;
  while (a < 0.0) a += 2 * PI;
  b += cam.b;
  if (b > PI / 2) b = PI / 2;
  if (b < -PI / 2) b = -PI / 2;
  cam.a = a;
  cam.b = b;
  cam.d2x = sin(a);
  cam.d2y = cos(a);
  cam.dx = cam.d2x * cos(b);
  cam.dy = cam.d2y * cos(b);
  cam.dz = -sin(b);
  vec3d_t v[4];
  double beta, fov2 = curfov / 360.0 * PI;
  beta = b - fov2;
  v[0].x = cos(beta) * sin(a) + sin(-fov2) * cos(a);
  v[0].y = cos(beta) * cos(a) - sin(-fov2) * sin(a);
  v[0].z = v[1].z = -sin(beta);
  v[1].x = cos(beta) * sin(a) + sin(fov2) * cos(a);
  v[1].y = cos(beta) * cos(a) - sin(fov2) * sin(a);
  beta = b + fov2;
  v[2].x = cos(beta) * sin(a) + sin(fov2) * cos(a);
  v[2].y = cos(beta) * cos(a) - sin(fov2) * sin(a);
  v[2].z = v[3].z = -sin(beta);
  v[3].x = cos(beta) * sin(a) + sin(-fov2) * cos(a);
  v[3].y = cos(beta) * cos(a) - sin(-fov2) * sin(a);

  cam.cps[0].x = v[0].y * v[1].z - v[0].z * v[1].y;
  cam.cps[0].y = v[0].z * v[1].x - v[0].x * v[1].z;
  cam.cps[0].z = v[0].x * v[1].y - v[0].y * v[1].x;
  cam.cps[1].x = v[1].y * v[2].z - v[1].z * v[2].y;
  cam.cps[1].y = v[1].z * v[2].x - v[1].x * v[2].z;
  cam.cps[1].z = v[1].x * v[2].y - v[1].y * v[2].x;
  cam.cps[2].x = v[2].y * v[3].z - v[2].z * v[3].y;
  cam.cps[2].y = v[2].z * v[3].x - v[2].x * v[3].z;
  cam.cps[2].z = v[2].x * v[3].y - v[2].y * v[3].x;
  cam.cps[3].x = v[3].y * v[0].z - v[3].z * v[0].y;
  cam.cps[3].y = v[3].z * v[0].x - v[3].x * v[0].z;
  cam.cps[3].z = v[3].x * v[0].y - v[3].y * v[0].x;
}

static int bspBBoxVisible(bbox_t *bb) {
  vec3d_t p[8];
  p[0].x = p[3].x = p[4].x = p[7].x = bb->x1 - cam.x;
  p[1].x = p[2].x = p[5].x = p[6].x = bb->x2 - cam.x;
  p[0].y = p[1].y = p[4].y = p[5].y = bb->y1 - cam.y;
  p[2].y = p[3].y = p[6].y = p[7].y = bb->y2 - cam.y;
  p[4].z = p[5].z = p[6].z = p[7].z = bb->z1 - cam.z;
  p[0].z = p[1].z = p[2].z = p[3].z = bb->z2 - cam.z;

  int i, j, k;
  double x, y, z;
  for (i = 0; i < 5; ++i) {
    switch (i) {
      case 4:
        x = -cam.dx;
        y = -cam.dy;
        z = -cam.dz;
        break;
      default:
        x = -cam.cps[i].x;
        y = -cam.cps[i].y;
        z = -cam.cps[i].z;
        break;
    }
    k = 8;
    for (j = 0; j < 8; ++j)
      if (p[j].x * x + p[j].y * y + p[j].z * z > 0) --k;
    if (!k) return 0;
  }
  return !0;
}

static int bspBBoxInside(bbox_t *bb, double x, double y, double z, double b) {
  return (
    x >= bb->x1 - b && x <= bb->x2 + b &&
    y >= bb->y1 - b && y <= bb->y2 + b &&
    z >= bb->z1 - b && z <= bb->z2 + b
  );
}

static double dabs(double a) {
  return (a < 0) ? -a : a;
}

static double pszt(vertex_t p1, vertex_t p2, vertex_t p3, vertex_t *p4) {
  double dx21, dy21;
  if (p1.x > p2.x) {
    vertex_t p = p1;
    p1 = p2;
    p2 = p;
  }
  dx21 = p2.x - p1.x;
  dy21 = p2.y - p1.y;
  double x = p3.y - p1.y;
  x *= dx21 * dy21;
  x += p1.x * dy21 * dy21 + p3.x * dx21 * dx21;
  x /= dy21 * dy21 + dx21 * dx21;
  double y;
  if (x < p1.x) {
    x = p1.x;
    y = p1.y;
  } else if (x > p2.x) {
    x = p2.x;
    y = p2.y;
  } else {
    if (dabs(dx21) > dabs(dy21)) {
      if (dx21)
        y = p1.y + (x - p1.x) * dy21 / dx21;
      else
        return 0;
    } else if (dy21)
      y = p3.y - (x - p3.x) * dx21 / dy21;
    else
      return 0;
    if (p1.y > p2.y) {
      vertex_t p = p1;
      p1 = p2;
      p2 = p;
    }
    if (y < p1.y) {
      y = p1.y;
      x = p1.x;
    } else if (y > p2.y) {
      y = p2.y;
      x = p2.x;
    }
  }
  dx21 = p3.x - x;
  dy21 = p3.y - y;
  if (p4 != NULL) {
    p4->x = dx21;
    p4->y = dy21;
  }
  return dx21 * dx21 + dy21 * dy21;
}

double at(double y, double x) {
  if (x > 0)
    return atan(y / x);
  else if (x < 0)
    return ((y > 0) ? PI : -PI) - atan(y / -x);
  else
    return (y > 0) ? PI/2 : -PI/2;
}

static void bspTraceTreeForVisibles() {
  nodeflag_t vis = NF_NOTHING;
  sector_t *s, *ns;
  int f, c;
  vertex_t a, b;
  double dx, dy;

  void bspSetVisible(node_t *n) {
    n->flags = (n->flags & ~NF_VISIBLE) | (vis & NF_VISIBLE);
    if (n->l != NULL) bspSetVisible(n->l);
    if (n->r != NULL) bspSetVisible(n->r);
  }

  int bspSeeThrough(node_t *n, int i) {
    s = n->s;
    ns = n->p[i].nn->s;
    if (s == NULL || ns == NULL) return 0;
    f = s->f;
    if (ns->f > f) f = ns->f;
    c = s->c;
    if (ns->c < c) c = ns->c;
    return f < c;
  }

  void bspTraceNode(node_t *n) {
    n->flags |= NF_VISIBLE;
    int i; /* lokalis bazmeg!! */
    for (i = 0; i < n->n; ++i) {
      a = vc.p[n->p[i].a];
      b = vc.p[n->p[i].b];
      a.x -= cam.x; a.y -= cam.y;
      b.x -= cam.x; b.y -= cam.y;
      dx = b.x - a.x;
      dy = b.y - a.y;
      if (dx * a.y > dy * a.x) continue;
      if (n->p[i].nn != NULL && bspSeeThrough(n, i) && !(n->p[i].nn->flags & NF_VISIBLE))
        bspTraceNode(n->p[i].nn);
    }
  }

  void bspSearchOutsiders(node_t *n) {
    if (n->s != NULL && n->s->f < n->s->c) return;
    if (n->l != NULL) bspSearchOutsiders(n->l);
    if (n->r != NULL) bspSearchOutsiders(n->r);
    if (n->ow != NULL && n->ow->flags & NF_VISIBLE) n->flags |= NF_VISIBLE;
  }

  void bspSpreadVisibility(node_t *n) {
    if (n->l != NULL) bspSpreadVisibility(n->l);
    if (n->r != NULL) bspSpreadVisibility(n->r);
    if ((n->l != NULL && (n->l->flags & NF_VISIBLE)) ||
        (n->r != NULL && (n->r->flags & NF_VISIBLE))) {
      n->flags |= NF_VISIBLE;
    }
  }

  if (root == NULL) return;
  if (cn == NULL) vis = NF_VISIBLE;
  bspSetVisible(root);
  if (vis & NF_VISIBLE) return;
  bspTraceNode(cn);
  /* spread visibility upwards in the tree and search outsiders */
  bspSpreadVisibility(root);
  bspSearchOutsiders(root); /* requires SpreadVisibility */
}

#define GET_TEXTURE(t, n) (((t) >> ((n) * 10)) & 0x3ff)

static int visfaces, visnodes;

static void bspDrawNodes() {
  unsigned i, t;
  double x, y, u, v;
  line_t *l;
  vertex_t *a, *b;
  sector_t *ns;
  int ps;

  void bspDrawWall(vertex_t *a, vertex_t *b, double f, double c, line_t *l) {
    v = (l->v - c) * 0.015625;
    texSelectTexture(t);
/*    glActiveTexture(GL_TEXTURE1);
    texSelectTexture(0xc);
    glActiveTexture(GL_TEXTURE0);*/
    glBegin(GL_QUADS);
    glNormal3d(l->nx, l->nx, 0.0);
    glTexCoord2d(l->u1, v);
    glMultiTexCoord2d(GL_TEXTURE1, 0.0, 0.0);
    glVertex3d(a->x, a->y, c);
    glTexCoord2d(l->u2, v);
    glMultiTexCoord2d(GL_TEXTURE1, 1.0, 0.0);
    glVertex3d(b->x, b->y, c);
    v = (l->v - f) * 0.015625;
    glTexCoord2d(l->u2, v);
    glMultiTexCoord2d(GL_TEXTURE1, 1.0, 1.0);
    glVertex3d(b->x, b->y, f);
    glTexCoord2d(l->u1, v);
    glMultiTexCoord2d(GL_TEXTURE1, 0.0, 1.0);
    glVertex3d(a->x, a->y, f);
    glEnd();
    ++visfaces;
  }

  void bspDrawWalls(node_t *n) {
    for (i = n->n, l = n->p; i; --i, ++l) {
      a = vc.p + l->a;
      b = vc.p + l->b;
      ns = (l->nn != NULL) ? l->nn->s : NULL;
      if (n->s->f < n->s->c) {
        if ((b->x - a->x) * (b->y - cam.y) > (b->y - a->y) * (b->x - cam.x)) continue;
        if (ns != NULL) {
          if (ns->f > n->s->f && (t = GET_TEXTURE(l->t, 0))) {
            bspDrawWall(a, b, n->s->f, ns->f, l);
          }
          if (ns->c < n->s->c && (t = GET_TEXTURE(l->t, 2))) {
            bspDrawWall(a, b, ns->c, n->s->c, l);
          }
        }
        t = GET_TEXTURE(l->t, 1);
        if (t) {
          x = n->s->f;
          y = n->s->c;
          if (ns != NULL) {
            if (ns->f > x) x = ns->f;
            if (ns->c < y) y = ns->c;
          }
          bspDrawWall(a, b, x, y, l);
        }
      } else {
        if ((b->x - a->x) * (b->y - cam.y) < (b->y - a->y) * (b->x - cam.x)) continue;
        if (ns != NULL) {
          if (ns->c > n->s->c && (t = GET_TEXTURE(l->t, 0))) {
            bspDrawWall(b, a, n->s->c, ns->c, l);
          }
          if (ns->f < n->s->f && (t = GET_TEXTURE(l->t, 2))) {
            bspDrawWall(b, a, ns->f, n->s->f, l);
          }
        }
        t = GET_TEXTURE(l->t, 1);
        if (t) {
          x = n->s->c;
          y = n->s->f;
          if (ns != NULL) {
            if (ns->c > x) x = ns->c;
            if (ns->f < y) y = ns->f;
          }
          bspDrawWall(b, a, x, y, l);
        }
      }
    }
  }

  void bspDrawPlanes(node_t *n) {
    if (n->n < 3) return;
    if (n->s->f < cam.z && (t = GET_TEXTURE(n->s->t, 0))) {
      texSelectTexture(t);
      i = 0;
      if (n->s->f < n->s->c)
        for (; i < n->n; ++i)
          if (n->p[i].nn != NULL && n->p[i].nn->s->f != n->s->f) break;
      if (i == n->n && !ps) {
        ++ps;
        glEnable(GL_POLYGON_SMOOTH);
      } else if (i != n->n && ps) {
        glDisable(GL_POLYGON_SMOOTH);
        ps = 0;
      }
      glBegin(GL_POLYGON);
      glNormal3d(0.0, 0.0, 1.0);
      for (i = n->n, l = n->p; i; --i, ++l) {
        x = vc.p[l->a].x;
        y = vc.p[l->a].y;
        u = (x - n->bb.x1) / (n->bb.x2 - n->bb.x1);
        v = (y - n->bb.y1) / (n->bb.y2 - n->bb.y1);
        glTexCoord2d((x + n->s->u) * 0.015625, (y + n->s->v) * 0.015625);
        glMultiTexCoord2d(GL_TEXTURE1, u, v);
        glVertex3d(x, y, n->s->f);
      }
      glEnd();
      ++visfaces;
    }
    if (n->s->c > cam.z && (t = GET_TEXTURE(n->s->t, 1))) {
      texSelectTexture(t);
      i = 0;
      if (n->s->f < n->s->c)
        for (; i < n->n; ++i)
          if (n->p[i].nn != NULL && n->p[i].nn->s->c != n->s->c) break;
      if (i == n->n && !ps) {
        ++ps;
        glEnable(GL_POLYGON_SMOOTH);
      } else if (i != n->n && ps) {
        glDisable(GL_POLYGON_SMOOTH);
        ps = 0;
      }
      glBegin(GL_POLYGON);
      glNormal3d(0.0, 0.0, -1.0);
      for (i = n->n, l = n->p + n->n - 1; i; --i, --l) {
        x = vc.p[l->a].x;
        y = vc.p[l->a].y;
        u = (x - n->bb.x1) / (n->bb.x2 - n->bb.x1);
        v = (y - n->bb.y1) / (n->bb.y2 - n->bb.y1);
        glTexCoord2d((x + n->s->u) * 0.015625, (y + n->s->v) * 0.015625);
        glMultiTexCoord2d(GL_TEXTURE1, u, v);
        glVertex3d(x, y, n->s->c);
      }
      glEnd();
      ++visfaces;
    }
  }

  void bspDrawNode(node_t *n) {
    if (!bspBBoxVisible(&n->bb)) return;
    if (n->l != NULL) bspDrawNode(n->l);
    if (n->r != NULL) bspDrawNode(n->r);
    if (!(n->flags & NF_VISIBLE) || n->s == NULL || !n->n) return;
    glColor3ub(n->s->l, n->s->l, n->s->l);
//    glColor3d(1.0, 1.0, 1.0);
    texLoadTexture(0xc, 0);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    texSelectTexture(0xc);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_POLYGON_SMOOTH);
    if (r_drawwalls) bspDrawWalls(n);
    ps = 1;
    bspDrawPlanes(n);
    if (ps) glDisable(GL_POLYGON_SMOOTH);
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    ++visnodes;
  }

  if (root != NULL) bspDrawNode(root);
}

static void bspBBoxAdd(bbox_t *bb, int x, int y, int z) {
  if (x < bb->x1) bb->x1 = x;
  if (x > bb->x2) bb->x2 = x;
  if (y < bb->y1) bb->y1 = y;
  if (y > bb->y2) bb->y2 = y;
  if (z < bb->z1) bb->z1 = z;
  if (z > bb->z2) bb->z2 = z;
}

void bspBuildframe() {
  bspTraceTreeForVisibles();
  texUpdate();
  glEnable(GL_TEXTURE_2D);
/*  glEnable(GL_FOG);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFogf(GL_FOG_DENSITY, 0.1);
  glFogf(GL_FOG_START, 20);
  glFogf(GL_FOG_END, 100);*/
//  glHint(GL_FOG_HINT, GL_NICEST);
/*  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);*/
/*  glEnable(GL_NORMALIZE);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT1);
  float fv[4];
  fv[0] = fv[1] = fv[2] = 0.0, fv[3] = 1.0;
  glLightfv(GL_LIGHT1, GL_POSITION, fv);
  fv[3] = 5.0;
  fv[2] = fv[1] = fv[0] = 3.0;
  glLightfv(GL_LIGHT1, GL_AMBIENT, fv);
  fv[2] = fv[1] = fv[0] = 1.0;
  glLightfv(GL_LIGHT1, GL_DIFFUSE, fv);
  fv[2] = fv[1] = fv[0] = 0.0;
  glLightfv(GL_LIGHT1, GL_SPECULAR, fv);
  glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.01);*/
//  glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0001);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  double d = tan(curfov / 360.0 * PI);
  glFrustum(-d, d, -d, d, 1.0, 10000.0);
  glRotated(270.0 + cam.b / PI * 180.0, 1.0, 0.0, 0.0);
  glRotated(cam.a / PI * 180.0, 0.0, 0.0, 1.0);
  glTranslated(-cam.x, -cam.y, -cam.z);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.0);
  GLbitfield clear = GL_DEPTH_BUFFER_BIT;
  if (r_clear) {
    clear |= GL_COLOR_BUFFER_BIT;
    glClearColor(1.0, 0.5, 1.0, 0.0);
  }
  glClear(clear);
  visnodes = visfaces = 0;
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  bspDrawNodes();
  glDisable(GL_DEPTH_TEST);
/*  glDisable(GL_LIGHT1);
  glDisable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);*/
//  glDisable(GL_FOG);
  glDisable(GL_TEXTURE_2D);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  dmsg(MLDBG, "%d nodes", visnodes);
  dmsg(MLDBG, "%d faces", visfaces);
}

static void bspCollideTree(double x, double y, double z, double *dx, double *dy, double *dz, int hard) {
  unsigned i, j = 0;
  vertex_t p, f;
  double q, pz;
  line_t *l;
  double dx1, dy1, dx2, dy2;
  sector_t *ns;
  int front, in;

  void bspCollideNode(node_t *n) {
    if (n == NULL || !bspBBoxInside(&n->bb, x, y, z, 48.0)) return;
    bspCollideNode(n->l);
    bspCollideNode(n->r);
    if (n->s == NULL || !n->n) return;
    p.x = x + *dx;
    p.y = y + *dy;
    pz  = z + *dz;
    in = 1;
    for (i = n->n, l = n->p; i; --i, ++l) {
      dx1 = vc.p[l->b].x - vc.p[l->a].x;
      dy1 = vc.p[l->b].y - vc.p[l->a].y;
      dx2 = vc.p[l->b].x - x;
      dy2 = vc.p[l->b].y - y;
      front = dx1 * dy2 < dy1 * dx2;
      if (!front) in = 0;
      if (n->s->f < n->s->c) {
        if (pz < n->s->f - 16 - 8 || pz > n->s->c + 48 + 8) continue;
      } else {
        if (pz < n->s->c - 16 - 8 || pz > n->s->f + 48 + 8) continue;
      }
      if (pszt(vc.p[l->a], vc.p[l->b], p, &f) > 256.0) continue;
      in = 0;
      if (pz > n->s->f && pz < n->s->f + 48) {
        *dz = (n->s->f + 48 - z) * 0.1;
        pz  = z + *dz;
        onfloor = 1;
      }
      if (pz < n->s->c && pz > n->s->c - 16) {
        *dz = (n->s->c - 16 - z) * 0.1;
        pz  = z + *dz;
      }
      ns = (l->nn != NULL) ? l->nn->s : NULL;
      if (ns != NULL) {
        if (n->s->f < n->s->c) {
          dx1 = n->s->f;
          if (ns->f > dx1) dx1 = ns->f;
          dx2 = n->s->c;
          if (ns->c < dx2) dx2 = ns->c;
          if (dx2 - dx1 > 64 && ns->f < pz - 48 + 24 && ns->c > pz + 16) continue;
        } else {
          if (ns->f <= pz - 48 + 24 || ns->c >= pz + 16) continue;
        }
      } else if (n->s->f > n->s->c) {
        if (pz > n->s->f + 48 - 8 || pz < n->s->c - 16 + 8) continue;
      }
      q = sqrt(f.x * f.x + f.y * f.y);
      f.x /= q;  f.y /= q;
      if (hard) {
        if (j) {
          *dx += f.x;
          *dy += f.y;
        } else {
          *dx = f.x;
          *dy = f.y;
        }
        ++j;
        continue;
      }
      q = cos(at(f.x, -f.y) - at(*dy, *dx)) * sqrt(*dx * *dx + *dy * *dy);
      *dx = -f.y * q;
      *dy = f.x * q;
    }
    if (in) {
      if (pz > n->s->f && pz < n->s->f + 48) {
        *dz = (n->s->f + 48 - z) * 0.1;
        pz  = z + *dz;
        onfloor = 1;
      }
      if (pz < n->s->c && pz > n->s->c - 16) {
        *dz = (n->s->c - 16 - z) * 0.1;
        pz  = z + *dz;
      }
    }
  }

  if (root != NULL) bspCollideNode(root);
}

static node_t *bspGetNodeForCoords(double x, double y, double z) {
  node_t *nod = NULL;
  int bo, i;
  line_t *l;
  vertex_t *a, *b;
  double dx1, dy1, dx2, dy2;

  void bspGetNodeForCoordsSub(node_t *n) {
    if (n->l == NULL && n->r == NULL && n->n && n->s->f < n->s->c &&
        bspBBoxInside(&n->bb, x, y, z, 0.0)) {
      bo = 0;
      b = vc.p + n->p[0].a;
      for (i = n->n, l = n->p; i; --i, ++l) {
        a = b;
        b = vc.p + l->b;
        dx1 = b->x - a->x;
        dy1 = b->y - a->y;
        dx2 = b->x - x;
        dy2 = b->y - y;
        if (dx1 * dy2 > dy1 * dx2) {
          ++bo;
          break;
        }
      }
      if (!bo) {
        nod = n;
        return;
      }
    }
    if (n->l != NULL) bspGetNodeForCoordsSub(n->l);
    if (nod != NULL) return;
    if (n->r != NULL) bspGetNodeForCoordsSub(n->r);
  }

  if (root != NULL) bspGetNodeForCoordsSub(root);
  return nod;
}

#define CAM_DA_MIN (PI/3000)
#define CAM_DA_MAX (PI/100)
#define CAM_DA_INC (PI/1200)
#define CAM_DA_DEC (PI/2000)
#define CAM_V_MIN 0.001
#define CAM_V_MAX 3.0
#define CAM_V_INC 0.2
#define CAM_V_DEC 0.05
#define FALL_V_MAX 10.0
#define FALL_V_INC 0.2
#define FOVINC 10.0
#define FOVDEC 20.0

static int clip = 1;
static int gravity = 1;
static double zoomfov = 25.0;
static double mouse_sensitivity = 0.004;

static int cmd_move(int argc, char **argv) {
  if (argc < 2) {
    cmsg(MLINFO, "usage: %s <forward|back|left|right|up|down>", *argv);
    return !0;
  }
  if (conGetState() != CON_INACTIVE) return 0;
  if (!strcmp(argv[1], "forward")) {
    if (gravity) {
      cam.vx += cam.d2x * CAM_V_INC;
      cam.vy += cam.d2y * CAM_V_INC;
    } else {
      cam.vx += cam.dx * CAM_V_INC;
      cam.vy += cam.dy * CAM_V_INC;
      cam.vz += cam.dz * CAM_V_INC;
    }
  } else if (!strcmp(argv[1], "back")) {
    if (gravity) {
      cam.vx -= cam.d2x * CAM_V_INC;
      cam.vy -= cam.d2y * CAM_V_INC;
    } else {
      cam.vx -= cam.dx * CAM_V_INC;
      cam.vy -= cam.dy * CAM_V_INC;
      cam.vz -= cam.dz * CAM_V_INC;
    }
  } else if (!strcmp(argv[1], "left")) {
    cam.vx -= cam.d2y * CAM_V_INC;
    cam.vy += cam.d2x * CAM_V_INC;
  } else if (!strcmp(argv[1], "right")) {
    cam.vx += cam.d2y * CAM_V_INC;
    cam.vy -= cam.d2x * CAM_V_INC;
  } else if (!strcmp(argv[1], "up")) {
    if (onfloor) cam.vz = 4.0;
    if (!gravity) {
      double x, y, z;
      x = -cam.d2x * cam.dz;
      y = -cam.d2y * cam.dz;
      z = cam.d2y * cam.dy + cam.d2x * cam.dx;
      cam.vx += x * CAM_V_INC;
      cam.vy += y * CAM_V_INC;
      cam.vz += z * CAM_V_INC;
    }
  } else if (!strcmp(argv[1], "down")) {
    if (!gravity) {
      double x, y, z;
      x = -cam.d2x * cam.dz;
      y = -cam.d2y * cam.dz;
      z = cam.d2y * cam.dy + cam.d2x * cam.dx;
      cam.vx -= x * CAM_V_INC;
      cam.vy -= y * CAM_V_INC;
      cam.vz -= z * CAM_V_INC;
    }
  } else {
    cmsg(MLERR, "cmd_move: unknown direction: %s", argv[1]);
    return !0;
  }
  return 0;
}

static int cmd_turn(int argc, char **argv) {
  if (argc < 2) {
    cmsg(MLINFO, "usage: %s <left|right>", *argv);
    return !0;
  }
  if (conGetState() != CON_INACTIVE) return 0;
  if (!strcmp(argv[1], "left") && cam.da > -CAM_DA_MAX)
    cam.da -= CAM_DA_INC;
  else if (!strcmp(argv[1], "right") && cam.da < CAM_DA_MAX)
    cam.da += CAM_DA_INC;
  return 0;
}

static int cmd_zoom(int argc, char **argv) {
  if (conGetState() != CON_INACTIVE) return 0;
  curfov -= FOVDEC;
  return 0;
}

void bspSetupControls() {
  cmdAddCommand("move", cmd_move);
  cmdAddCommand("turn", cmd_turn);
  cmdAddCommand("zoom", cmd_zoom);
  cmdAddCommand("attack", cmd_null);
}

void bspKeyUpdate() {
  int mx, my;
  double l;
  SDL_GetRelativeMouseState(&mx, &my);
  if (gravity) {
    if (clip) cam.vz -= FALL_V_INC;
    l = sqrt(cam.vx * cam.vx + cam.vy * cam.vy);
    if (l > CAM_V_MAX) {
      l = 1 / l;
      cam.vx *= l;
      cam.vy *= l;
      l = CAM_V_MAX;
    }
  } else {
    l = sqrt(cam.vx * cam.vx + cam.vy * cam.vy + cam.vz * cam.vz);
    if (l > CAM_V_MAX) {
      l = 1 / l;
      cam.vx *= l;
      cam.vy *= l;
      cam.vz *= l;
      l = CAM_V_MAX;
    }
  }
  if (cam.vz < -FALL_V_MAX) cam.vz = -FALL_V_MAX;
  if (cam.vz > FALL_V_MAX) cam.vz = FALL_V_MAX;
  onfloor = 0;
  if (clip) {
    bspCollideTree(cam.x, cam.y, cam.z, &cam.vx, &cam.vy, &cam.vz, 0);
    bspCollideTree(cam.x, cam.y, cam.z, &cam.vx, &cam.vy, &cam.vz, 1);
  }
  cam.x += cam.vx;
  cam.y += cam.vy;
  cam.z += cam.vz;
  cam.a += cam.da;
  curfov += FOVINC;
  if (curfov > FOV)
    curfov = FOV;
  else if (curfov < zoomfov)
    curfov = zoomfov;
  if (l > CAM_V_MIN) {
    cam.vx -= CAM_V_DEC * cam.vx;
    cam.vy -= CAM_V_DEC * cam.vy;
    if (!gravity) cam.vz -= CAM_V_DEC * cam.vz;
  } else {
    cam.vx = cam.vy = 0.0;
    if (!gravity) cam.vz = 0.0;
  }
  if (cam.da > CAM_DA_MIN)
    cam.da -= CAM_DA_DEC;
  else if (cam.da < -CAM_DA_MIN)
    cam.da += CAM_DA_DEC;
  else
    cam.da = 0;
  bspRotateCam(mouse_sensitivity * mx, mouse_sensitivity * my);
  cn = bspGetNodeForCoords(cam.x, cam.y, cam.z);
}

node_t *bspGetNodeForLine(unsigned a, unsigned b) {
  node_t *nod = NULL;
  int i;

  void bspGetNodeForLineSub(node_t *n) {
    for (i = 0; i < n->n; ++i)
      if ((n->p[i].flags & LF_TWOSIDED) && n->p[i].a == a && n->p[i].b == b) {
        nod = n;
        return;
      }
    if (n->l != NULL) bspGetNodeForLineSub(n->l);
    if (nod != NULL) return;
    if (n->r != NULL) bspGetNodeForLineSub(n->r);
  }

  if (root != NULL) bspGetNodeForLineSub(root);
  return nod;
}

typedef struct {
  short x : 16;
  short y : 16;
} __attribute__((packed)) fvertex_t;

typedef struct {
  short a : 16;
  short b : 16;
  unsigned char flags : 8;
  unsigned short u1 : 16;
  unsigned short u2 : 16;
  unsigned short v : 16;
  unsigned int t : 32;
} __attribute__((packed)) fline_t;

static line_t *linepool = NULL;

void bspFreeTree() {
  if (linepool != NULL) {
    mmFree(linepool);
    linepool = NULL;
  }
  if (root != NULL) {
    mmFree(root);
    root = NULL;
  }
  if (vc.p != NULL) {
    mmFree(vc.p);
    vc.p = NULL;
  }
  vc.n = 0;
}

int bspLoadTree(FILE *f) {
  fvertex_t fv;
  fline_t fl;
  int i, j;
  int rn, rl;
  int err = 0;
  node_t *np;
  line_t *lp;
  double x, y, l;

  node_t *bspLoadNode() {
    if (!fread(&i, sizeof(int), 1, f)) { ++err; return NULL; }
    if (!i) return NULL;
    if (!rn) {
      cmsg(MLERR, "bspLoadNode: out of preallocated nodes");
      ++err;
      return NULL;
    }
    --rn;
    node_t *n = np++;
    n->n = i - 1;
    if (!fread(&i, sizeof(int), 1, f)) { ++err; return n; }
    if (i) {
      n->s = sc.p + i;
      texLoadTexture(GET_TEXTURE(n->s->t, 0), 0);
      texLoadTexture(GET_TEXTURE(n->s->t, 1), 0);
    } else
      n->s = NULL;
    n->p = NULL;
    if (n->n) {
      if (n->n > rl) {
        cmsg(MLERR, "bspLoadNode: out of preallocated lines");
        ++err;
        return n;
      }
      rl -= n->n;
      n->p = lp;
      lp += n->n;
      n->cx = n->cy = 0;
      x = y = 0;
      for (i = 0; i < n->n; ++i) {
        if (!fread(&fl, sizeof(fline_t), 1, f)) { ++err; return n; }
        n->p[i].a = fl.a;
        n->p[i].b = fl.b;
        n->p[i].u1 = (double)fl.u1 * 0.015625;
        n->p[i].u2 = (double)fl.u2 * 0.015625;
        n->p[i].v = fl.v;
        n->p[i].flags = fl.flags;
        n->p[i].nn = NULL;
        n->p[i].t = fl.t;
        texLoadTexture(GET_TEXTURE(fl.t, 0), 0);
        texLoadTexture(GET_TEXTURE(fl.t, 1), 0);
        texLoadTexture(GET_TEXTURE(fl.t, 2), 0);
        x += vc.p[fl.a].x;
        y += vc.p[fl.a].y;
      }
      n->cx = x / n->n;
      n->cy = y / n->n;
      for (i = 0; i < n->n; ++i) {
        x = vc.p[n->p[i].a].y - vc.p[n->p[i].b].y;
        y = vc.p[n->p[i].b].x - vc.p[n->p[i].a].x;
        l = 1 / sqrt(x * x + y * y);
        n->p[i].nx = x * l;
        n->p[i].ny = y * l;
      }
    }
    n->l = bspLoadNode();
    n->r = bspLoadNode();
    n->ow = NULL;
    j = 0;
    if (n->l != NULL) {
      n->bb.x1 = n->l->bb.x1;
      n->bb.y1 = n->l->bb.y1;
      n->bb.z1 = n->l->bb.z1;
      n->bb.x2 = n->l->bb.x2;
      n->bb.y2 = n->l->bb.y2;
      n->bb.z2 = n->l->bb.z2;
      j = 1;
    } else if (n->r != NULL) {
      n->bb.x1 = n->r->bb.x1;
      n->bb.y1 = n->r->bb.y1;
      n->bb.z1 = n->r->bb.z1;
      n->bb.x2 = n->r->bb.x2;
      n->bb.y2 = n->r->bb.y2;
      n->bb.z2 = n->r->bb.z2;
      j = 2;
    } else if (n->n) {
      n->bb.x1 = n->bb.x2 = vc.p[n->p[0].a].x;
      n->bb.y1 = n->bb.y2 = vc.p[n->p[0].a].y;
      n->bb.z1 = n->bb.z2 = n->s->f;
      j = 2;
    }
    switch (j) {
      case 1:
        if (n->r != NULL) {
          bspBBoxAdd(&n->bb, n->r->bb.x1, n->r->bb.y1, n->r->bb.z1);
          bspBBoxAdd(&n->bb, n->r->bb.x2, n->r->bb.y2, n->r->bb.z2);
        }
        /* intentionally no break here */
      case 2:
        for (i = 0; i < n->n; ++i) {
          bspBBoxAdd(&n->bb, vc.p[n->p[i].a].x, vc.p[n->p[i].a].y, n->s->f);
          bspBBoxAdd(&n->bb, vc.p[n->p[i].a].x, vc.p[n->p[i].a].y, n->s->c);
        }
        break;
      default:
        cmsg(MLERR, "bspLoadNode: unable to initialize bounding boxes");
        break;
    }
    return n;
  }

  void bspNeighSub(node_t *n) {
    for (i = 0; i < n->n; ++i)
      if ((n->p[i].flags & LF_TWOSIDED) && n->p[i].nn == NULL)
        n->p[i].nn = bspGetNodeForLine(n->p[i].b, n->p[i].a);
    if (n->l != NULL) bspNeighSub(n->l);
    if (n->r != NULL) bspNeighSub(n->r);
  }

  node_t *bspGetContainerNode(node_t *m) {
    node_t *r = NULL;
    int i;

    void bspGetContSub(node_t *n) {
      if (n->s != NULL) {
        if (n->s->f > n->s->c) return;
        for (i = 0; i < m->n; ++i) {
          if (bspBBoxInside(&n->bb, vc.p[m->p[i].a].x, vc.p[m->p[i].a].y, m->s->c, 0.0) ||
              bspBBoxInside(&n->bb, vc.p[m->p[i].a].x, vc.p[m->p[i].a].y, m->s->f, 0.0)) {
            r = n;
            return;
          }
        }
      }
      if (n->l != NULL) bspGetContSub(n->l);
      if (r != NULL) return;
      if (n->r != NULL) bspGetContSub(n->r);
    }

    if (root != NULL) bspGetContSub(root);
    return r;
  }

  void bspSearchOutsiders(node_t *n) {
    if (n->s != NULL && n->s->f < n->s->c) return;
    if (n->l != NULL) bspSearchOutsiders(n->l);
    if (n->r != NULL) bspSearchOutsiders(n->r);
    if (n->n) {
      n->ow = bspGetContainerNode(n);
//      cmsg(MLINFO, "outsider! %d %d", n->ow->s - sc.p, n->ow->n);
    }
  }

  if (!fread(&vc.n, sizeof(fvertex_t), 1, f)) return 0;
  vc.p = (vertex_t *)mmAlloc(vc.n * sizeof(vertex_t));
  if (vc.p == NULL) return 0;
  for (i = 0; i < vc.n; ++i) {
    if (!fread(&fv, sizeof(fvertex_t), 1, f)) return 0;
    vc.p[i].x = fv.x;
    vc.p[i].y = fv.y;
  }
  cmsg(MLINFO, "%d verteces", vc.n);
  if (!fread(&rn, sizeof(int), 1, f)) return 0;
  if (!fread(&rl, sizeof(int), 1, f)) return 0;
  np = root = (node_t *)mmAlloc(rn * sizeof(node_t));
  if (root == NULL) return 0;
  lp = linepool = (line_t *)mmAlloc(rl * sizeof(line_t));
  if (linepool == NULL) return 0;
  bspLoadNode();
  cmsg(MLINFO, "%d nodes, %d lines", np - root, lp - linepool);
  cmsg(MLINFO, "%d textures", texGetNofTextures());
  bspNeighSub(root);
  bspSearchOutsiders(root);
  return !0;
}

void bspFreeMap() {
  cmsg(MLDBG, "bspFreeMap");
  bspReady = 0;
  bspFreeTree();
  if (sc.p != NULL) {
    mmFree(sc.p);
    sc.p = NULL;
  }
  sc.n = 0;
  texFlush();
}

int bspLoadMap(const char *fname) {
  int ret = 0;
  FILE *f = fopen(fname, "rb");
  if (f == NULL) {
    cmsg(MLERR, "bspLoadMap: unable to open file %s for reading", fname);
    return ret;
  }
  cmsg(MLINFO, "Loading map %s", fname);
  bspFreeMap();
  bspReady = 1;
  if (!fread(&sc.n, sizeof(int), 1, f)) goto end;
  sc.p = (sector_t *)mmAlloc(sc.n * sizeof(sector_t));
  if (sc.p == NULL) goto end;
  if (fread(sc.p, sizeof(sector_t), sc.n, f) != sc.n) goto end;
  cmsg(MLINFO, "%d sectors", sc.n);
  if (!bspLoadTree(f)) goto end;
  ++ret;
 end:
  fclose(f);
  if (!ret) {
    bspFreeMap();
    cmsg(MLERR, "Loading failed");
  } else {
    cmsg(MLINFO, "Map successfuly loaded");
  }
  texLoadReady();
  return ret;
}

static void bspFovSet(void *addr) {
  double *d = addr;
  if (*d > 120.0) {
    cmsg(MLWARN, "maximal value is 120.0, clamped");
    *d = 120.0;
  }
  if (*d < 10.0) {
    cmsg(MLWARN, "maximal value is 10.0, clamped");
    *d = 10.0;
  }
  bspRotateCam(0.0, 0.0);
}

int cmd_map(int argc, char **argv) {
  if (argc < 2) {
    cmsg(MLINFO, "usage: %s <mapname>", *argv);
    return !0;
  }
  cheats = 0;
  char s[64];
  snprintf(s, 63, "editor/%s.bsp", argv[1]);
  s[63] = '\0';
  if (!bspLoadMap(s)) return !0;
  if (!strcmp(*argv, "devmap")) ++cheats;
  return 0;
}

void bspInit() {
  sc.p = NULL;
  sc.n = 0;
  vc.p = NULL;
  vc.n = 0;

  cam.x = 0.0;
  cam.y = 0.0;
  cam.z = 0.0;
  cam.vx = 0.0;
  cam.vy = 0.0;
  cam.vz = 0.0;
  cam.a = 0.0;
  cam.da = 0.0;
  cam.b = 0.0;
  bspRotateCam(0.0, 0.0);
  cmdAddBool("clip", &clip, 1);
  cmdAddBool("gravity", &gravity, 1);
  cmdAddBool("r_clear", &r_clear, 1);
  cmdAddBool("r_drawwalls", &r_drawwalls, 1);
  cmdAddDouble("zoomfov", &zoomfov, 1);
  cmdSetAccessFuncs("zoomfov", NULL, bspFovSet);
  cmdAddDouble("fov", &FOV, 1);
  cmdSetAccessFuncs("fov", NULL, bspFovSet);
  cmdAddDouble("mouse_sensitivity", &mouse_sensitivity, 1);
  cmdAddCommand("map", cmd_map);
  cmdAddCommand("devmap", cmd_map);
}

void bspDone() {
  bspFreeMap();
}
