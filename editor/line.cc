#include <stdlib.h>
#include <math.h>
#include "line.h"
#include "vertex.h"
#include "gr.h"
#include "ed.h"

lc_t lc;
line_t *sl = NULL, tmpline;

line_t *edGetLine(int a, int b) {
  for (unsigned i = 0; i < lc.n; ++i)
    if ((lc.p[i].a == a && lc.p[i].b == b) || (lc.p[i].a == b && lc.p[i].b == a))
      return lc.p + i;
  return NULL;
}

line_t *edAddLine(int a, int b, int sf, int sb, int u, int v, int flags, int tf, int tb, int du) {
  line_t *p = edGetLine(a, b);
  if (p != NULL || a == b) return p;
  if (lc.n == lc.alloc) {
    lc.alloc *= 2;
    line_t *p = (line_t *)malloc(lc.alloc * sizeof(line_t));
    if (p == NULL) return p;
    unsigned i;
    for (i = 0; i < lc.n; ++i)
      p[i] = lc.p[i];
    if (sl != NULL) sl += p - lc.p;
    free(lc.p);
    lc.p = p;
  }
  lc.p[lc.n].a = a;
  lc.p[lc.n].b = b;
  lc.p[lc.n].sf = sf;
  lc.p[lc.n].sb = sb;
  lc.p[lc.n].u = u;
  lc.p[lc.n].v = v;
  lc.p[lc.n].flags = flags;
  lc.p[lc.n].tf = tf;
  lc.p[lc.n].tb = tb;
  lc.p[lc.n].du = du;
  ++lc.n;
  return lc.p + lc.n - 1;
}

void edDelLine(line_t *p) {
  unsigned i = p - lc.p;
  if (p == NULL || i > lc.n) return;
  lc.p[i] = lc.p[--lc.n];
  sl = NULL;
}

void edMouseButtonLine(int mx, int my, int button) {
  (void)mx;
  (void)my;
  if (button == 1) {
    if (sv != NULL) {
      if (tmpline.a == -1) {
        tmpline.a = sv - &vc.front();
        tmpline.b = -1;
      } else if (tmpline.b != -1 && tmpline.b != tmpline.a) {
        edAddLine(tmpline.a, tmpline.b, 0, 0, 0, 0, LF_NOTHING, 0, 0, 0);
        grBegin();
        grSetColor(255);
        edVector(vc[tmpline.a].x, vc[tmpline.a].y, vc[tmpline.b].x, vc[tmpline.b].y);
        grEnd();
        tmpline.a = tmpline.b;
        tmpline.b = -1;
      }
    }
  } else {
    if (button == 3) {
      if (tmpline.a != -1) {
        if (tmpline.b != -1) {
          grBegin();
          grSetColor(254);
          grSetPixelMode(PMD_XOR);
          edVector(vc[tmpline.a].x, vc[tmpline.a].y, vc[tmpline.b].x, vc[tmpline.b].y);
          grSetPixelMode(PMD_SET);
          grEnd();
          tmpline.b = -1;
        }
        tmpline.a = -1;
      } else if (sl != NULL) {
        grBegin();
        grSetColor(0);
        edVector(vc[sl->a].x, vc[sl->a].y, vc[sl->b].x, vc[sl->b].y);
        grEnd();
        edDelLine(sl);
      }
    }
  }
}

void edMouseMotionLine(int mx, int my, int umx, int umy) {
  (void)mx;
  (void)my;
  if (tmpline.a != -1) {
    if (sv != NULL && edGetLine(tmpline.a, sv - &vc.front()) == NULL) {
      grBegin();
      grSetColor(254);
      grSetPixelMode(PMD_XOR);
      if (tmpline.b != -1) {
        edVector(vc[tmpline.a].x, vc[tmpline.a].y, vc[tmpline.b].x, vc[tmpline.b].y);
      }
      if (sv != NULL) {
        tmpline.b = sv - &vc.front();
        edVector(vc[tmpline.a].x, vc[tmpline.a].y, vc[tmpline.b].x, vc[tmpline.b].y);
      } else
        tmpline.b = -1;
      grSetPixelMode(PMD_SET);
      grEnd();
    }
  } else {
    line_t *min = NULL;
    for (unsigned i = 0; i < lc.n; ++i) {
      lc.p[i].md = pszt(vc[lc.p[i].a], vc[lc.p[i].b], umx, umy);
      if (min == NULL || lc.p[i].md < min->md) min = lc.p + i;
    }
    if ((min != NULL || sl != NULL) && min != sl) {
      grBegin();
      if (sl != NULL) {
        if (sl->sf && sl->sb)
          grSetColor(76);
        else
          grSetColor(255);
        edVector(vc[sl->a].x, vc[sl->a].y, vc[sl->b].x, vc[sl->b].y);
      }
      if (min != NULL) {
        grSetColor(253);
        edVector(vc[min->a].x, vc[min->a].y, vc[min->b].x, vc[min->b].y);
      }
      grEnd();
      edStBegin();
      if (min != NULL) {
        const int dx = vc[min->b].x - vc[min->a].x;
        const int dy = vc[min->b].y - vc[min->a].y;
        const int i = min->u + sqrtf(dx * dx + dy * dy) + min->du;
        grprintf("line #%04d - u:%02d:%02d (%02d) v:%02d tf:%03x:%03x:%03x tb:%03x:%03x:%03x",
          min - lc.p, min->u, i, i & 0x3f, min->v,
          (min->tf >> 20) & 0x3ff, (min->tf >> 10) & 0x3ff, min->tf & 0x3ff,
          (min->tb >> 20) & 0x3ff, (min->tb >> 10) & 0x3ff, min->tb & 0x3ff);
      }
      edStEnd();
    }
    sl = min;
  }
}

void edKeyboardLine(int key) {
  int i = 0;
  static int cb[3] = { 0, 0, 0 };
  if (sl != NULL) {
    ++i;
    switch (key) {
      case SDLK_1:
        if (sl->u) --sl->u;
        break;
      case SDLK_2:
        ++sl->u;
        break;
      case SDLK_3:
        if (sl->v) --sl->v;
        break;
      case SDLK_4:
        ++sl->v;
        break;
      case SDLK_5:
        --sl->du;
        break;
      case SDLK_6:
        ++sl->du;
        break;
      case SDLK_f: {
        int c;
        c = sl->a, sl->a = sl->b, sl->b = c;
        c = sl->sb, sl->sb = sl->sf, sl->sf = c;
        c = sl->tb, sl->tb = sl->tf, sl->tf = c;
        line_t *tmp = sl;
        edScreen();
        sl = tmp;
        break;
      }
      case SDLK_a:
        if (sl->tf & 0x3ff00000)
          sl->tf = (sl->tf & 0xfffff) | ((sl->tf & 0x3ff00000) - 0x100000);
        break;
      case SDLK_w:
        if ((sl->tf & 0x3ff00000) < 0x3ff00000)
          sl->tf = (sl->tf & 0xfffff) | ((sl->tf & 0x3ff00000) + 0x100000);
        break;
      case SDLK_s:
        if (sl->tf & 0xffc00)
          sl->tf = (sl->tf & 0x3ff003ff) | ((sl->tf & 0xffc00) - 0x400);
        break;
      case SDLK_e:
        if ((sl->tf & 0xffc00) < 0xffc00)
          sl->tf = (sl->tf & 0x3ff003ff) | ((sl->tf & 0xffc00) + 0x400);
        break;
      case SDLK_d:
        if (sl->tf & 0x3ff)
          sl->tf = (sl->tf & 0x3ffffc00) | ((sl->tf & 0x3ff) - 1);
        break;
      case SDLK_r:
        if ((sl->tf & 0x3ff) < 0x3ff)
          sl->tf = (sl->tf & 0x3ffffc00) | ((sl->tf & 0x3ff) + 1);
        break;
      case SDLK_h:
        if (sl->tb & 0x3ff00000)
          sl->tb = (sl->tb & 0xfffff) | ((sl->tb & 0x3ff00000) - 0x100000);
        break;
      case SDLK_y:
        if ((sl->tb & 0x3ff00000) < 0x3ff00000)
          sl->tb = (sl->tb & 0xfffff) | ((sl->tb & 0x3ff00000) + 0x100000);
        break;
      case SDLK_j:
        if (sl->tb & 0xffc00)
          sl->tb = (sl->tb & 0x3ff003ff) | ((sl->tb & 0xffc00) - 0x400);
        break;
      case SDLK_u:
        if ((sl->tb & 0xffc00) < 0xffc00)
          sl->tb = (sl->tb & 0x3ff003ff) | ((sl->tb & 0xffc00) + 0x400);
        break;
      case SDLK_k:
        if (sl->tb & 0x3ff)
          sl->tb = (sl->tb & 0x3ffffc00) | ((sl->tb & 0x3ff) - 1);
        break;
      case SDLK_i:
        if ((sl->tb & 0x3ff) < 0x3ff)
          sl->tb = (sl->tb & 0x3ffffc00) | ((sl->tb & 0x3ff) + 1);
        break;
      case SDLK_c:
        cb[0] = sl->tf;
        cb[1] = sl->tb;
        cb[2] = sl->v;
        break;
      case SDLK_v:
        sl->tf = cb[0];
        sl->tb = cb[1];
        sl->v = cb[2];
        break;
      default:
        --i;
        break;
    }
  }
  if (i && sl != NULL) {
    edStBegin();
    int dx = vc[sl->b].x - vc[sl->a].x;
    int dy = vc[sl->b].y - vc[sl->a].y;
    i = sl->u + sqrtf(dx * dx + dy * dy) + sl->du;
    grprintf("line #%04d - u:%02d:%02d (%02d) v:%02d tf:%03x:%03x:%03x tb:%03x:%03x:%03x",
      sl - lc.p, sl->u, i, i & 0x3f, sl->v,
      (sl->tf >> 20) & 0x3ff, (sl->tf >> 10) & 0x3ff, sl->tf & 0x3ff,
      (sl->tb >> 20) & 0x3ff, (sl->tb >> 10) & 0x3ff, sl->tb & 0x3ff);
    edStEnd();
  }
}

