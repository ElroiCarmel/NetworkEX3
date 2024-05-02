#ifndef RUDP_API
#define RUDP_API 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define MAX_DGRAM_SIZE 65507
#define BUFF_SIZE 65600
#define FILE_SIZE (1<<21)

#define SERVER_PORT 5060
#define SERVER_IP "127.0.0.1"

#define ACK 1
#define SIN 2
#define FIN 4
#define STOF 8 // Start of file transfer
#define ENOF 16 // End of file transfer

struct _rudp_socket {
    int socket_fd; // UDP socket file descriptor
    int isServer; // True if the RUDP socket acts like a server, false if client
    int isConnected; // True if the client connected to the server and vice versa
    struct sockaddr_in dest_adrr; /* Destination address, Client fills it when it connects via rudp_connect()
                                     server fills it when it receives connection via rudp_accept() */
};

struct _rudp_header {
    unsigned short checksum; // Checksum for data integrity. Sender calculate checksum with data, Server receievs it and checks for data integrity
    unsigned short length; // Length of the data without the header length
    unsigned char flags;
};

typedef struct _rudp_socket RUDP_Socket;
typedef struct _rudp_header RUDP_Header;

/* Allocates a new structure for the RUDP socket (contains basic information about the socket itself). Also creates a UDP socket as a baseline for the RUDP.
 isServer means that this socket acts like a server. If set to server socket, it also binds the socket to a specific port. */
RUDP_Socket* RUDP_Socket_alloc(int isServer, unsigned short listen_port, int sec_tout);

/* Tries to connect to the other side via RUDP to given IP and port. Returns 0 on failure and 1 on success. Fails if called when the socket is connected/set to server. */
int rudp_connect(RUDP_Socket* sockfd, const char * dest_ip, unsigned short dest_port, char* buff, int buff_size);

/* Accepts incoming connection request and completes the handshake, returns 0 on failure and 1 on success. Fails if called when the socket is connected/set to client. */
int rudp_accept(RUDP_Socket* sockfd, char* buff, int buff_size);

// Receives data from the other side and put it into the buffer. Returns the number of received bytes on success, 0 if got FIN packet (disconnect), and -1 on error. Fails if called when the socket is disconnected.
int rudp_recv(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size);

// Sends data stores in buffer to the other side. Returns the number of sent bytes on success, 0 if got FIN packet (disconnect), and -1 on error. Fails if called when the socket is disconnected.
int rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size);

// Disconnects from an actively connected socket. Returns 1 on success, 0 when the socket is already disconnected (failure).
int rudp_disconnect(RUDP_Socket *sockfd, char* buff, unsigned int buff_size);

/*  Pack a header and a time to one byte-sequence starting in address dest. 
    If data is NULL there's only header to send so return header.*/
const char* packet_alloc(RUDP_Header* header, char* data, size_t* pack_size);

// Send a simple ACK packet. fails (return 0) if socket disconnected
int RUDP_sendACK(RUDP_Socket* sockfd);

// This function releases all the memory allocation and resources of the socket.
int rudp_close(RUDP_Socket *sockfd);

/**
* @brief A checksum function that returns 16 bit checksum for data.
* @param data The data to do the checksum for.
* @param bytes The length of the data in bytes.
* @return The checksum itself as 16 bit unsigned number.
*/
unsigned short int calculate_checksum(void *data, unsigned int bytes);

/**
* @brief A random data generator function based on srand() and rand().
* @param size The size of the data to generate (up to 2^32 bytes).
* @return A pointer to the buffer.
*/
char *util_generate_random_data(unsigned int size);

/**
 * @brief A helper function for measuring time in milliseconds
 * @param result The address to save the calculation
 * @param earlier The earlier time
 * @param later The later time
*/
void timespec_substract(struct timespec *result, struct timespec *earlier, struct timespec *later);

/**
 * A simple function to convert seconds and nano-seconds to milli-seconds
 * @result A double representation of milliseconds
*/
double timespec_to_ms(struct timespec* target);
#endif