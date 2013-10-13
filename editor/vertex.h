#ifndef _VERTEX_H
#define _VERTEX_H 1

typedef struct {
  int x, y;
  int md;
} vertex_t;

typedef struct {
  vertex_t *p;
  unsigned alloc, n;
} vc_t;

extern vc_t vc;
extern vertex_t *sv;

vertex_t *edAddVertex(int x, int y);
vertex_t *edGetVertex(int x, int y);
void edDelVertex(vertex_t *p);
void edMouseButtonVertex(int mx, int my, int button);
void edMouseMotionVertex(int mx, int my, int umx, int umy);
void edKeyboardVertex(int key);
int pszt(vertex_t p1, vertex_t p2, int mx, int my);

#endif /* _VERTEX_H */
