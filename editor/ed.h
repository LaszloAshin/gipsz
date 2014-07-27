#ifndef _ED_H
#define _ED_H 1

extern int snap;
extern int gs;
extern int moving;
extern int omx, omy;

void edInit();
void edDone();
void edKeyboard(int key);
void edMouseButton(int mx, int my, int button);
void edMouseMotion(int mx, int my, int state);

void edVertex(int x, int y);
void edVector(int x1, int y1, int x2, int y2);
void edOctagon(int x, int y, int a);
void edObject(int x, int y, int alpha, int r, int c);
void edScreen();
void edStBegin();
void edStEnd();
void edGetViewPort(int* x1, int* y1, int* x2, int* y2);
void edLine(int x1, int y1, int x2, int y2);

#endif /* _ED_H */
