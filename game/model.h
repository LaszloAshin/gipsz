#ifndef _MODEL_H
#define _MODEL_H 1

struct Model;
struct Stat;

int modelInit();
void modelFlush();
void modelDone();
Stat *modelInitStat(void *mp);
void modelSetStatAngle(Stat *sp, float a, float b, float c);
void modelUpdate(void *mp, void *sp);
void modelDraw(Model *mp, Stat *sp);
Model *modelGetForName(int name);
Model *modelAdd(int name);

#endif /* _MODEL_H */
