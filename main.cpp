#include <glib.h>
#include <stdio.h>
#define GDK_VERSION_MIN_REQUIRED (GDK_VERSION_3_10)
#include <gdk/gdk.h>
#include <time.h>

//This gets defined by SDL and breaks the protobuf headers
#undef Status

#include "hu_uti.h"
#include "hu_aap.h"
#include "hu_uti.h"

#include "main.h"
#include "outputs.h"
#include "callbacks.h"
#include "server.h"

gst_app_t gst_app;

float g_dpi_scalefactor = 1.2f;

IHUAnyThreadInterface* g_hu = nullptr;

uint64_t get_cur_timestamp() {
        struct timespec tp;
        /* Fetch the time stamp */
        clock_gettime(CLOCK_REALTIME, &tp);

        return tp.tv_sec * 1000000 + tp.tv_nsec / 1000;
}

static int gst_loop(gst_app_t *app) {
        int ret;
        GstStateChangeReturn state_ret;

        app->loop = g_main_loop_new(NULL, FALSE);
        g_main_loop_run(app->loop);
        return ret;
}


int main(int argc, char *argv[]) {

        GOOGLE_PROTOBUF_VERIFY_VERSION;
        std::map<std::string, std::string> settings;

        hu_log_library_versions();
        // hu_install_crash_handler();

        gst_app_t *app = &gst_app;
        int ret = 0;
        errno = 0;
        pthread_t server_thread;

        gst_init(NULL, NULL);
        struct sigaction action;
        sigaction(SIGINT, NULL, &action);
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
            return 1;
        }

        sigaction(SIGINT, &action, NULL);

        gst_init(NULL, NULL);
        sigaction(SIGINT, NULL, &action);
        sigaction(SIGINT, &action, NULL);
        
        DesktopEventCallbacks callbacks;
        HUServer headunit(callbacks, settings);

        if (pthread_create(&server_thread, NULL, event_command_server, &headunit) != 0) {
            perror("Failed to create thread");
            return EXIT_FAILURE;
        }

        //loop to emulate the caar
        while(true)
        {
            /* Start AA processing */
            ret = headunit.hu_aap_start(true, true);

            if (ret < 0) {
                printf("Phone is not connected. Connect a supported phone and restart.\n");
                continue;
                // return 0;
            }

            callbacks.connected = true;
            g_hu = &headunit.GetAnyThreadInterface();

              /* Start gstreamer pipeline and main loop */
            ret = gst_loop(app);
            if (ret < 0) {
                printf("STATUS:gst_loop() ret: %d\n", ret);
            }

            callbacks.connected = false;

            /* Stop AA processing */
            ret = headunit.hu_aap_shutdown();
            if (ret < 0) {
                printf("STATUS:hu_aap_stop() ret: %d\n", ret);
                SDL_Quit();
                return (ret);
            }

            g_hu = nullptr;
        }
        
        pthread_cancel(server_thread);
        pthread_join(server_thread, NULL);
		SDL_Quit();

        return (ret);
}
