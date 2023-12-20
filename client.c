#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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
#define IMG_SIZE_BYTES (IMG_WIDTH * IMG_HEIGHT * BYTES_PER_PIXEL)

#define IMG_SEGMENT_NUM 4
#define IMG_SEGMENT_SIZE (IMG_SIZE_BYTES / IMG_SEGMENT_NUM)

#define START 0x0
#define DATA 0x1
#define RETRANSMIT 0x2
#define END 0x3

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
}pixel;

sem_t frame_req;
sem_t frame_post;


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
    sem_init(&frame_req, 1, 0); 
    sem_init(&frame_post, 1, 0); 
    
    while(1){
        printf("posting frame_sem\n");
        usleep(1000000);
        sem_post(&frame_req);
        sem_wait(&frame_post);
    }

    sem_destroy(&frame_post);
    sem_destroy(&frame_req);
}

void* client_routine(void* arg){
    char* ip_addr = (char*)arg;
    uint8_t confirm = 0x0f;
    // create socket
    int client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    // bind socket
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = inet_addr(ip_addr),
    };
    int server_addr_len = sizeof(server_addr);
 

    uint8_t server_msg[IMG_SEGMENT_SIZE + 2];
    uint8_t curr_seq_num = 0;
    while(1){

        // receive new message
        uint8_t msg[2];
        msg[0] = START;
        msg[1] = 0x0f;
        int bytes_sent = sendto(client_fd, msg, sizeof(msg), 0, (struct sockaddr*)&server_addr, server_addr_len);


        printf("waiting for new img\n");
        bool img_received = false;
        while(!img_received){
            int tmp = recvfrom(client_fd, &server_msg, IMG_SEGMENT_SIZE + 2, 0, (struct sockaddr*)&server_addr, &server_addr_len);
            if(tmp == -1){
                if(errno == EAGAIN){
                    msg[0] = RETRANSMIT;
                    sendto(client_fd, msg, 2, 0, (struct sockaddr*)&server_addr, server_addr_len);
                    continue;
                }
                printf("recv failed(%d)\n", errno);
                return NULL;
            }
            switch(server_msg[0]){
                case DATA:
                    
                    msg[1] &= ~(1 << server_msg[1]);
                    printf("recv data with size=%d, seq_num=%u, remaining seq_num: %#x\n", tmp, server_msg[1], msg[1]);
                    memcpy(img_buf + (tmp - 2) * server_msg[1], server_msg + 2, tmp - 2);
                    break;
                case END:
                    if(msg[1] == 0){
                        printf("recv end, all seq_num recvd\n");
                        img_received = true;
                        break;
                    }
                    printf("recv end, remaining seq_num: %#x\n", msg[1]);
                    msg[0] = RETRANSMIT;
                    sendto(client_fd, msg, 2, 0, (struct sockaddr*)&server_addr, server_addr_len);
                    break;
                default:
                    printf("unknown msg id: %u", server_msg[0]);
                    break;
            }
        }

        printf("img received\n");

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
        sem_wait(&frame_req);
        glutSwapBuffers();
        glutPostRedisplay();

        sem_post(&frame_post);
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
