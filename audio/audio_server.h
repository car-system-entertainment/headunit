#ifndef AUDIO_SERVER_H
#define AUDIO_SERVER_H

#include <asoundlib.h>
#include <thread>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <cmath>

#include "server/server.h"
#include "hu_uti.h"
#include "hu_aap.h"

class AudioServer {
public:
    int command_read_fd; // Read end of the pipe
    int command_write_fd; // Write end of the pipe

    AudioServer();
    ~AudioServer();
    
    // media file
    void start_media_file(const char *filename);
    void stop_media_file();
    void play_media_file();
    void pause_media_file();

    // media from Radio FM

    // media from Bluetooth

    // media from AUX input

    void SetVolumeAAChannel1(double volume);
    void SetVolumeAAChannel2(double volume);

    void MediaPacketAUD(uint64_t timestamp, const byte *buf, int len);
    void MediaPacketAU1(uint64_t timestamp, const byte *buf, int len);

    void TaskAudioServer();

private:
    GstElement *pipeline;

    GstElement *aa_src_channel_1;
    GstElement *aa_src_channel_2;
    GstElement *file_src;

    GstElement *vol_aa_channel_1;
    GstElement *vol_aa_channel_2;
    GstElement *vol_file_src;

    GstElement *mixer;
    GstElement *sink;

    int pipefd[2]; // Pipe for communication between threads

    void MediaPacket(GstElement *appsrc, const byte *buf, int len);
    void set_media_volume(MediaAudioChannel channel, MediaAudioSide side, double volume);
    static void wrapper_decoder_pad_added(GstElement *decoder, GstPad *pad, gpointer data);
    void decoder_pad_added(GstElement *element, GstPad *pad, gpointer user_data);
};

#endif // AUDIO_SERVER_H
