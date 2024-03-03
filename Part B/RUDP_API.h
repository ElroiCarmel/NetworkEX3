#ifndef RUDP_API
#define RUDP_API

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

#define SERVER_PORT 5060
#define SERVER_IP "127.0.0.1"

#define ACK 1
#define SIN 2
#define FIN 4
#define STOF 8
#define ENOF 16

typedef struct _rudp_socket RUDP_Socket;
typedef struct _rudp_header RUDP_Header;

/* Allocates a new structure for the RUDP socket (contains basic information about the socket itself). Also creates a UDP socket as a baseline for the RUDP.
 isServer means that this socket acts like a server. If set to server socket, it also binds the socket to a specific port. */
RUDP_Socket* RUDP_Socket_alloc(int isServer, unsigned short listen_port, int sec_tout);

/* Tries to connect to the other side via RUDP to given IP and port. Returns 0 on failure and 1 on success. Fails if called when the socket is connected/set to server. */
int rudp_connect(RUDP_Socket* sockfd, const char * dest_ip, unsigned short dest_port, char* buff, int buff_size);

/* Accepts incoming connection request and completes the handshake, returns 0 on failure and 1 on success. Fails if called when the socket is connected/set to client. */
int rudp_accept(RUDP_Socket* sockfd, char* buff, int buff_size);


/*  Pack a header and a time to one byte-sequence starting in address dest. 
    If data is NULL there's only header to send so return header.*/
const char* packet_alloc(RUDP_Header* header, char* data, size_t* pack_size);


#endif