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
    /*
    For saving the statistics data.
    Time in seconds, btyes in bytes
    */
    double *time_storage, *speed_storage; 
    size_t strg_size=0, strg_capacity=1;
    time_storage = (double*)malloc(sizeof(double));
    if (time_storage==NULL) {
        printf("Out of memory error...\n");
        return 1;
    }
    speed_storage = (double*)malloc(sizeof(double));
    if (speed_storage==NULL) {
        printf("Out of memory error...\n");
        return 1;
    }
    
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

        int bytes_recv = 1; size_t total_recv = 0, expected_total = 1<<21; //Keep track of bytes
        char buffer[BUFF_SIZE] = {0}; // Space to save the data we got
        clock_t start=clock();; // For statistics
        
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
                    
                    if (strg_size==strg_capacity)
                    {
                        time_storage = (double*) realloc(time_storage, strg_capacity*2*sizeof(double));
                        if (time_storage==NULL) {
                        free(time_storage);
                        printf("Out of memory error...\n");
                        return 1;
                    }

                        speed_storage = (double*) realloc(speed_storage, strg_capacity*2*sizeof(double));
                        if (speed_storage==NULL) {
                            free(speed_storage);
                            printf("Out of memory error...\n");
                            return 1;
                        }     
                        strg_capacity *= 2;
                    }
                    
                    // Deal with statistics
                    double t = (double) (clock() - start) / CLOCKS_PER_SEC;
                    time_storage[strg_size] = t * 1e3; //  save in ms
                    speed_storage[strg_size] = (double) (total_recv / t)/1e6; //save in MB/s
                    strg_size++;
                    fprintf(stdout, "File transfer completed, %lf bytes received\n", (double) total_recv);
                    fprintf(stdout, "Time for receiveing file: %lf ms\n", t*1000);
                    fprintf(stdout, "Speed: %lf bytes/s\n", total_recv/t);
                    // Print
                    fprintf(stdout,"Waiting for Sender response..\n\n");
                    total_recv -= expected_total; // Reset count
                    start = clock();
                } 
            }
        }
        //Sender probably closed the conncetion
        fprintf(stdout, "Sender sent exit message...\n");
        fprintf(stdout, "Sender %s:%d disconnected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        //Print stattitics
        fprintf(stdout, "***************************************\n");
        size_t i;
        double time_sum = 0, speed_sum = 0;
        for (i = 0; i < strg_size; i++) {
            double t = time_storage[i];
            double speed = speed_storage[i];
            fprintf(stdout, "Run #%ld Data:     Time = %.4lfms, Speed: %.2lfMB/s\n", i, t, speed);
            time_sum += t; speed_sum += speed;
        }
        fprintf(stdout, "Average time: %.4lf ms\nAverage speed: %.2lf MB/s\n", time_sum/strg_size, speed_sum/strg_size);
        // Close the connection
        close(client_socket);
        
        break;
    }
    
    //print stats


    close(sock);
    free(speed_storage); free(time_storage);
    fprintf(stdout, "Server finished!\n");
    return 0;
}