#include <stdint.h>

void client_get_frame(uint8_t img_buf[IMG_HEIGHT*IMG_WIDTH*BYTES_PER_PIXEL]);
void client_init();
void client_destroy();
