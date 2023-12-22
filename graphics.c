#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "graphics.h"
GLubyte gl_frame[IMG_HEIGHT][IMG_WIDTH][3];

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2i(-1, -1);    
    glDrawPixels(IMG_WIDTH, IMG_HEIGHT, GL_RGB,
                GL_UNSIGNED_BYTE, gl_frame);
    glutSwapBuffers();

    glutPostRedisplay();
}

void graphics_update_frame(uint8_t frame[IMG_HEIGHT][IMG_WIDTH][3]){
    memcpy(gl_frame, frame, IMG_HEIGHT * IMG_WIDTH * 3);
    glutSwapBuffers();
    glutPostRedisplay();
}

void graphics_start(int argc, char** argv)
{  
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(IMG_WIDTH, IMG_HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(argv[0]);
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glutDisplayFunc(display);
    glutMainLoop();
}


