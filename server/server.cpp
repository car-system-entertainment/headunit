#include "server.h"
#include "server_response.h"


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

void *task_command_server(void *args) {
    HUServer *headunit = (HUServer*) args;
    int server_fd, client;
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
        if ((client = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
            
        while (1)
        {
            // Recebendo dados do cliente
            int len = read(client, buffer, BUFFER_SIZE);
            
            if (len > 0) {
                IHUAnyThreadInterface* g_hu = nullptr;
                g_hu = &headunit->GetAnyThreadInterface();

                HeadunitMessage *headunit_message = (HeadunitMessage*) buffer;

                if (headunit_message->header.device == APP_UI && headunit_message->header.action_type == REQUEST) {
                    switch (headunit_message->header.action)
                    {
                        case EV_MOUSE_PRESS: {
                            TouchEvent *touch_event = (TouchEvent*) headunit_message->payload;
                            printf("mouse press event dx: %f, dy: %f\n", touch_event->pos_x, touch_event->pos_y);
                            aa_touch_event(headunit, HU::TouchInfo::TOUCH_ACTION_PRESS, touch_event);
                            break;
                        }

                        case EV_MOUSE_RELEASE: {
                            TouchEvent *touch_event = (TouchEvent*) headunit_message->payload;
                            printf("mouse release event dx: %f, dy: %f\n", touch_event->pos_x, touch_event->pos_y);
                            aa_touch_event(headunit, HU::TouchInfo::TOUCH_ACTION_RELEASE, touch_event);
                            break;
                        }
                        case EV_MOUSE_MOVE: {
                            TouchEvent *touch_event = (TouchEvent*) headunit_message->payload;
                            printf("mouse move event dx: %f, dy: %f\n", touch_event->pos_x, touch_event->pos_y);
                            aa_touch_event(headunit, HU::TouchInfo::TOUCH_ACTION_DRAG, touch_event);
                            break;
                        }

                        case EV_STATE_REQUEST: {
                            uint16_t len = sizeof(HeadunitMessage) + sizeof(HUStateResponse);
                            printf("request headunit state\n");
                            HeadunitMessage *response = (HeadunitMessage *) malloc(len);
                            response->header.device = APP_HU;
                            response->header.action = EV_STATE_REQUEST;
                            response->header.action_type = RESPONSE;
                            response->header.size = sizeof(HUStateResponse);

                            HUStateResponse *state = (HUStateResponse *) response->payload;
                            state->state = aa_get_status(headunit);

                            send(client, (uint8_t *) response, len, 0);
                            free(response);
                            break;
                        }

                        case EV_MEDIA_START:
                            break;

                        case EV_MEDIA_STOP:
                            break;

                        default:
                            break;
                    }
                }
            }
            else {
                close(client);
                break;
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
    }

    // Fechando o socket do servidor
    close(server_fd);
}