#include "audio.h"
#include <cstdio>

AudioOutput::AudioOutput(const char *outDev)
{
    printf("snd_asoundlib_version: %s\n", snd_asoundlib_version());
    logw("Device name %s\n", outDev);
    int err = 0;

    if ((err = snd_pcm_open(&aud_handle, outDev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        loge("Playback open error: %s\n", snd_strerror(err));
        printf("erro ao abrir device name %s\n", outDev);
    }
    if ((err = snd_pcm_set_params(aud_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2,48000, 1, 1000000)) < 0) {   /* 1.0sec */
        loge("Playback open error: %s\n", snd_strerror(err));
    }

    if ((err = snd_pcm_prepare(aud_handle)) < 0) {
        loge("snd_pcm_prepare error: %s\n", snd_strerror(err));
    }

    if ((err = snd_pcm_open(&au1_handle, outDev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        loge("Playback open error: %s\n", snd_strerror(err));
    }
    if ((err = snd_pcm_set_params(au1_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, 16000, 1, 1000000)) < 0) {   /* 1.0sec */
        loge("Playback open error: %s\n", snd_strerror(err));
    }

    if ((err = snd_pcm_prepare(au1_handle)) < 0) {
        loge("snd_pcm_prepare error: %s\n", snd_strerror(err));
    }
}

AudioOutput::~AudioOutput()
{
    snd_pcm_close(aud_handle);
    snd_pcm_close(au1_handle);
}

void AudioOutput::MediaPacketAUD(uint64_t timestamp, const byte *buf, int len)
{
    // Do we need the timestamp?
    if (aud_handle)
    {
        MediaPacket(aud_handle, buf, len);
    }
}

void AudioOutput::MediaPacketAU1(uint64_t timestamp, const byte *buf, int len)
{
    if (au1_handle)
    {
        MediaPacket(au1_handle, buf, len);
    }
}

void AudioOutput::MediaPacket(snd_pcm_t *pcm, const byte *buf, int len)
{
    snd_pcm_sframes_t framecount = snd_pcm_bytes_to_frames(pcm, len);
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm, buf, framecount);
    if (frames < 0) {
        frames = snd_pcm_recover(pcm, frames, 1);
        if (frames < 0) {
            loge("snd_pcm_recover failed: %s\n", snd_strerror(frames));
        } else {
            frames = snd_pcm_writei(pcm, buf, framecount);
        }
    }
    if (frames >= 0 && frames < framecount) {
        loge("Short write (expected %i, wrote %i)\n", (int)framecount, (int)frames);
    }
}

GstAudioOutput::GstAudioOutput(const char *outDev)
{
    printf("GStreamer version: %s\n", gst_version_string());

    const char *pipeline_str = 
        "appsrc name=source1 is-live=true block=false do-timestamp=true ! "
        "audio/x-raw,format=S16LE,channels=2,rate=48000,layout=interleaved ! "
        "volume name=vol1 ! "
        "audioconvert ! "
        "audioresample ! "
        "queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! "
        "audiomixer name=mixer ! "
        "audioconvert ! "
        "autoaudiosink sync=false "

        "appsrc name=source2 is-live=true block=false do-timestamp=true ! "
        "audio/x-raw,format=S16LE,channels=1,rate=16000,layout=interleaved ! "
        "volume name=vol2 ! "
        "audioconvert ! "
        "audioresample ! "
        "queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! "
        "mixer.";

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str, &error);

    if (error) {
        printf("Failed to create pipeline: %s\n", error->message);
        g_error_free(error);
    }

    appsrc_aud = gst_bin_get_by_name(GST_BIN(pipeline), "source1");
    appsrc_au1 = gst_bin_get_by_name(GST_BIN(pipeline), "source2");

    volume_aud = gst_bin_get_by_name(GST_BIN(pipeline), "vol1");
    volume_au1 = gst_bin_get_by_name(GST_BIN(pipeline), "vol2");
    
    g_object_set(appsrc_aud,
        "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
        "format", GST_FORMAT_TIME,
        "is-live", TRUE,
        "block", TRUE,
        nullptr
    );

    g_object_set(appsrc_au1,
        "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
        "format", GST_FORMAT_TIME,
        "is-live", TRUE,
        "block", TRUE,
        nullptr
    );

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        printf("Unable to set the pipeline to the playing state\n");
    }

    SetVolumeAUD(1.0);
    SetVolumeAUD1(1.0);
}

void GstAudioOutput::SetVolumeAUD(double volume) {
    if (volume_aud) {
        g_object_set(volume_aud, "volume", volume, nullptr);
    }
}

void GstAudioOutput::SetVolumeAUD1(double volume) {
    if (volume_au1) {
        g_object_set(volume_au1, "volume", volume, nullptr);
    }
}

GstAudioOutput::~GstAudioOutput()
{
    // Stop and cleanup pipelines
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    if (appsrc_au1) {
        gst_element_set_state(appsrc_au1, GST_STATE_NULL);
        gst_object_unref(appsrc_au1);
    }
    if (appsrc_aud) {
        gst_element_set_state(appsrc_aud, GST_STATE_NULL);
        gst_object_unref(appsrc_aud);
    }
    if (volume_au1) {
        gst_object_unref(volume_au1);
    }
    if (volume_aud) {
        gst_object_unref(volume_aud);
    }
}

void GstAudioOutput::MediaPacketAUD(uint64_t timestamp, const byte *buf, int len)
{
    if (appsrc_aud) {
        MediaPacket(appsrc_aud, buf, len);
    }
}

void GstAudioOutput::MediaPacketAU1(uint64_t timestamp, const byte *buf, int len)
{
    if (appsrc_au1) {
        MediaPacket(appsrc_au1, buf, len);
    }
}

void GstAudioOutput::MediaPacket(GstElement *appsrc, const byte *buf, int len)
{
    GstBuffer *gst_buffer = gst_buffer_new_allocate(nullptr, len, nullptr);
    gst_buffer_fill(gst_buffer, 0, buf, len);
    
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), gst_buffer);
    if (ret != GST_FLOW_OK) {
        loge("Failed to push buffer: %d\n", ret);
    }
}

snd_pcm_sframes_t MicInput::read_mic_cancelable(snd_pcm_t* mic_handle, void *buffer, snd_pcm_uframes_t size, bool* canceled)
{
    int pollfdAllocCount = snd_pcm_poll_descriptors_count(mic_handle);
    struct pollfd* pfds = (struct pollfd*)alloca((pollfdAllocCount + 1) * sizeof(struct pollfd));
    unsigned short* revents = (unsigned short*)alloca(pollfdAllocCount * sizeof(unsigned short));

    int polldescs = snd_pcm_poll_descriptors(mic_handle, pfds, pollfdAllocCount);
    pfds[polldescs].fd = cancelPipeRead;
    pfds[polldescs].events = POLLIN;
    pfds[polldescs].revents = 0;

    *canceled = false;
    while (true)
    {
        if (poll(pfds, polldescs+1, -1) <= 0)
        {
            loge("poll failed");
            break;
        }

        if (pfds[polldescs].revents & POLLIN)
        {
            unsigned char bogusByte;
            read(cancelPipeRead, &bogusByte, 1);

            *canceled = true;
            return 0;
        }
        unsigned short audioEvents = 0;
        snd_pcm_poll_descriptors_revents(mic_handle, pfds, polldescs, &audioEvents);

        if (audioEvents & POLLIN)
        {
            //got it
            break;
        }
    }
    snd_pcm_sframes_t ret = snd_pcm_readi(mic_handle, buffer, size);
    return ret;
}

void MicInput::MicThreadMain(IHUAnyThreadInterface* threadInterface)
{
    pthread_setname_np(pthread_self(), "mic_thread");

    snd_pcm_t* mic_handle = nullptr;

    int err = 0;
    if ((err = snd_pcm_open(&mic_handle, micDevice.c_str(), SND_PCM_STREAM_CAPTURE,  SND_PCM_NONBLOCK)) < 0)
    {
        loge("Playback open error: %s\n", snd_strerror(err));
        return;
    }

    if ((err = snd_pcm_set_params(mic_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1,16000, 1, 250000)) < 0)
    {   /* 0.25sec */
        loge("Playback open error: %s\n", snd_strerror(err));
        snd_pcm_close(mic_handle);
        return;
    }
    if ((err = snd_pcm_prepare(mic_handle)) < 0)
    {
        loge("snd_pcm_prepare: %s\n", snd_strerror(err));
        snd_pcm_close(mic_handle);
        return;
    }

    if ((err = snd_pcm_start(mic_handle)) < 0)
    {
        loge("snd_pcm_start: %s\n", snd_strerror(err));
        snd_pcm_close(mic_handle);
        return;
    }

    const size_t tempSize = 1024*1024;
    const snd_pcm_sframes_t bufferFrameCount = snd_pcm_bytes_to_frames(mic_handle, tempSize);
    bool canceled = false;
    while(!canceled)
    {
        uint8_t* tempBuffer = new uint8_t[tempSize];
        snd_pcm_sframes_t frames = read_mic_cancelable(mic_handle, tempBuffer, bufferFrameCount, &canceled);
        if (frames < 0)
        {
            if (frames == -ESTRPIPE)
            {
                frames = snd_pcm_recover(mic_handle, frames, 0);
                if (frames < 0)
                {
                    loge("recover failed");
                }
                else
                {
                    frames = read_mic_cancelable(mic_handle, tempBuffer, bufferFrameCount, &canceled);
                }
            }

            if (frames < 0)
            {
                delete [] tempBuffer;
                canceled = true;
            }
        }
        ssize_t bytesRead = snd_pcm_frames_to_bytes(mic_handle, frames);
        threadInterface->hu_queue_command([tempBuffer, bytesRead](IHUConnectionThreadInterface& s)
        {
            //doesn't seem like the timestamp is used so pass 0
            s.hu_aap_enc_send_media_packet(1, AA_CH_MIC, HU_PROTOCOL_MESSAGE::MediaDataWithTimestamp, 0, tempBuffer, bytesRead);
            delete [] tempBuffer;
        });
    }

    if ((err = snd_pcm_drop(mic_handle)) < 0)
    {
        loge("snd_pcm_drop: %s\n", snd_strerror(err));
    }

    snd_pcm_close(mic_handle);
}

MicInput::MicInput(const std::string& micDevice) : micDevice(micDevice)
{
    int cancelPipe[2];
    if (pipe(cancelPipe) < 0)
    {
        loge("pipe failed");
    }
    cancelPipeRead = cancelPipe[0];
    cancelPipeWrite = cancelPipe[1];
}

MicInput::~MicInput()
{
    Stop();

    close(cancelPipeRead);
    close(cancelPipeWrite);
}

void MicInput::Start(IHUAnyThreadInterface* threadInterface)
{
    if (!mic_readthread.joinable())
    {
        mic_readthread = std::thread([this, threadInterface](){ MicThreadMain(threadInterface);});
    }
}

void MicInput::Stop()
{
    if (mic_readthread.joinable())
    {
        //write single byte
        write(cancelPipeWrite, &cancelPipeWrite, 1);
        mic_readthread.join();
    }
}
