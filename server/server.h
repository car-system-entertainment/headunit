#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "hu_uti.h"
#include "hu_aap.h"
#include "hu_uti.h"
#include "main.h"

#pragma pack(1)

typedef enum ACTION {
    EV_MOUSE_PRESS = 0x01,
    EV_MOUSE_RELEASE,
    EV_MOUSE_MOVE,
    EV_STATE_REQUEST,   // request headunit state
    EV_MEDIA_START,     // start media playback with path of the file
    EV_MEDIA_PLAY,      // play actual media
    EV_MEDIA_PAUSE,     // pause actual media
    EV_MEDIA_STOP,      // stop media playback
    EV_MEDIA_SET_VOL,   // set media volume
} ACTION;

typedef enum ACTION_TYPE {
    REQUEST = 0x01,
    RESPONSE,
} ACTION_TYPE;

typedef enum DEVICE {
    APP_UI = 0x55,
    APP_HU = 0xAA,
} DEVICE;

typedef enum MediaAudioSide {
    EV_MEDIA_AUDIO_FRONT = 0x01,
    EV_MEDIA_AUDIO_BACK,
    EV_MEDIA_AUDIO_BOTH,
} MediaAudioSide;

typedef enum MediaAudioChannel {
    EV_MEDIA_AUDIO_CHANNEL_LEFT = 0x01,
    EV_MEDIA_AUDIO_CHANNEL_RIGHT,
} MediaAudioChannel;

typedef struct HeadunitHeader {
    DEVICE device;
    ACTION action;
    ACTION_TYPE action_type;
    uint16_t size;
} HeadunitHeader;

typedef struct HeadunitMessage {
    HeadunitHeader header;
    uint8_t payload[];
} ServerRequest;

typedef struct TouchEvent {
    uint16_t width;
    uint16_t heigth;
    float pos_x;
    float pos_y;
} TouchEvent;

typedef struct EVMediaStartRequest {
    char path[1024];
} EvMediaStartRequest;

typedef struct EVSetMediaVolumeRequest {
    MediaAudioChannel channel;
    MediaAudioSide side; // front or back all
    double volume;
} EVSetMediaVolumeRequest;

void *task_command_server(void *arg);

#endif  // SERVER_H