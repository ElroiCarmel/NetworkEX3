#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5060

char *util_generate_random_data(unsigned int size);

int main(int argc, char *argv[])
{

    unsigned int file_size = 1 << 21;

    int option = 1; // For user choice whether send another file
    
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    //Create the socket

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        perror("socket(2)");
        return 1;
    }

    if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) < 0) {
        perror("inet_pton(3)");
        close(sock);
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    fprintf(stdout, "Connecting to Receiver: %s:%d...\n", SERVER_IP, SERVER_PORT);

    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("connect(2)");
        close(sock);
        return 1;
    }

    //Generate random file
    char* random_file = util_generate_random_data(file_size);
    if (random_file == NULL) {
        fprintf(stdout, "Out of memory!..\n");
        return 1;
    }

    
    fprintf(stdout, "Successfully connected to Reciever!\n");
    do {
        
        fprintf(stdout, "Sending random file to server...\n");
        
        int bytes_sent = send(sock, random_file, file_size, 0);

        if (bytes_sent <= 0) {
            perror("send(2)");
            close(sock);
            free(random_file);
            return 1;
        }

        fprintf(stdout, "File transfer complete! Sent %d bytes to the server!\n", bytes_sent);

        fprintf(stdout, "\nEnter 1 to send the random file again, 0 to exit: ");

        scanf("%d", &option);

        
    } while (option != 0);
    free(random_file);
    close(sock);
    fprintf(stdout, "Connection closed...\n");
    return 0;
}

/*
* @brief A random data generator function based on srand() and rand().
* @param size The size of the data to generate (up to 2^32 bytes).
* @return A pointer to the buffer.
*/
char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;
    // Argument check.
    if (size == 0) return NULL;
    buffer = (char *)calloc(size, sizeof(char));
    // Error checking.
    if (buffer == NULL)
    return NULL;
    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
    *(buffer + i) = ((unsigned int)rand() % 256);
    return buffer;
}

