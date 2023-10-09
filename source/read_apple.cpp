#include "read_apple.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <cassert>

// #define READ_APPLE_MAIN
#define APPLE_FILE "../../assets/apple.bin"
#define FRAME_NO 200
#define WIDTH 640
#define HEIGHT 360
#define FRAME_SIZE WIDTH* HEIGHT / 8
#define N_FRAMES 6574

void apple_open(struct Apple* a) {
    a->f = fopen(APPLE_FILE, "rb");
    assert(nullptr != a->f);
    a->frame_size = FRAME_SIZE;
    a->frame_count = N_FRAMES;
    a->w = WIDTH;
    a->h = HEIGHT;
}

void apple_read_frame(struct Apple* a, char* buf, int i) {
    fseek(a->f, FRAME_SIZE * i, SEEK_SET);
    int read = fread(buf, sizeof(char), sizeof(char) * FRAME_SIZE, a->f);
}

int apple_get_pixel(char* buf, int x, int y) {
    int idx = ((WIDTH * y / 8) + (x / 8));
    int bit = 7 - (x % 8);
    char b = buf[idx];
    return b & (1 << bit);
}

void apple_close(struct Apple* a) {
    fclose(a->f);
}

#ifdef READ_APPLE_MAIN
int main(int argc, char* argv[]) {
    struct Apple a;
    apple_open(&a);
    if (NULL == a.f) {
        return 1;
    }

    char frame[FRAME_SIZE];
    for (int i = 0; i < N_FRAMES; ++i) {
        apple_read_frame(&a, frame, i);
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                if (apple_get_pixel(frame, x, y) > 0) {
                    printf("X");
                } else {
                    printf(" ");
                }
            }
            printf("\n");
        }
        // print_frame(frame);
        // printf("read %d, should be %d\n", read, FRAME_SIZE);
        usleep(1000000.0f / 15.0f);
    }

    apple_close(&a);
}
#endif  // READ_APPLE_MAIN
