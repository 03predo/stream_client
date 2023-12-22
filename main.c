#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <semaphore.h>

#include "client.h"
#include "graphics.h"

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
}pixel;

sem_t frame_req;
sem_t frame_post;

uint8_t img_buf[IMG_HEIGHT*IMG_WIDTH*BYTES_PER_PIXEL];
uint8_t frame[IMG_HEIGHT][IMG_WIDTH][3];

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

void* frame_timer(void* arg){
    sem_init(&frame_req, 1, 0); 
    sem_init(&frame_post, 1, 0); 
    
    while(1){
        printf("posting frame_sem\n");
        usleep(1000000 / FPS);
        sem_post(&frame_req);
        sem_wait(&frame_post);
    }

    sem_destroy(&frame_post);
    sem_destroy(&frame_req);
}

void* client_routine(void* arg){

    client_init();
    while(1){
        client_get_frame(img_buf);
        printf("img received\n");

        for(int i = 0; i < IMG_HEIGHT; ++i){
            for(int j = 0; j < IMG_WIDTH * BYTES_PER_PIXEL; j += 2){
                pixel p;
                if(j == 0){
                    p = rgb565_to_rgb888(img_buf[i*640 + j], 0);
                }else{
                    p = rgb565_to_rgb888(img_buf[i*640 + j], img_buf[i*640 + j - 1]);
                }
                frame[i][j / 2][0] = p.red;
                frame[i][j / 2][1] = p.green;
                frame[i][j / 2][2] = p.blue;
            }
        
        }
        printf("waiting for frame_sem\n");
        sem_wait(&frame_req);
        graphics_update_frame(frame);
        sem_post(&frame_post);
    }
    client_destroy();
}

int main(int argc, char** argv){

    pthread_t frame_thread;
    pthread_create(&frame_thread, 0, frame_timer, (void*)argv[1]);
    pthread_t client_thread;
    pthread_create(&client_thread, 0, client_routine, (void*)argv[1]);
      
    graphics_start(argc, argv); 

    pthread_join(client_thread, 0);
    pthread_join(frame_thread, 0);

    return 0;
}
