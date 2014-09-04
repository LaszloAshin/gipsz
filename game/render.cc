/* vim: set ts=2 sw=8 tw=0 et :*/
#include "render.h"
#include "bsp.h"
#include <game/bbox.hh>
#include "player.h"
#include "tex.h"
#include "console.h"
#include "cmd.h"
#include "obj.h"

#include <lib/vec.hh>

#include <SDL/SDL_opengl.h>

#include <cmath>

float curfov = 90.0;
static int r_clear = 0;
static int r_drawwalls = 1;

static int visfaces, visnodes;

static void
rDrawWall(const Line& l, float f, float c, unsigned t)
{
  float v = (l.s().v() - c) * 0.015625;
  texSelectTexture(t);
/*  glActiveTexture(GL_TEXTURE1);
  texSelectTexture(0xc);
  glActiveTexture(GL_TEXTURE0);*/
  glBegin(GL_QUADS);
  glNormal3f(l.n().x(), l.n().y(), 0.0);
  glTexCoord2f(l.s().u1(), v);
//  glMultiTexCoord2f(GL_TEXTURE1, 0.0, 0.0);
  glVertex3f(l.a().x(), l.a().y(), c);

  glTexCoord2f(l.s().u2(), v);
//  glMultiTexCoord2f(GL_TEXTURE1, 1.0, 0.0);
  glVertex3f(l.b().x(), l.b().y(), c);
  v = (l.s().v() - f) * 0.015625;
  glTexCoord2f(l.s().u2(), v);
//  glMultiTexCoord2f(GL_TEXTURE1, 1.0, 1.0);
  glVertex3f(l.b().x(), l.b().y(), f);

  glTexCoord2f(l.s().u1(), v);
//  glMultiTexCoord2f(GL_TEXTURE1, 0.0, 1.0);
  glVertex3f(l.a().x(), l.a().y(), f);
  glEnd();
  ++visfaces;
}

static void
rDrawWalls(const Leaf& n)
{
  const std::tr1::shared_ptr<const Sector> s(n.s());
  if (s->height() < std::numeric_limits<double>::epsilon()) return;
  for (Leaf::const_iterator i(n.begin()); i != n.end(); ++i) {
    if (isBehind(cam.pos().xy(), *i)) continue;
    const std::tr1::shared_ptr<const Sector> ns(i->sectorBehind());
    if (const unsigned t = GET_TEXTURE(i->s().textureId(), 1)) {
      const double f = ns ? std::max(s->f(), ns->f()) : s->f();
      const double c = ns ? std::min(s->c(), ns->c()) : s->c();
      rDrawWall(*i, f, c, t);
    }
    if (!ns) continue;
    if (ns->f() > s->f()) {
      if (const unsigned t = GET_TEXTURE(i->s().textureId(), 0)) rDrawWall(*i, s->f(), ns->f(), t);
    }
    if (ns->c() < s->c()) {
      if (const unsigned t = GET_TEXTURE(i->s().textureId(), 2)) rDrawWall(*i, ns->c(), s->c(), t);
    }
  }
}

static void
rDrawPlanes(const Leaf& n)
{
  if (n.ls().size() < 3) return;
  unsigned t;
  if (n.s()->f() < cam.pos().z() && (t = GET_TEXTURE(n.s()->t(), 0))) {
    texSelectTexture(t);
    glBegin(GL_POLYGON);
    glNormal3f(0.0, 0.0, 1.0);
    for (Leaf::const_iterator i(n.begin()); i != n.end(); ++i) {
      const float x = i->a().x();
      const float y = i->a().y();
//      const float u = (x - n->bb.x1) / (n->bb.x2 - n->bb.x1);
//      const float v = (y - n->bb.y1) / (n->bb.y2 - n->bb.y1);
      glTexCoord2f((x + n.s()->u()) * 0.015625, (y + n.s()->v()) * 0.015625);
//      glMultiTexCoord2f(GL_TEXTURE1, u, v);
      glVertex3f(x, y, n.s()->f());
    }
    glEnd();
    ++visfaces;
  }
  if (n.s()->c() > cam.pos().z() && (t = GET_TEXTURE(n.s()->t(), 1))) {
    texSelectTexture(t);
    glBegin(GL_POLYGON);
    glNormal3f(0.0, 0.0, -1.0);
    for (Leaf::const_reverse_iterator i(n.rbegin()); i != n.rend(); ++i) {
      const float x = i->a().x();
      const float y = i->a().y();
//      const float u = (x - n->bb.x1) / (n->bb.x2 - n->bb.x1);
//      const float v = (y - n->bb.y1) / (n->bb.y2 - n->bb.y1);
      glTexCoord2f((x + n.s()->u()) * 0.015625, (y + n.s()->v()) * 0.015625);
//      glMultiTexCoord2f(GL_TEXTURE1, u, v);
      glVertex3f(x, y, n.s()->c());
    }
    glEnd();
    ++visfaces;
  }
}

static void
rDrawNode(const Node& n)
{
  if (!visibleByCamFrustum(n.bb())) return;
  const double det = n.div().determine(cam.pos().xy());
  if (det > 0.0f) {
    if (n.back().get()) rDrawNode(*n.back());
  } else {
    if (n.front().get()) rDrawNode(*n.front());
  }
  if (const Leaf* leaf = dynamic_cast<const Leaf*>(&n)) {
    glColor3ub(leaf->s()->l(), leaf->s()->l(), leaf->s()->l());
//  glColor3f(1.0, 1.0, 1.0);
/*  texLoadTexture(0xc, 0);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    texSelectTexture(0xc);
    glActiveTexture(GL_TEXTURE0);*/
    glDisable(GL_POLYGON_SMOOTH);
    if (r_drawwalls) rDrawWalls(*leaf);
    rDrawPlanes(*leaf);
/*    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);*/
    ++visnodes;
  }
  if (det > 0.0f) {
    if (n.front().get()) rDrawNode(*n.front());
  } else {
    if (n.back().get()) rDrawNode(*n.back());
  }
}

static void
rDrawTree()
{
  if (root.get()) rDrawNode(*root.get());
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
  glTranslatef(-cam.pos().x(), -cam.pos().y(), -cam.pos().z());
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
  dmsg(MLDBG, "pos %+03.1f %+03.1f %+03.1f", cam.pos().x(), cam.pos().y(), cam.pos().z());
  dmsg(MLDBG, "velo %+03.1f %+03.1f %+03.1f", cam.velo().x(), cam.velo().y(), cam.velo().z());
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
