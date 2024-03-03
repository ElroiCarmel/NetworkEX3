#include "RUDP_API.h"

int main() {
    char buffer[BUFF_SIZE] = {0};

    RUDP_Socket* sock = RUDP_Socket_alloc(0, 0, 5);
    rudp_connect(sock, SERVER_IP, SERVER_PORT, buffer, BUFF_SIZE);

    
    return 0;
}