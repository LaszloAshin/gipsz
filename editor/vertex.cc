#include <stdlib.h>
#include "vertex.h"
#include "line.h"
#include "ed.h"
#include "gr.h"

void
Vertex::print(std::ostream& os, int index)
const
{
	os << "vertex #" << index << " x=" << x << " y=" << y << " md=" << md << std::endl;
}

void
Vertex::save(std::ostream& os)
const
{
  char buf[2 * 2], *p = buf;
  *(short *)p = x;
  *(short *)(p + 2) = y;
  os.write(buf, sizeof(buf));
}

Vertex
Vertex::load(std::istream& is)
{
  Vertex result;
  char buf[2 * 2], *p = buf;
  is.read(buf, sizeof(buf));
  result.x = *(short *)p;
  result.y = *(short *)(p + 2);
  return result;
}


Vertexes vc;
Vertex* sv = 0;

Vertex* edAddVertex(int x, int y) {
  const int selected = sv ? (sv - &vc.front()) : -1;
  vc.push_back(Vertex(x, y));
  sv = (selected >= 0) ? &vc.at(selected) : 0;
  return &vc.back();
}

Vertex *edGetVertex(int x, int y) {
  for (unsigned i = 0; i < vc.size(); ++i)
    if (vc[i].x == x && vc[i].y == y)
      return &vc.at(i);
  return NULL;
}

void edDelVertex(Vertex *p) {
  const unsigned i = p - &vc.front();
  if (!p || i >= vc.size()) return;
  for (unsigned j = 0; j < lc.size(); ++j)
    if (lc[j].a == (int)i || lc[j].b == (int)i) {
      lc[j] = lc.back();
	  lc.pop_back();
      sl = 0;
      --j;
    }
  vc[i] = vc.back();
  vc.pop_back();
  for (unsigned j = 0; j < lc.size(); ++j) {
    if (lc[j].a == (int)vc.size()) lc[j].a = i;
    if (lc[j].b == (int)vc.size()) lc[j].b = i;
  }
  sv = 0;
}

void edMouseButtonVertex(int mx, int my, int button) {
  Vertex *v = edGetVertex(mx, my);
  if (snap && v == NULL && sv != NULL && sv->md <= gs*gs) v = sv;
  if (button == 1) {
    if (v != NULL) {
      sv = v;
      moving = 1;
    } else {
      edAddVertex(mx, my);
      grBegin();
      grSetColor(255);
      edVertex(mx, my);
      grEnd();
    }
  } else {
    if (moving) {
      moving = 0;
      edScreen();
    }
    if (button == 3 && v != NULL) {
      edDelVertex(v);
      edScreen();
    }
  }
}

void edMouseMotionVertex(int mx, int my, int umx, int umy) {
  (void)umx;
  (void)umy;
  grBegin();
  grSetPixelMode(PMD_XOR);
  grSetColor(255);
  if (omx != -1 || omy != -1) edOctagon(omx, omy, 4);
  edOctagon(mx, my, 4);
  grSetPixelMode(PMD_SET);
  grEnd();
  if (moving && sv != NULL) {
    if (edGetVertex(mx, my) == NULL) {
      grBegin();
      grSetColor(254);
      grSetPixelMode(PMD_XOR);
      edVertex(sv->x, sv->y);
      edVertex(mx, my);
      grSetPixelMode(PMD_SET);
      grEnd();
      sv->x = mx;
      sv->y = my;
    }
  }
}

void edKeyboardVertex(int key) {
  (void)key;
}

int pszt(Vertex p1, Vertex p2, int mx, int my) {
  int dx21, dy21;
  if (p1.x > p2.x) {
    Vertex p(p1);
    p1 = p2;
    p2 = p;
  }
  dx21 = p2.x - p1.x;
  dy21 = p2.y - p1.y;
  int x = my - p1.y;
  x *= dx21 * dy21;
  x += p1.x * dy21 * dy21 + mx * dx21 * dx21;
  x /= dy21 * dy21 + dx21 * dx21;
  int y;
  if (x < p1.x) {
    x = p1.x;
    y = p1.y;
  } else if (x > p2.x) {
    x = p2.x;
    y = p2.y;
  } else {
    if (abs(dx21) > abs(dy21)) {
      if (dx21)
        y = p1.y + (x - p1.x) * dy21 / dx21;
      else
        return 0;
    } else if (dy21)
      y = my - (x - mx) * dx21 / dy21;
    else
      return 0;
    if (p1.y > p2.y) {
      Vertex p(p1);
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
  dx21 = mx - x;
  dy21 = my - y;
  return dx21 * dx21 + dy21 * dy21;
}
