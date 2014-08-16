/* vim: set ts=2 sw=8 tw=0 et :*/
#include <stdlib.h>
#include <SDL/SDL.h>
#include "sector.h"
#include "line.h"
#include "vertex.h"
#include "gr.h"
#include "ed.h"

#include <stdexcept>

void
Sector::saveText(std::ostream& os, size_t index)
const
{
  os << "sector " << index << ' ';
  os << f << ' ';
  os << c << ' ';
  os << int(l) << ' ';
  os << u << ' ';
  os << v << ' ';
  os << t << std::endl;
}

Sector
Sector::loadText(std::istream& is, size_t expectedIndex)
{
  Sector result;
  std::string name;
  is >> name;
  if (name != "sector") throw std::runtime_error("sector expected");
  size_t index;
  is >> index;
  if (index != expectedIndex) throw std::runtime_error("unexpected index");
  is >> result.f;
  is >> result.c;
  is >> result.l;
  is >> result.u;
  is >> result.v;
  is >> result.t;
  return result;
}

Sectors sc;
int ss = 0;

int edGetSector(unsigned s) {
  return (s && s < sc.size() && (sc[s].f || sc[s].c || sc[s].l)) ? s : 0;
}

int edAddSector(unsigned s, int f, int c, int l, int u, int v, int t) {
  if (!s) return 0;
  if (s >= sc.size()) sc.resize(s + 1);
  sc[s].f = f;
  sc[s].c = c;
  sc[s].l = l;
  sc[s].u = u;
  sc[s].v = v;
  sc[s].t = t;
  return s;
}

void edDelSector(int s) {
  if (edGetSector(s)) sc[s].f = sc[s].c = sc[s].l = 0;
  for (unsigned i = 0; i < lc.size(); ++i) {
    if (lc[i].sf == s) lc[i].sf = 0;
    if (lc[i].sb == s) lc[i].sb = 0;
  }
}

int edAllocSector() {
  int j = 0, bo;
  do {
    ++j;
    bo = 0;
    for (unsigned i = 0; i < lc.size(); ++i)
      if (lc[i].sf == j || lc[i].sb == j) {
        ++bo;
        break;
      }
  } while (bo);
  return j;
}

void edSelectSector(int s, int o) {
  for (unsigned i = 0; i < lc.size(); ++i) {
    if (lc[i].sf == s) {
      if (o)
        grSetColor(253);
      else if (lc[i].sf && lc[i].sb)
        grSetColor(76);
      else
        grSetColor(255);
      edVector(vc[lc[i].a].x, vc[lc[i].a].y, vc[lc[i].b].x, vc[lc[i].b].y);
    }
    if (lc[i].sb == s) {
      if (o)
        grSetColor(253);
      else if (lc[i].sf && lc[i].sb)
        grSetColor(76);
      else
        grSetColor(255);
      edVector(vc[lc[i].b].x, vc[lc[i].b].y, vc[lc[i].a].x, vc[lc[i].a].y);
    }
  }
}

void edMouseButtonSector(int mx, int my, int button) {
  (void)mx;
  (void)my;
  if (button == 1) {
    if (sv != NULL) {
      if (tmpline.a == -1) {
        tmpline.a = sv - &vc.front();
        tmpline.b = -1;
        SDLMod m = SDL_GetModState();
        if (!(m & KMOD_SHIFT)) {
          if (ss) {
            grBegin();
            edSelectSector(ss, 0);
            grEnd();
          }
          ss = edAllocSector();
        }
      } else if (tmpline.b != -1 && tmpline.b != tmpline.a) {
        Line* p = edGetLine(tmpline.a, tmpline.b);
        if (p != NULL) {
          if (p->sf != ss && p->sb != ss) {
            if (p->a == tmpline.a)
              p->sf = ss;
            else
              p->sb = ss;
          }
          if (!edGetSector(ss)) edAddSector(ss, 0, 80, 0xff, 0, 0, 0x401);
        } else {
          grBegin();
          grSetColor(253);
          grSetPixelMode(PMD_XOR);
          edVector(vc[tmpline.a].x, vc[tmpline.a].y, vc[tmpline.b].x, vc[tmpline.b].y);
          grSetPixelMode(PMD_SET);
          grEnd();
        }
        tmpline.a = tmpline.b;
        tmpline.b = -1;
        grBegin();
        edSelectSector(ss, 1);
        grEnd();
      }
    }
  } else {
    if (button == 3) {
      if (tmpline.a != -1) {
        if (tmpline.b != -1) {
          grBegin();
          grSetColor(253);
          grSetPixelMode(PMD_XOR);
          edVector(vc[tmpline.a].x, vc[tmpline.a].y, vc[tmpline.b].x, vc[tmpline.b].y);
          grSetPixelMode(PMD_SET);
          grEnd();
          tmpline.b = -1;
        }
        tmpline.a = -1;
      } else if (ss != 0) {
        edDelSector(ss);
        edScreen();
      }
    }
  }
}

void edMouseMotionSector(int mx, int my, int umx, int umy) {
  (void)mx;
  (void)my;
  SDLMod m = SDL_GetModState();
  if (tmpline.a != -1) {
    if (sv != NULL && edGetLine(tmpline.a, sv - &vc.front()) != NULL) {
      grBegin();
      grSetColor(253);
      grSetPixelMode(PMD_XOR);
      if (tmpline.b != -1) {
        edVector(vc[tmpline.a].x, vc[tmpline.a].y, vc[tmpline.b].x, vc[tmpline.b].y);
      }
      if (sv != NULL) {
        tmpline.b = sv - &vc.front();
        edVector(vc[tmpline.a].x, vc[tmpline.a].y, vc[tmpline.b].x, vc[tmpline.b].y);
      }
      grSetPixelMode(PMD_SET);
      grEnd();
    }
  } else if (!(m & KMOD_SHIFT)) {
    int s;
    Line* min = 0;
    for (unsigned i = 0; i < lc.size(); ++i) {
      lc[i].md = pszt(vc[lc[i].a], vc[lc[i].b], umx, umy);
      if (min == NULL || lc[i].md < min->md) min = &lc.at(i);
    }
    if (min != NULL) {
      int dx1 = vc[min->b].x - vc[min->a].x;
      int dy1 = vc[min->b].y - vc[min->a].y;
      int dx2 = umx - vc[min->a].x;
      int dy2 = umy - vc[min->a].y;
      s = (dx1 * dy2 < dy1 * dx2) ? min->sb : min->sf;
      if (s != ss) {
        if (ss || s) {
          grBegin();
          if (ss) edSelectSector(ss, 0);
          if (s) edSelectSector(s, 1);
          grEnd();
          edStBegin();
		  int i;
          if (s && (i = edGetSector(s)) > 0)
            grprintf("sector #%04d - f:%03d c:%03d l:0x%02x u:0x%02x v:0x%02x tc:%03x tf:%03x",
              i, sc[i].f, sc[i].c, sc[i].l, sc[i].u, sc[i].v,
              (sc[i].t >> 10) & 0x3ff, sc[i].t & 0x3ff);
          edStEnd();
        }
        ss = s;
      }
    }
  }
}

void edKeyboardSector(int key) {
  static int cb[3] = { 0, 0, 0 };
  int i = edGetSector(ss);
  if (!i) i = edAddSector(ss, 0, 80, 0xff, 0, 0, 0x401);
  if (i > 0)
    switch (key) {
      case SDLK_1:
        sc[i].f -= 8;
        break;
      case SDLK_2:
        sc[i].f += 8;
        break;
      case SDLK_3:
        sc[i].c -= 8;
        break;
      case SDLK_4:
        sc[i].c += 8;
        break;
      case SDLK_5:
        if (sc[i].l > 8)
          sc[i].l -= 8;
        else
          sc[i].l = 0;
        break;
      case SDLK_6:
        if (sc[i].l < 0xff-8)
          sc[i].l += 8;
        else
          sc[i].l = 0xff;
        break;
      case SDLK_h:
        if (sc[i].u > 0) --sc[i].u;
        break;
      case SDLK_y:
        ++sc[i].u;
        break;
      case SDLK_j:
        if (sc[i].v > 0) --sc[i].v;
        break;
      case SDLK_u:
        ++sc[i].v;
        break;
      case SDLK_k:
        if (sc[i].t & 0xffc00)
          sc[i].t = (sc[i].t & 0x3ff) | ((sc[i].t & 0xffc00) - 0x400);
        break;
      case SDLK_i:
        if ((sc[i].t & 0xffc00) < 0xffc00)
          sc[i].t = (sc[i].t & 0x3ff) | ((sc[i].t & 0xffc00) + 0x400);
        break;
      case SDLK_l:
        if (sc[i].t & 0x3ff)
          sc[i].t = (sc[i].t & 0xffc00) | ((sc[i].t & 0x3ff) - 1);
        break;
      case SDLK_o:
        if ((sc[i].t & 0x3ff) < 0x3ff)
          sc[i].t = (sc[i].t & 0xffc00) | ((sc[i].t & 0x3ff) + 1);
        break;
      case SDLK_c:
        cb[0] = sc[i].t;
        cb[1] = sc[i].u;
        cb[2] = sc[i].v;
        break;
      case SDLK_v:
        sc[i].t = cb[0];
        sc[i].u = cb[1];
        sc[i].v = cb[2];
        break;
      default:
        i = -1;
        break;
    }
  if (i > 0) {
    edStBegin();
    grprintf("sector #%04d - f:%03d c:%03d l:0x%02x u:0x%02x v:0x%02x tc:%03x tf:%03x",
      i, sc[i].f, sc[i].c, sc[i].l, sc[i].u, sc[i].v,
      (sc[i].t >> 10) & 0x3ff, sc[i].t & 0x3ff);
    edStEnd();
  }
}
