#include "RUDP_API.h"

int main() {
    char buffer[BUFF_SIZE] = {0};
    
    RUDP_Socket* sock = RUDP_Socket_alloc(1, SERVER_PORT, 0);

    rudp_accept(sock, buffer, BUFF_SIZE);

    return 0;
}