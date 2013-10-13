#include <GL/gl.h>
#include <GL/glut.h>
#include <stdlib.h>

float theta = 0.0;

void buildframe();

void GLUTCALLBACK cbDisplay(void) {
  static int dbuf = -1;
  if (dbuf == -1) dbuf = glutGet(GLUT_WINDOW_DOUBLEBUFFER);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
/*  glPushMatrix();
  glRotatef(theta, 0.0, 0.0, 1.0);
  glBegin(GL_TRIANGLES);
  glColor3f(1.0, 0.0, 0.0);  glVertex2f(0.0, 1.0);
  glColor3f(0.0, 1.0, 0.0);  glVertex2f(0.87, -0.5);
  glColor3f(0.0, 0.0, 1.0);  glVertex2f(-0.87, -0.5);
  glEnd();
  glPopMatrix();*/
  buildframe();
/*  glBegin(GL_QUADS);
  glColor3ub(0xff, 0x80, 0x60);
  int x1 = -50, y1 = -10, x2 = 50, y2 = -50;
  glVertex3i(x1, 64, y1);
  glVertex3i(x2, 64, y2);
  glVertex3i(x2, -64, y2);
  glVertex3i(x1, -64, y1);
  glEnd();*/
  if (dbuf) {
    glutSwapBuffers();
  } else {
    glFlush();
  }
}

void GLUTCALLBACK cbTimer(int i) {
  theta += 1.0;
  glutPostRedisplay();
  glutTimerFunc(40, cbTimer, 0);
}

int kbd_key(int);

void GLUTCALLBACK cbKeyboard(unsigned char key, int x, int y) {
  switch (key) {
    case 27:
      exit(0);
    default:
      if (kbd_key(key)) glutPostRedisplay();
      break;
  }
}

void init();
void done();

int main(int argc, char **argv, char **envp) {
  init();
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(640, 480);
  int win = glutCreateWindow("gipsz");
  glEnable(GL_DEPTH_TEST);
  glutDisplayFunc(cbDisplay);
  glutKeyboardFunc(cbKeyboard);
//  glutTimerFunc(40, cbTimer, 0);
//  glutFullScreen();
  glutSetCursor(GLUT_CURSOR_NONE);
  glScalef(0.003125, 0.0041666, 0.001);
  gluPerspective(30, 0.75, 10, 100);
  glutMainLoop();
  glutDestroyWindow(win);
  done();
  return 0;
}
