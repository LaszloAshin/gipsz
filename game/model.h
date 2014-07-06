#ifndef _MODEL_H
#define _MODEL_H 1

int modelInit();
void modelFlush();
void modelDone();
void *modelInitStat(void *mp);
void modelSetStatAngle(void *sp, float a, float b, float c);
void modelUpdate(void *mp, void *sp);
void modelDraw(void *mp, void *sp);
void *modelGetForName(int name);
void *modelAdd(int name);

#endif /* _MODEL_H */
