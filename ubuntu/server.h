#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned char action;
    uint16_t width;
    uint16_t heigth;
    float pos_x;
    float pos_y;
} TouchEvent;

void *event_command_server(void *arg);
