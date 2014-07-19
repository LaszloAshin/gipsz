#ifndef _VERTEX_H
#define _VERTEX_H 1

#include <vector>

struct Vertex {
  int x, y;
  int md;

  Vertex() : x(0), y(0), md(0) {}
  Vertex(int x0, int y0) : x(x0), y(y0), md(0) {}
};

typedef std::vector<Vertex> Vertexes;

extern Vertexes vc;
extern Vertex* sv;

Vertex* edAddVertex(int x, int y);
Vertex* edGetVertex(int x, int y);
void edDelVertex(Vertex *p);
void edMouseButtonVertex(int mx, int my, int button);
void edMouseMotionVertex(int mx, int my, int umx, int umy);
void edKeyboardVertex(int key);
int pszt(Vertex p1, Vertex p2, int mx, int my);

#endif /* _VERTEX_H */
