#include "audio_server.h"


AudioServer::AudioServer()
{
    printf("GStreamer version: %s\n", gst_version_string());
    pipe(pipefd);
    command_read_fd = pipefd[0];
    command_write_fd = pipefd[1];

    const char *pipeline_str = 
        "appsrc name=aa_src_1 is-live=true block=false do-timestamp=true ! "
        "audio/x-raw,format=S16LE,channels=2,rate=48000,layout=interleaved ! "
        "volume name=aa_vol_1 ! "
        "audioconvert ! "
        "audioresample ! "
        "queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! "
        "audiomixer name=mixer ! "
        "audioconvert ! "
        "autoaudiosink sync=false "

        "appsrc name=aa_src_2 is-live=true block=false do-timestamp=true ! "
        "audio/x-raw,format=S16LE,channels=1,rate=16000,layout=interleaved ! "
        "volume name=aa_vol_2 ! "
        "audioconvert ! "
        "audioresample ! "
        "queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! "
        "mixer. "

        "filesrc name=file_src location=/dev/null ! "
        "decodebin name=decoder ! "
        "audioconvert ! "
        "audioresample ! "
        "volume name=file_vol ! "
        "queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! "
        "mixer.";


    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str, &error);

    if (error) {
        printf("Failed to create pipeline: %s\n", error->message);
        g_error_free(error);
    }

    aa_src_channel_1 = gst_bin_get_by_name(GST_BIN(pipeline), "aa_src_1");
    aa_src_channel_2 = gst_bin_get_by_name(GST_BIN(pipeline), "aa_src_2");
    file_src = gst_bin_get_by_name(GST_BIN(pipeline), "file_src");

    vol_aa_channel_1 = gst_bin_get_by_name(GST_BIN(pipeline), "aa_vol_1");
    vol_aa_channel_2 = gst_bin_get_by_name(GST_BIN(pipeline), "aa_vol_2");
    vol_file_src = gst_bin_get_by_name(GST_BIN(pipeline), "file_vol");

    g_object_set(aa_src_channel_1,
        "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
        "format", GST_FORMAT_TIME,
        "is-live", TRUE,
        "block", TRUE,
        nullptr
    );

    g_object_set(aa_src_channel_2,
        "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
        "format", GST_FORMAT_TIME,
        "is-live", TRUE,
        "block", TRUE,
        nullptr
    );

    GstElement *decoder = gst_bin_get_by_name(GST_BIN(pipeline), "decoder");
    mixer = gst_bin_get_by_name(GST_BIN(pipeline), "mixer");

    g_signal_connect(decoder, "pad-added", G_CALLBACK(wrapper_decoder_pad_added), this);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        printf("Unable to set the pipeline to the playing state\n");
    }
}

void AudioServer::wrapper_decoder_pad_added(GstElement *decoder, GstPad *pad, gpointer data) {
    static_cast<AudioServer*>(data)->decoder_pad_added(decoder, pad, NULL);
}

void AudioServer::decoder_pad_added(GstElement *decoder, GstPad *pad, gpointer user_data) {
    GstPad *sinkpad = gst_element_get_static_pad(mixer, "sink");
    GstPadLinkReturn ret = gst_pad_link(pad, sinkpad);

    if (ret != GST_PAD_LINK_OK) {
        printf("Failed to link decoder's pad to the mixer's pad\n");
    }

    gst_object_unref(sinkpad);
}

void AudioServer::MediaPacketAUD(uint64_t timestamp, const byte *buf, int len)
{
    if (aa_src_channel_1) {
        MediaPacket(aa_src_channel_1, buf, len);
    }
}

void AudioServer::MediaPacketAU1(uint64_t timestamp, const byte *buf, int len)
{
    if (aa_src_channel_2) {
        MediaPacket(aa_src_channel_2, buf, len);
    }
}

void AudioServer::MediaPacket(GstElement *appsrc, const byte *buf, int len)
{
    GstBuffer *gst_buffer = gst_buffer_new_allocate(nullptr, len, nullptr);
    gst_buffer_fill(gst_buffer, 0, buf, len);
    
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), gst_buffer);
    if (ret != GST_FLOW_OK) {
        loge("Failed to push buffer: %d\n", ret);
    }
}

AudioServer::~AudioServer()
{
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    if (aa_src_channel_1) {
        gst_element_set_state(aa_src_channel_1, GST_STATE_NULL);
        gst_object_unref(aa_src_channel_1);
    }
    if (aa_src_channel_2) {
        gst_element_set_state(aa_src_channel_2, GST_STATE_NULL);
        gst_object_unref(aa_src_channel_2);
    }
    if (vol_aa_channel_1) gst_object_unref(vol_aa_channel_1);
    if (vol_aa_channel_2) gst_object_unref(vol_aa_channel_2);
    if (mixer) gst_object_unref(mixer);
}

void AudioServer::SetVolumeAAChannel1(double volume)
{
    if (vol_aa_channel_1) {
        g_object_set(vol_aa_channel_1, "volume", volume, nullptr);
    }
}

void AudioServer::SetVolumeAAChannel2(double volume)
{
    if (vol_aa_channel_2) {
        g_object_set(vol_aa_channel_2, "volume", volume, nullptr);
    }
}

void AudioServer::start_media_file(const char *filename) {
    // pausa os outros fluxos
    if (aa_src_channel_1) {
        gst_element_set_state(aa_src_channel_1, GST_STATE_READY);
    }

    // não pausa o canal 2 pq é o audio de navegação, verificar depois

    if (file_src) {
        gst_element_set_state(file_src, GST_STATE_READY);
        g_object_set(file_src, "location", filename, nullptr);
        gst_element_set_state(file_src, GST_STATE_PLAYING);
        gst_object_unref(file_src);
    }
}

void AudioServer::stop_media_file() {
    if (file_src) {
        gst_element_set_state(file_src, GST_STATE_READY);
    }
}

void AudioServer::play_media_file() {
    if (file_src) {
        gst_element_set_state(file_src, GST_STATE_PLAYING);
    }
}

void AudioServer::pause_media_file() {
    if (file_src) {
        gst_element_set_state(file_src, GST_STATE_PAUSED);
    }
}

void AudioServer::set_media_volume(MediaAudioChannel channel, MediaAudioSide side, double volume) {
    if (vol_file_src) {
        g_object_set(vol_file_src, "volume", volume, nullptr);
    }    
}

void AudioServer::TaskAudioServer()
{
    // while (true)
    // {   
    //     uint8_t buffer[1024];
    //     uint16_t ret = read(command_read_fd, buffer, sizeof(buffer));
        
    //     if (ret) {
    //         ServerRequest *request = (ServerRequest *) buffer;
            
    //         switch (request->action) {
    //             case EV_MEDIA_START: {
    //                 EVMediaStartRequest *media_start_request = (EVMediaStartRequest *) request->payload;
    //                 start_media_file(media_start_request->path);
    //                 break;
    //             }

    //             case EV_MEDIA_STOP:
    //                 stop_media_file();
    //             break;

    //             case EV_MEDIA_PLAY:
    //                 play_media_file();
    //             break;

    //             case EV_MEDIA_PAUSE:
    //                 pause_media_file();
    //             break;

    //             case EV_MEDIA_SET_VOL: {
    //                 EVSetMediaVolumeRequest *media_set_volume_request = (EVSetMediaVolumeRequest *) request->payload;
    //                 // set_media_volume(
    //                 //     media_set_volume_request->channel,
    //                 //     media_set_volume_request->side,
    //                 //     media_set_volume_request->volume
    //                 // );
    //                 // Handle media set volume
    //                 break;
    //             }

    //             default:
    //                 break;
    //         }
    //     }
    // }   
}