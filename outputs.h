#pragma once

#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <SDL2/SDL.h>
//This gets defined by SDL and breaks the protobuf headers
#undef Status

#include "hu_uti.h"
#include "hu_aap.h"

#include "callbacks.h"

#ifndef ASPECT_RATIO_FIX
#define ASPECT_RATIO_FIX 1
#endif

struct gst_app_t;

class VideoOutput {
    GstElement *vid_pipeline = nullptr;
    GstAppSrc *vid_src = nullptr;
    GSource* timeout_src = nullptr;
    SDL_Window* window = nullptr;
    bool nightmode = false;
    DesktopEventCallbacks* callbacks;

    static gboolean bus_callback(GstBus *bus, GstMessage *message, gpointer *ptr);
    static gboolean wrapper_lvgl_bus_callback(GstBus *bus, GstMessage *message, gpointer *ptr);

    static void aa_touch_event(SDL_Window* window, HU::TouchInfo::TOUCH_ACTION action, unsigned int x, unsigned int y);
    static gboolean sdl_poll_event_wrapper(gpointer data);
    static GstFlowReturn lvgl_poll_event_wrapper(GstElement *sink, gpointer data);
    gboolean lvgl_bus_callback(GstBus *bus, GstMessage *message);

    gboolean sdl_poll_event();
    GstFlowReturn lvgl_poll_event(GstElement *sink);

public:
    VideoOutput(DesktopEventCallbacks* callbacks);
    ~VideoOutput();

    void MediaPacket(uint64_t timestamp, const byte * buf, int len);
    void SendNightMode();
};
