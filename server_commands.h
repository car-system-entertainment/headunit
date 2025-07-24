#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "hu_aap.h"
#include "main.h"

#define PORT 1234


class TCPServer {
  public:
    void run();
    
    IHUAnyThreadInterface* g_hu = nullptr;

  private:
    void aa_touch_event(HU::TouchInfo::TOUCH_ACTION action, unsigned int x, unsigned int y);
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    unsigned char buffer[1024] = { 0 };
};
