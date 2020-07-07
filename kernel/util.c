#include "hardware.h"

#define FONT_WIDTH 8
#define FONT_HEIGHT 8

extern unsigned char font[128][FONT_HEIGHT];

struct FrameBuffer *FB;

unsigned int fb_x = 0;
unsigned int fb_y = 0;


void init_frame_buffer(struct FrameBuffer *fb) {
    FB = fb;
    fb_x = 0;
    fb_y = 0;
    struct Pixel *pixel;
    for (unsigned int i = 0; i < fb->height; i++) {
        for (unsigned int j = 0; j < fb->width; j++) {
            pixel = fb->base + fb->width * i + j;
            pixel->r = 0;
            pixel->g = 0;
            pixel->b = 0;
            pixel->_reserved = 1;
        }
    }
}

static void putc(char c) {
    //make newline and return when '\n' or '\r' is given.
    if (c == '\n' || c == '\r') {
        fb_x = 0;
        fb_y += FONT_HEIGHT;
        return;
    }
    //make newline when it comes to the right edge of the screen.
    if (fb_x + FONT_WIDTH > FB->width) {
        fb_x = 0;
        fb_y += FONT_HEIGHT;
    }
    //flush the screen when it comes to the bottom of the screen.
    if (fb_y + FONT_HEIGHT > FB->height) {
        init_frame_buffer(FB);
    }
    struct Pixel *current_base = FB->base + FB->width * fb_y + fb_x;
    struct Pixel *pixel;
    unsigned char *single_font = font[(int)c];

    for (unsigned int i = 0; i < FONT_HEIGHT; i++) {
        for (unsigned int j = 0; j < FONT_WIDTH; j++) {
            pixel = current_base + FB->width * i + j;
            if ((single_font[i] >> (FONT_WIDTH - 1 - j)) % 2 == 1) {
                pixel->r = 255;
                pixel->g = 255;
                pixel->b = 255;
            }
        }
    }
    fb_x += FONT_WIDTH;
}

void puts(char *str) {
    unsigned int i = 0;
    while (str[i] != '\0') {
        putc(str[i]);
        i++;
    }
}

extern void hello() {
  puts("hello!\n");
  return;
}

void puth(unsigned long long value, unsigned char digits_len) {
    
    for (unsigned char current_len = digits_len; current_len > 0; current_len--) {
        unsigned long long divider = 1;

        for (unsigned int i = 1; i < current_len; i++) {
            divider = 16 * divider;
        }

        unsigned int hexadecimal = value / divider % 16;

        if (hexadecimal < 10) {
            putc((char)(hexadecimal + 48));
        }
        else {
            putc((char)(hexadecimal + 87));
        }
    }
    putc('\n');
}


//compare two strings by given length and return 0 when they matches and return 1 when they don't.
int strcmp(const char* a, const char* b, unsigned int cmp_length) {    
    int flag = 0;
    for (unsigned int i = 0; i < cmp_length; i++) {
        if (a[i] != b[i]) {
            flag = 1;
            break;
        }
    }
    return flag;
}