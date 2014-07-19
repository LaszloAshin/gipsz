
void modelCreateCube(model_t *m, float a) {
  m->nverts = 16;
  m->vp = (vert_t *)mmAlloc(m->nverts * sizeof(vert_t));
  if (m->vp == NULL) {
    m->nverts = 0;
    return;
  }
  m->ntris = 12;
  m->tp = (tri_t *)mmAlloc(m->ntris * sizeof(tri_t));
  if (m->tp == NULL) {
    m->ntris = 0;
    mmFree(m->tp);
    m->nverts = 0;
    return;
  }
  m->vp[ 0].x = -a; m->vp[ 0].y = -a; m->vp[ 0].z =  a;
  m->vp[ 1].x =  a; m->vp[ 1].y = -a; m->vp[ 1].z =  a;
  m->vp[ 2].x =  a; m->vp[ 2].y =  a; m->vp[ 2].z =  a;
  m->vp[ 3].x = -a; m->vp[ 3].y =  a; m->vp[ 3].z =  a;
  m->vp[ 4].x = -a; m->vp[ 4].y = -a; m->vp[ 4].z = -a;
  m->vp[ 5].x =  a; m->vp[ 5].y = -a; m->vp[ 5].z = -a;
  m->vp[ 6].x =  a; m->vp[ 6].y =  a; m->vp[ 6].z = -a;
  m->vp[ 7].x = -a; m->vp[ 7].y =  a; m->vp[ 7].z = -a;
  m->vp[ 8].x = -a; m->vp[ 8].y = -a; m->vp[ 8].z =  a;
  m->vp[ 9].x = -a; m->vp[ 9].y =  a; m->vp[ 9].z =  a;
  m->vp[10].x = -a; m->vp[10].y =  a; m->vp[10].z = -a;
  m->vp[11].x = -a; m->vp[11].y = -a; m->vp[11].z = -a;
  m->vp[12].x =  a; m->vp[12].y = -a; m->vp[12].z =  a;
  m->vp[13].x =  a; m->vp[13].y =  a; m->vp[13].z =  a;
  m->vp[14].x =  a; m->vp[14].y =  a; m->vp[14].z = -a;
  m->vp[15].x =  a; m->vp[15].y = -a; m->vp[15].z = -a;

  m->vp[ 0].s = 0.0; m->vp[ 0].t = 0.0;
  m->vp[ 1].s = 1.0; m->vp[ 1].t = 0.0;
  m->vp[ 2].s = 1.0; m->vp[ 2].t = 1.0;
  m->vp[ 3].s = 0.0; m->vp[ 3].t = 1.0;
  m->vp[ 4].s = 0.0; m->vp[ 4].t = 1.0;
  m->vp[ 5].s = 1.0; m->vp[ 5].t = 1.0;
  m->vp[ 6].s = 1.0; m->vp[ 6].t = 0.0;
  m->vp[ 7].s = 0.0; m->vp[ 7].t = 0.0;
  m->vp[ 8].s = 0.0; m->vp[ 8].t = 0.0;
  m->vp[ 9].s = 1.0; m->vp[ 9].t = 0.0;
  m->vp[10].s = 1.0; m->vp[10].t = 1.0;
  m->vp[11].s = 0.0; m->vp[11].t = 1.0;
  m->vp[12].s = 0.0; m->vp[12].t = 0.0;
  m->vp[13].s = 1.0; m->vp[13].t = 0.0;
  m->vp[14].s = 1.0; m->vp[14].t = 1.0;
  m->vp[15].s = 0.0; m->vp[15].t = 1.0;

  m->tp[ 0].a =  0; m->tp[ 0].b =  1; m->tp[ 0].c =  2;
  m->tp[ 1].a =  0; m->tp[ 1].b =  2; m->tp[ 1].c =  3;
  m->tp[ 2].a =  3; m->tp[ 2].b =  2; m->tp[ 2].c =  6;
  m->tp[ 3].a =  3; m->tp[ 3].b =  6; m->tp[ 3].c =  7;
  m->tp[ 4].a =  7; m->tp[ 4].b =  6; m->tp[ 4].c =  5;
  m->tp[ 5].a =  7; m->tp[ 5].b =  5; m->tp[ 5].c =  4;
  m->tp[ 6].a =  4; m->tp[ 6].b =  5; m->tp[ 6].c =  1;
  m->tp[ 7].a =  4; m->tp[ 7].b =  1; m->tp[ 7].c =  0;
  m->tp[ 8].a =  8; m->tp[ 8].b =  9; m->tp[ 8].c = 10;
  m->tp[ 9].a =  8; m->tp[ 9].b = 10; m->tp[ 9].c = 11;
  m->tp[10].a = 13; m->tp[10].b = 12; m->tp[10].c = 15;
  m->tp[11].a = 13; m->tp[11].b = 15; m->tp[11].c = 14;
  m->tex = 6;
}

void modelCreateTorus(model_t *m, float r, int step) {
  float r2 = 0.2 * r;
  int s2 = step >> 1;
  m->nverts = (step + 1) * (s2 + 1);
  m->vp = (vert_t *)mmAlloc(m->nverts * sizeof(vert_t));
  if (m->vp == NULL) {
    m->nverts = 0;
    return;
  }
  m->ntris = step * s2 * 2;
  m->tp = (tri_t *)mmAlloc(m->ntris * sizeof(tri_t));
  if (m->tp == NULL) {
    m->ntris = 0;
    mmFree(m->tp);
    m->nverts = 0;
    return;
  }
  int i, j, n = 0;
  for (i = 0; i <= step; ++i) {
    float alpha = (2.0f * M_PI) * i / step;
    for (j = 0; j <= s2; ++j) {
      float beta = (2.0f * M_PI) * j / s2;
      m->vp[n].x = (r + r2 * sinf(beta)) * cosf(alpha);
      m->vp[n].y = (r + r2 * sinf(beta)) * sinf(alpha);
      m->vp[n].z = r2 * cosf(beta);
      m->vp[n].s = 1.0 / (2.0f * M_PI) * alpha;
      m->vp[n].t = 1.0 / (2.0f * M_PI) * beta;
      ++n;
    }
  }
  n = 0;
  for (i = 0; i < step; ++i) {
    for (j = 0; j < s2; ++j) {
      int k = i * (s2 + 1) + j;
      m->tp[n].a = k;
      m->tp[n].b = k + 1;
      m->tp[n].c = k + s2 + 1;
      ++n;
      m->tp[n].a = k + s2 + 1;
      m->tp[n].b = k + 1;
      m->tp[n].c = k + s2 + 2;
      ++n;
    }
  }
  m->tex = 15;
}

void modelCreateSphere(model_t *m, float r, int step) {
  int i, j, k, n;
  float alpha, beta;

  m->nverts = (step + 1) * (step + 1);
  m->vp = (vert_t *)mmAlloc(m->nverts * sizeof(vert_t));
  if (m->vp == NULL) {
    m->nverts = 0;
    return;
  }
  m->ntris = step * step * 2;
  m->tp = (tri_t *)mmAlloc(m->ntris * sizeof(tri_t));
  if (m->tp == NULL) {
    m->ntris = 0;
    mmFree(m->tp);
    m->nverts = 0;
    return;
  }
  n = 0;
  for (i = 0; i <= step; ++i) {
    alpha = (2.0f * M_PI) * i / step;
    for (j = 0; j <= step; ++j) {
      beta = M_PI * j / step;
      m->vp[n].x = r * cosf(alpha) * sinf(beta);
      m->vp[n].y = r * sinf(alpha) * sinf(beta);
      m->vp[n].z = r * cosf(beta);
      m->vp[n].s = 1.0 / (2.0f * M_PI) * alpha;
      m->vp[n].t = 1.0 / M_PI * beta;
      ++n;
    }
  }
  n = 0;
  for (i = 0; i < step; ++i) {
    for (j = 0; j < step; ++j) {
      k = i * (step + 1) + j;
      m->tp[n].a = k;
      m->tp[n].b = k + 1;
      m->tp[n].c = k + step + 1;
      ++n;
      m->tp[n].a = k + step + 1;
      m->tp[n].b = k + 1;
      m->tp[n].c = k + step + 2;
      ++n;
    }
  }
  m->tex = 16;
}

void modelCreateTube(model_t *m, float r, float l, int step) {
  int i, j, k, n;
  float alpha;

  m->nverts = (step + 1) * 2;
  m->vp = (vert_t *)mmAlloc(m->nverts * sizeof(vert_t));
  if (m->vp == NULL) {
    m->nverts = 0;
    return;
  }
  m->ntris = step * 2;
  m->tp = (tri_t *)mmAlloc(m->ntris * sizeof(tri_t));
  if (m->tp == NULL) {
    m->ntris = 0;
    mmFree(m->tp);
    m->nverts = 0;
    return;
  }
  n = 0;
  for (i = 0; i <= step; ++i) {
    alpha = (2.0f * M_PI) * i / step;
    for (j = 0; j <= 1; ++j) {
      m->vp[n].x = r * cosf(alpha);
      m->vp[n].y = r * sinf(alpha);
      m->vp[n].z = l * (0.5 - j);
      m->vp[n].s = l * j / 64.0;
      m->vp[n].t = 1.0 / (2.0f * M_PI) * alpha;
      ++n;
    }
  }
  n = 0;
  for (i = 0; i < step; ++i) {
    k = i * 2;
    m->tp[n].a = k;
    m->tp[n].b = k + 1;
    m->tp[n].c = k + 2;
    ++n;
    m->tp[n].a = k + 2;
    m->tp[n].b = k + 1;
    m->tp[n].c = k + 3;
    ++n;
  }
  m->tex = 2;
}

void modelCreateElbow(model_t *m, float r2, float r, int step) {
  int i, j, k, n;
  float alpha, beta;

  m->nverts = (step + 1) * (step + 1);
  m->vp = (vert_t *)mmAlloc(m->nverts * sizeof(vert_t));
  if (m->vp == NULL) {
    m->nverts = 0;
    return;
  }
  m->ntris = step * step * 2;
  m->tp = (tri_t *)mmAlloc(m->ntris * sizeof(tri_t));
  if (m->tp == NULL) {
    m->ntris = 0;
    mmFree(m->tp);
    m->nverts = 0;
    return;
  }
  n = 0;
  for (i = 0; i <= step; ++i) {
    alpha = (M_PI / 2.0f) * i / step;
    for (j = 0; j <= step; ++j) {
      beta = (2.0f * M_PI) * j / step;
      m->vp[n].x = (r + r2 * sinf(beta)) * cosf(alpha);
      m->vp[n].y = (r + r2 * sinf(beta)) * sinf(alpha);
      m->vp[n].z = r2 * cosf(beta);
      m->vp[n].s = 1.0 / (M_PI / 2.0f) * alpha;
      m->vp[n].t = 1.0 / (2.0f * M_PI) * beta;
      ++n;
    }
  }
  n = 0;
  for (i = 0; i < step; ++i) {
    for (j = 0; j < step; ++j) {
      k = i * (step + 1) + j;
      m->tp[n].a = k;
      m->tp[n].b = k + 1;
      m->tp[n].c = k + step + 1;
      ++n;
      m->tp[n].a = k + step + 1;
      m->tp[n].b = k + 1;
      m->tp[n].c = k + step + 2;
      ++n;
    }
  }
  m->tex = 2;
}
