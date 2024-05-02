#include "RUDP_API.h"

int main() {
    char buffer[BUFF_SIZE] = {0};

    RUDP_Socket* sock = RUDP_Socket_alloc(0, 0, 5);

    fprintf(stdout, "Connecting to server %s:%d..\n", SERVER_IP, SERVER_PORT);
    
    if (rudp_connect(sock, SERVER_IP, SERVER_PORT, buffer, BUFF_SIZE) == 0 ) {
        fprintf(stdout, "Failed connecting to server... exiting\n");
        rudp_close(sock);
        return 1;
    }

    char* data = util_generate_random_data(FILE_SIZE);

    char opt = 'y';

    do {

        fprintf(stdout, "Sending file to Receiver..\n");

        int bytes_sent = rudp_send(sock, data, FILE_SIZE);
        if (bytes_sent == -1) {
            fprintf(stdout, "Error sending file... exiting..\n");
            rudp_close(sock);
            return 1;
        }

        fprintf(stdout, "Success! %d bytes sent to receiver\n", bytes_sent);

        fprintf(stdout, "Send the file again? (y/n)?: ");

        fscanf(stdin, " %c", &opt);
    } while (opt == 'y');

    fprintf(stdout, "Disconnecting from Receiver..\n");

    rudp_disconnect(sock, buffer, BUFF_SIZE);

    rudp_close(sock);

    return 0;
}