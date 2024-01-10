#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "log.h"
#include "client.h"


#define IMG_SIZE_BYTES (IMG_WIDTH * IMG_HEIGHT * BYTES_PER_PIXEL)
#define IMG_SEGMENT_NUM 4
#define IMG_SEGMENT_SIZE (IMG_SIZE_BYTES / IMG_SEGMENT_NUM)

#define START 0x0
#define DATA 0x1
#define RETRANSMIT 0x2
#define END 0x3

typedef struct {
    int fd;
    struct sockaddr_in server_addr;
    int server_addr_len;
}client_t;

client_t client;
uint8_t server_msg[IMG_SEGMENT_SIZE + 2];

void client_init(){
    client.fd = -1;
    client.server_addr_len = -1;

    // create socket
    client.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 70000;
    setsockopt(client.fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    // bind socket
    client.server_addr.sin_family = AF_INET,
    client.server_addr.sin_port = htons(PORT),
    client.server_addr.sin_addr.s_addr = inet_addr(SERVER_IP),

    client.server_addr_len = sizeof(client.server_addr);
 
}

void client_destroy(){
    close(client.fd);
    client.fd = -1;
}

void client_get_frame(uint8_t img_buf[IMG_HEIGHT*IMG_WIDTH*BYTES_PER_PIXEL]){
    uint8_t curr_seq_num = 0;

    // receive new message
    uint8_t msg[2];
    msg[0] = START;
    msg[1] = 0x0f;
    int bytes_sent = sendto(client.fd, msg, sizeof(msg), 0, (struct sockaddr*)&client.server_addr, client.server_addr_len);


    LOGD("waiting for new img\n");
    bool img_received = false;
    while(!img_received){
        int tmp = recvfrom(client.fd, &server_msg, IMG_SEGMENT_SIZE + 2, 0, (struct sockaddr*)&client.server_addr, &client.server_addr_len);
        if(tmp == -1){
            if(errno == EAGAIN){
                LOG("recv timeout, remaining seq_num: %#x\n", msg[1]);
                return;
            }
            LOGD("recv failed(%d)\n", errno);
            return;
        }
        switch(server_msg[0]){
            case DATA: 
                msg[1] &= ~(1 << server_msg[1]);
                LOGD("recv data with size=%d, seq_num=%u, remaining seq_num: %#x\n", tmp, server_msg[1], msg[1]);
                memcpy(img_buf + (tmp - 2) * server_msg[1], server_msg + 2, tmp - 2); 
                break;
            case END:             
                LOG("recv end, remaining seq_num: %#x\n", msg[1]);
                if(msg[1] == 0){
                    LOGD("all seq_num recvd\n");
                    img_received = true;
                    break;
                }
                //return;
            default:
                LOG("unknown msg id: %u\n", server_msg[0]);
                break;
        }
    } 
}
