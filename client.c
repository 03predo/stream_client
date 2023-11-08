#include <stdio.h>
#include <stdint.h>

#include <pthread.h>
#include <semaphore.h>

#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdlib.h>

#define PORT 2848
#define LISTEN_LEN 5
#define IMG_HEIGHT 240
#define IMG_WIDTH 320
#define BYTES_PER_PIXEL 2

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
}pixel;

sem_t frame_sem;

uint8_t img_buf[IMG_HEIGHT*IMG_WIDTH*BYTES_PER_PIXEL];
GLubyte gl_img[IMG_HEIGHT][IMG_WIDTH][3];

pixel rgb565_to_rgb888(uint8_t b1, uint8_t b2){
  pixel p = {
    .red = (b1 >> 3),
    .green = (((b1 & 0b111) << 3) + (b2 >> 5)),
    .blue = (b2 & 0b11111),
  };
  p.red = (p.red * 527 + 23) >> 6;
  p.green = (p.green * 259 + 33) >> 6;
  p.blue = (p.blue * 527 + 23) >> 6; 
  return p;
}

void init(void)
{  
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glShadeModel(GL_FLAT);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2i(-1, -1);    
    glDrawPixels(IMG_WIDTH, IMG_HEIGHT, GL_RGB,
                GL_UNSIGNED_BYTE, gl_img);
    glutSwapBuffers();

    glutPostRedisplay();
}
void* frame_timer(void* arg){
    sem_init(&frame_sem, 0, 0);
    while(1){
        printf("posting frame_sem\n");
        sem_post(&frame_sem);
        usleep(500000);
    }
    sem_destroy(&frame_sem);
}

void* client_routine(void* arg){
    char* ip_addr = (char*)arg;
    uint8_t confirm = 0xff;
    // create socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // bind socket
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = inet_addr(ip_addr),
    };
    int ret = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    if(ret < 0){
        printf("connect failed\n");
        return NULL;
    }
    while(1){
    
    
    // receive new message
    printf("waiting for new img\n");
        int bytes_received = 0;
        while(bytes_received != (IMG_HEIGHT * IMG_WIDTH * BYTES_PER_PIXEL)){
            int tmp = recv(client_fd, img_buf + bytes_received, IMG_HEIGHT*IMG_WIDTH*BYTES_PER_PIXEL - bytes_received, MSG_WAITALL);
            if(tmp == -1){
                printf("recv failed(%d)\n", errno);
                return NULL;
            }
            printf("sent %d bytes\n", tmp);
            bytes_received += tmp;
        }

    printf("img received, sending confirmation\n");
    int bytes_sent = send(client_fd, &confirm, 1, 0);

    for(int i = 0; i < IMG_HEIGHT; ++i){
        for(int j = 0; j < IMG_WIDTH * BYTES_PER_PIXEL; j += 2){
            pixel p;
            if(j == 0){
                p = rgb565_to_rgb888(img_buf[i*640 + j], 0);
            }else{
                p = rgb565_to_rgb888(img_buf[i*640 + j], img_buf[i*640 + j - 1]);
            }
            gl_img[i][j / 2][0] = p.red;
            gl_img[i][j / 2][1] = p.green;
            gl_img[i][j / 2][2] = p.blue;
        }
    
    }
    printf("waiting for frame_sem\n");
    sem_wait(&frame_sem);
    glutSwapBuffers();
    glutPostRedisplay();

    }
    close(client_fd);
}

int main(int argc, char** argv){

    if(argc != 2){
        printf("usage: %s ipaddr\n", argv[0]);
        return -1;
    }
    pthread_t frame_thread;
    pthread_create(&frame_thread, 0, frame_timer, (void*)argv[1]);
    pthread_t client_thread;
    pthread_create(&client_thread, 0, client_routine, (void*)argv[1]);
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(IMG_WIDTH, IMG_HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(argv[0]);
    init();
    glutDisplayFunc(display);
    glutMainLoop();
    pthread_join(client_thread, 0);
    pthread_join(frame_thread, 0);

    return 0;
}
