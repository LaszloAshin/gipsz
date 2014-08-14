/* vim: set ts=2 sw=8 tw=0 et :*/
#include <stdlib.h>
#include <math.h>
#include <SDL/SDL_opengl.h>
#include "render.h"
#include "bsp.h"
#include "bbox.h"
#include "player.h"
#include "tex.h"
#include "console.h"
#include "cmd.h"
#include "obj.h"

float curfov = 90.0;
static int r_clear = 0;
static int r_drawwalls = 1;

static void
rSetVisible(node_t *n, nodeflag_t vis)
{
  n->flags = nodeflag_t((n->flags & ~NF_VISIBLE) | (vis & NF_VISIBLE));
  if (n->l != NULL) rSetVisible(n->l, vis);
  if (n->r != NULL) rSetVisible(n->r, vis);
}

static void
rSearchOutsiders(node_t *n)
{
  if (n->s != NULL && n->s->f < n->s->c) return;
  if (n->l != NULL) rSearchOutsiders(n->l);
  if (n->r != NULL) rSearchOutsiders(n->r);
  if (n->ow != NULL && n->ow->flags & NF_VISIBLE) n->flags = nodeflag_t(n->flags | NF_VISIBLE);
}

static void
rSpreadVisibility(node_t *n)
{
  if (n->l != NULL) rSpreadVisibility(n->l);
  if (n->r != NULL) rSpreadVisibility(n->r);
  if ((n->l != NULL && (n->l->flags & NF_VISIBLE)) ||
      (n->r != NULL && (n->r->flags & NF_VISIBLE))) {
    n->flags = nodeflag_t(n->flags | NF_VISIBLE);
  }
}

static void rTraceTreeForVisibles() {
  nodeflag_t vis = NF_NOTHING;
  if (root == NULL) return;
  if (cn == NULL) vis = NF_VISIBLE;
  rSetVisible(root, vis);
  if (vis & NF_VISIBLE) return;
  /* spread visibility upwards in the tree and search outsiders */
  rSpreadVisibility(root);
  rSearchOutsiders(root); /* requires SpreadVisibility */
}

static int visfaces, visnodes;

static void
rDrawWall(vertex_t *a, vertex_t *b, float f, float c, line_t *l, unsigned t)
{
  float v = (l->v - c) * 0.015625;
  texSelectTexture(t);
/*  glActiveTexture(GL_TEXTURE1);
  texSelectTexture(0xc);
  glActiveTexture(GL_TEXTURE0);*/
  glBegin(GL_QUADS);
  glNormal3f(l->nx, l->ny, 0.0);
  glTexCoord2f(l->u1, v);
//  glMultiTexCoord2f(GL_TEXTURE1, 0.0, 0.0);
  glVertex3f(a->x, a->y, c);

  glTexCoord2f(l->u2, v);
//  glMultiTexCoord2f(GL_TEXTURE1, 1.0, 0.0);
  glVertex3f(b->x, b->y, c);
  v = (l->v - f) * 0.015625;
  glTexCoord2f(l->u2, v);
//  glMultiTexCoord2f(GL_TEXTURE1, 1.0, 1.0);
  glVertex3f(b->x, b->y, f);

  glTexCoord2f(l->u1, v);
//  glMultiTexCoord2f(GL_TEXTURE1, 0.0, 1.0);
  glVertex3f(a->x, a->y, f);
  glEnd();
  ++visfaces;
}

static void
rDrawWalls(node_t *n)
{
  for (line_t *l = n->p; l < n->p + n->n; ++l) {
    vertex_t *a = vc.p + l->a;
    vertex_t *b = vc.p + l->b;
    const sector_t* const ns = l->backSectorId ? (sc.p + l->backSectorId) : 0;
    unsigned t;
    if (n->s->f < n->s->c) {
      if ((b->x - a->x) * (b->y - cam.y) > (b->y - a->y) * (b->x - cam.x)) continue;
      if (ns != NULL) {
        if (ns->f > n->s->f && (t = GET_TEXTURE(l->t, 0))) {
          rDrawWall(a, b, n->s->f, ns->f, l, t);
        }
        if (ns->c < n->s->c && (t = GET_TEXTURE(l->t, 2))) {
          rDrawWall(a, b, ns->c, n->s->c, l, t);
        }
      }
      t = GET_TEXTURE(l->t, 1);
      if (t) {
        float x = n->s->f;
        float y = n->s->c;
        if (ns != NULL) {
          if (ns->f > x) x = ns->f;
          if (ns->c < y) y = ns->c;
        }
        rDrawWall(a, b, x, y, l, t);
      }
    } else {
      if ((b->x - a->x) * (b->y - cam.y) < (b->y - a->y) * (b->x - cam.x)) continue;
      if (ns != NULL) {
        if (ns->c > n->s->c && (t = GET_TEXTURE(l->t, 0))) {
          rDrawWall(b, a, n->s->c, ns->c, l, t);
        }
        if (ns->f < n->s->f && (t = GET_TEXTURE(l->t, 2))) {
          rDrawWall(b, a, ns->f, n->s->f, l, t);
        }
      }
      t = GET_TEXTURE(l->t, 1);
      if (t) {
        float x = n->s->c;
        float y = n->s->f;
        if (ns != NULL) {
          if (ns->c > x) x = ns->c;
          if (ns->f < y) y = ns->f;
        }
        rDrawWall(b, a, x, y, l, t);
      }
    }
  }
}

static void
rDrawPlanes(node_t *n)
{
  if (n->n < 3) return;
  unsigned t;
  if (n->s->f < cam.z && (t = GET_TEXTURE(n->s->t, 0))) {
    texSelectTexture(t);
    glBegin(GL_POLYGON);
    glNormal3f(0.0, 0.0, 1.0);
    for (line_t *l = n->p; l < n->p + n->n; ++l) {
      const float x = vc.p[l->a].x;
      const float y = vc.p[l->a].y;
//      const float u = (x - n->bb.x1) / (n->bb.x2 - n->bb.x1);
//      const float v = (y - n->bb.y1) / (n->bb.y2 - n->bb.y1);
      glTexCoord2f((x + n->s->u) * 0.015625, (y + n->s->v) * 0.015625);
//      glMultiTexCoord2f(GL_TEXTURE1, u, v);
      glVertex3f(x, y, n->s->f);
    }
    glEnd();
    ++visfaces;
  }
  if (n->s->c > cam.z && (t = GET_TEXTURE(n->s->t, 1))) {
    texSelectTexture(t);
    glBegin(GL_POLYGON);
    glNormal3f(0.0, 0.0, -1.0);
    for (line_t *l = n->p + n->n - 1; l >= n->p; --l) {
      const float x = vc.p[l->a].x;
      const float y = vc.p[l->a].y;
//      const float u = (x - n->bb.x1) / (n->bb.x2 - n->bb.x1);
//      const float v = (y - n->bb.y1) / (n->bb.y2 - n->bb.y1);
      glTexCoord2f((x + n->s->u) * 0.015625, (y + n->s->v) * 0.015625);
//      glMultiTexCoord2f(GL_TEXTURE1, u, v);
      glVertex3f(x, y, n->s->c);
    }
    glEnd();
    ++visfaces;
  }
}

static void
rDrawNode(node_t *n)
{
  if (!bbVisible(&n->bb)) return;
  vertex_t camv;
  camv.x = cam.x;
  camv.y = cam.y;
  const int det = n->div.determine(camv);
  if (det > 0) {
    if (n->r) rDrawNode(n->r);
  } else {
    if (n->l) rDrawNode(n->l);
  }
  if (/*(n->flags & NF_VISIBLE) &&*/ n->s && n->n) {
  glColor3ub(n->s->l, n->s->l, n->s->l);
//  glColor3f(1.0, 1.0, 1.0);
/*  texLoadTexture(0xc, 0);
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_2D);
  texSelectTexture(0xc);
  glActiveTexture(GL_TEXTURE0);*/
  glDisable(GL_POLYGON_SMOOTH);
  if (r_drawwalls) rDrawWalls(n);
  rDrawPlanes(n);
/*  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE0);*/
  ++visnodes;
  }
  if (det > 0) {
    if (n->l) rDrawNode(n->l);
  } else {
    if (n->r) rDrawNode(n->r);
  }
}

static void
rDrawTree()
{
  if (root != NULL) rDrawNode(root);
}

//extern char texdbg[8192];

void rBuildFrame() {
  float d;

  if (!bspIsLoaded()) return;
/*  glEnable(GL_FOG);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFogf(GL_FOG_DENSITY, 0.1);
  glFogf(GL_FOG_START, 20);
  glFogf(GL_FOG_END, 100);*/
//  glHint(GL_FOG_HINT, GL_NICEST);
/*  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);*/
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  d = tanf(curfov / 360.0 * M_PI);
  glFrustum(-d, d, -d, d, 1.0, 10000.0);
  glRotatef(270.0 + cam.b / M_PI * 180.0, 1.0, 0.0, 0.0);
  glRotatef(cam.a / M_PI * 180.0, 0.0, 0.0, 1.0);
  glTranslatef(-cam.x, -cam.y, -cam.z);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.0);
  GLbitfield clear = GL_DEPTH_BUFFER_BIT;
  if (r_clear) {
    clear |= GL_COLOR_BUFFER_BIT;
    glClearColor(1.0, 0.5, 1.0, 0.0);
  }
  glClear(clear);
  visnodes = visfaces = 0;
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_ALWAYS);
  rTraceTreeForVisibles();
  glEnable(GL_TEXTURE_2D);
  rDrawTree();
  glDisable(GL_TEXTURE_2D);
//  glEnable(GL_NORMALIZE);
  float fv[4];
  const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  const float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  fv[0] = 0.0;//cam.x;
  fv[1] = 0.0;//cam.y;
  fv[2] = 0.0;//cam.z;
  fv[3] = 1.0;
  glLightfv(GL_LIGHT1, GL_POSITION, fv);
  glLightfv(GL_LIGHT1, GL_AMBIENT, white);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, white);
  glLightfv(GL_LIGHT1, GL_SPECULAR, black);
//  glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.01);
//  glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0001);
//  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT1);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  objDrawObjects();
//  glShadeModel(GL_SMOOTH);
  glDisable(GL_LIGHT1);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);

/*  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);
  texSelectTexture(3);
  glBegin(GL_QUADS);
  glColor3f(1.0, 1.0, 1.0);
  glTexCoord2f(0.0, 0.0);
  glVertex3f(10.0*cam.v[3].x + cam.x, 10.0*cam.v[3].y + cam.y, 10.0*cam.v[3].z + cam.z);
  glTexCoord2f(0.0, 1.0);
  glVertex3f(10.0*cam.v[2].x + cam.x, 10.0*cam.v[2].y + cam.y, 10.0*cam.v[2].z + cam.z);
  glTexCoord2f(1.0, 1.0);
  glVertex3f(10.0*cam.v[1].x + cam.x, 10.0*cam.v[1].y + cam.y, 10.0*cam.v[1].z + cam.z);
  glTexCoord2f(1.0, 0.0);
  glVertex3f(10.0*cam.v[0].x + cam.x, 10.0*cam.v[0].y + cam.y, 10.0*cam.v[0].z + cam.z);
  glEnd();
  glDisable(GL_TEXTURE_2D);*/

//  glDisable(GL_FOG);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  dmsg(MLDBG, "%d nodes", visnodes);
  dmsg(MLDBG, "%d faces", visfaces);
/*  glRasterPos2d(0.0, 0.0);
  glPixelZoom(20.0, 20.0);
  glDrawPixels(2, 2, GL_LUMINANCE, GL_UNSIGNED_BYTE, texdbg);*/
}

void rInit() {
  cmdAddBool("r_clear", &r_clear, 1);
  cmdAddBool("r_drawwalls", &r_drawwalls, 1);
}

void rDone() {
}
