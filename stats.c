#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#include "log.h"
#include "client.h"

#define SAMPLE_NUM 100

uint8_t img_buf[IMG_HEIGHT*IMG_WIDTH*BYTES_PER_PIXEL];
double sum;

int main(){
    
    printf("starting sampling\n");
    client_init();
    struct timeval  tv1, tv2;
    double time_elapsed;
    for(int i = 0; i < SAMPLE_NUM; ++i){
        gettimeofday(&tv1, NULL);
        client_get_frame(img_buf);
        gettimeofday(&tv2, NULL);
        time_elapsed = (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
        sum += time_elapsed;
        if(time_elapsed < 0.1){ 
            usleep(100000 - time_elapsed*100000);
        }
    }
    printf("average of %d frames: %f sec\n", SAMPLE_NUM, (sum / SAMPLE_NUM));

}
