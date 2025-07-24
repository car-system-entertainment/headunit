#include <ctime>
#include <iostream>
#include <string>

#include "server_commands.h"


void TCPServer::aa_touch_event(HU::TouchInfo::TOUCH_ACTION action, unsigned int x, unsigned int y) {

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
        printf("data send: %d, %d\n", x, y);

        if (ret < 0) {
            printf("aa_touch_event(): hu_aap_enc_send() failed with (%d)\n", ret);
        }
    });
}

void TCPServer::run() {
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                SO_REUSEADDR | SO_REUSEPORT, &opt,
                sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        valread = read(new_socket, buffer, 5);
        if (valread < 0) break;

        if (valread == 5){
            char cmd = buffer[0];
            unsigned int pos_x = (buffer[2] << 8) + buffer[1];
            unsigned int pos_y = (buffer[4] << 8) + buffer[3];

            // printf("data received\n: %d, %d, %d, %d", windowW, windowH, pos_x, pos_y);

            if (cmd == 1) {
                aa_touch_event(HU::TouchInfo::TOUCH_ACTION_PRESS, pos_x, pos_y);
            }
            else if (cmd == 2){
                aa_touch_event(HU::TouchInfo::TOUCH_ACTION_RELEASE, pos_x, pos_y);
            }

            else if (cmd == 3){
                aa_touch_event(HU::TouchInfo::TOUCH_ACTION_DRAG, pos_x, pos_y);
            }
        }
        // send(new_socket, hello, strlen(hello), 0);
    }

    // closing the connected socket
    close(new_socket);
    shutdown(server_fd, SHUT_RDWR);
}
