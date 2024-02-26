#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <time.h>

#define SERVER_PORT 5060
#define BUFF_SIZE 1024

int main(int argc, char* argv[]) {

    struct sockaddr_in client;
    struct sockaddr_in server;

    socklen_t client_len = sizeof(client);

    memset(&client, 0, sizeof(client));
    memset(&server, 0, sizeof(server));


    int opt = 1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1)
    {
        perror("socket(2)");
        return 1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsocketopt(2)");
        close(sock);
        return 1;
    }

    server.sin_addr.s_addr = INADDR_ANY;

    server.sin_family = AF_INET;

    server.sin_port = htons(SERVER_PORT);

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind(2)");
        close(sock);
        return 1;
    }

    if (listen(sock, 1) < 0)
    {
        perror("listen(2)");
        close(sock);
        return 1;
    }

    fprintf(stdout, "Listening for incoming connections on port %d...\n", SERVER_PORT);

    while (1)
    {
        int client_socket = accept(sock, (struct sockaddr *)&client, &client_len);

        if (client_socket  < 0)
        {
            perror("accept(2)");
            close(sock);
            return 1;
        }

        fprintf(stdout, "Sender %s:%d connected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        
        // Keep recv till the client closed the socket

        int bytes_recv = 1; unsigned int total_recv = 0, expected_total = 1<<21; //Keep track of bytes
        char buffer[BUFF_SIZE] = {0}; // Space to save the data we got

        while (bytes_recv) // 
        {
            bytes_recv = recv(client_socket, buffer, BUFF_SIZE, 0);

            if (bytes_recv < 0)
            {
                perror("recv(2)");
                close(client_socket);
                close(sock);
                return 1;
            }           
            else {
                total_recv += bytes_recv;
                if (total_recv >= expected_total) //Meaning file transfer completed
                {
                    // Deal with statistics

                    total_recv-=expected_total; // Reset count
                } 
            }
        }

        // Close the connection
        close(client_socket);

        fprintf(stdout, "Sender %s:%d disconnected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        
    }

    close(sock);
    fprintf(stdout, "Server finished!\n");
    return 0;
}