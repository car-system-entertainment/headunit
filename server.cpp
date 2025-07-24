#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "hu_uti.h"
#include "hu_aap.h"
#include "hu_uti.h"
#include "server.h"
#include "main.h"

#define PORT 5000
#define BUFFER_SIZE 1024


void aa_touch_event(HUServer *headunit, HU::TouchInfo::TOUCH_ACTION action, TouchEvent *touch_event) {
    IHUAnyThreadInterface* g_hu = nullptr;
    g_hu = &headunit->GetAnyThreadInterface();

    float normx = touch_event->pos_x / float(touch_event->width);
    float normy = touch_event->pos_y / float(touch_event->heigth);

    unsigned int x = (unsigned int) (normx * 1280);
    unsigned int y = (unsigned int) (normy * 720);

    g_hu->hu_queue_command([action, x, y](IHUConnectionThreadInterface & s) {
        HU::InputEvent inputEvent;
        inputEvent.set_timestamp(get_cur_timestamp());
        HU::TouchInfo* touchEvent = inputEvent.mutable_touch();
        touchEvent->set_action(action);
        HU::TouchInfo::Location* touchLocation = touchEvent->add_location();
        touchLocation->set_x(x);
        touchLocation->set_y(y);
        touchLocation->set_pointer_id(0);

        /* Send touch event */

        int ret = s.hu_aap_enc_send_message(0, AA_CH_TOU, HU_INPUT_CHANNEL_MESSAGE::InputEvent, inputEvent);
        if (ret < 0) {
            printf("aa_touch_event(): hu_aap_enc_send() failed with (%d)\n", ret);
        }
    });
}

HU_STATE aa_get_status(HUServer *headunit) {
    return headunit->hu_app_get_state();
}

void *event_command_server(void *args) {
    HUServer *headunit = (HUServer*) args;
    int server_fd, new_socket;
    struct sockaddr_in address;

    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Criando o socket do servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurando o socket para reutilizar o endereço
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configurando o endereço do servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Aceita conexões de qualquer endereço
    address.sin_port = htons(PORT);

    // Vinculando o socket ao endereço e porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Colocando o socket no modo de escuta
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("TCP server started\n");

    while (1) {
        // Aceitando uma nova conexão
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
            
        while (1)
        {
            // Recebendo dados do cliente
            int len = read(new_socket, buffer, BUFFER_SIZE);
            
            if (len > 0) {
                IHUAnyThreadInterface* g_hu = nullptr;
                g_hu = &headunit->GetAnyThreadInterface();

                if (buffer[0] == 0x55 && buffer[1] == 0x01 && ((buffer[3] << 8) + buffer[2] == 0x0D)) {
                    TouchEvent touch_event;
                    touch_event.action = buffer[4];
                        
                    memcpy(&touch_event.width, &buffer[5], sizeof(uint16_t));
                    memcpy(&touch_event.heigth, &buffer[7], sizeof(uint16_t));
                    memcpy(&touch_event.pos_x, &buffer[9], sizeof(float));
                    memcpy(&touch_event.pos_y, &buffer[13], sizeof(float));

                    // mouse down
                    if (touch_event.action == 0x01) {
                        printf("mouse press event: dx -> %f, dy -> %f\n", touch_event.pos_x, touch_event.pos_y);
                        aa_touch_event(headunit, HU::TouchInfo::TOUCH_ACTION_PRESS, &touch_event);
                    }
                    // mouse release
                    else if (touch_event.action == 0x02)
                    {
                        printf("mouse release event: dx -> %f, dy -> %f\n", touch_event.pos_x, touch_event.pos_y);
                        aa_touch_event(headunit, HU::TouchInfo::TOUCH_ACTION_RELEASE, &touch_event);
                    }
                    //mouse drag
                    else if (touch_event.action == 0x03)
                    {
                        printf("mouse drag event: dx -> %f, dy -> %f\n", touch_event.pos_x, touch_event.pos_y);
                        aa_touch_event(headunit, HU::TouchInfo::TOUCH_ACTION_DRAG, &touch_event);
                    }
                }
                else if (buffer[0] == 0x55 && buffer[1] == 0x02 && g_hu) {
                    HU_STATE state = aa_get_status(headunit);
                    unsigned char message[5];
                    message[0] = 0x55; // header
                    message[1] = 0x01; // status
                    message[2] = 0x00; // MSB payload
                    message[3] = 0x01; // LSB
                    message[4] = state;
                    
                    send(new_socket, &message, sizeof(message), 0);
                }
            }
            else {
                close(new_socket);
                break;
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
    }

    // Fechando o socket do servidor
    close(server_fd);
}