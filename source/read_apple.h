#ifndef READ_APPLE_H
#define READ_APPLE_H

#include <stdio.h>

struct Apple {
    FILE* f;
    int frame_size;
    int frame_count;
    int w;
    int h;
};

void apple_open(Apple* a);
void apple_read_frame(Apple* a, char* buf, int i);
int apple_get_pixel(char* buf, int x, int y);
void apple_close(Apple* a);

#endif  // READ_APPLE_H
