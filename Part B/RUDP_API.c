#include "RUDP_API.h"

struct _rudp_socket {
    int socket_fd; // UDP socket file descrriptor
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

RUDP_Socket* RUDP_Socket_alloc(int isServer, unsigned short listen_port, int sec_tout) {
    RUDP_Socket* sock = (RUDP_Socket*) calloc(1, sizeof(RUDP_Socket));
    if (sock == NULL) {
        fprintf(stdout, "Out of memory!\n");
        exit(1);
    }
    
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("socket(2)");
        exit(1);
    }
    sock->socket_fd = udp_sock;
    if (isServer) {
        // Create a socket address and bind the udp_sock  to the listen_port
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(listen_port);
        if (bind(udp_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
            perror("bind(2)");
            close(udp_sock);
            exit(1);
        }
        sock->isServer = 1;
    }
    else {
         struct timeval tv = {sec_tout, 0};
         if (setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(tv)) == -1)
            {
                perror("setsockopt(2)");
                close(udp_sock);
                exit(1);      
            }
        sock->isServer = 0;
    }
    return sock;
}


int rudp_connect(RUDP_Socket* sockfd, const char * dest_ip, unsigned short dest_port, char* buff, int buff_size) {
    if (sockfd->isServer || sockfd->isConnected) return 1;
    
    // create socket address

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    if (inet_pton(AF_INET, dest_ip, &serv_addr.sin_addr) < 0) {
        perror("inet_pton(2)");
        close(sockfd->socket_fd);
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(dest_port);

    // create SIN packet
    RUDP_Header sin_h;
    memset(&sin_h, 0, sizeof(sin_h));
    sin_h.flags |= SIN;
    size_t sin_pack_s;
    const char * sin_packet = packet_alloc(&sin_h, NULL, &sin_pack_s);

    // Send it
    long byte_sent = sendto(sockfd->socket_fd, sin_packet, sin_pack_s, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (byte_sent < 0) {
        perror("sendto(2)");
        close(sockfd->socket_fd);
        exit(1);
    }

    // Wait for SIN+ACK packet
    struct sockaddr_in recv_server;
    socklen_t rev_server_len = sizeof(recv_server);
    int bytes_recv = recvfrom(sockfd->socket_fd, buff, buff_size, 0, (struct sockaddr *) &recv_server, &rev_server_len);
    
    if (bytes_recv < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            fprintf(stdout, "Time-Out occured. Server didn't respond\n");
            return 0;
        }
    }
    
    RUDP_Header* recv_header = (RUDP_Header*) buff;
    int is_sin_ack = (recv_header->flags & SIN) && (recv_header->flags & ACK);
    if (is_sin_ack == 0) {
        fprintf(stdout, "Server didn't respond with SYN+ACK");
        return 0;
    }

    fprintf(stdout, "Server replied with SYN+ACK!\n");

    // Create ACK packet
    RUDP_Header ack_h;
    memset(&ack_h, 0, sizeof(ack_h));
    ack_h.flags |= SIN;
    size_t ack_pack_s;
    const char * ack_packet = packet_alloc(&sin_h, NULL, &ack_pack_s);
    // Send ACK
    byte_sent = sendto(sockfd->socket_fd, ack_packet, ack_pack_s, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (byte_sent < 0) {
        perror("sendto(2)");
        close(sockfd->socket_fd);
        exit(1);
    }
    // connect the udp and set isConnected to 1
    memcpy(&(sockfd->dest_adrr), &serv_addr, sizeof(serv_addr));
    sockfd->isConnected = 1;
    free((char*)sin_packet); free((char*)ack_packet);
    return 1;
}

int rudp_accept(RUDP_Socket* sockfd, char* buff, int buff_size) {
    if (sockfd->isConnected || !sockfd->isServer) return 0;
    

    fprintf(stdout, "Server waiting for connection request..\n");
    struct sockaddr_in client_addr;
    socklen_t client_addr_s = sizeof(client_addr);

    int bytes_recv  = recvfrom(sockfd->socket_fd, buff, buff_size, 0, (struct sockaddr*) &client_addr, &client_addr_s);
    
    if (bytes_recv < 0) {
        perror("recvfrom(2)");
        close(sockfd->socket_fd);
        exit(1);
    }

    // check if SIN packet arrived
    RUDP_Header* recv_header = (RUDP_Header*) buff;
    if (recv_header->flags & SIN) {
        fprintf(stdout, "Got SYN packet!\n");
        // Send SIN+ACK
        RUDP_Header sin_ack_h;
        memset(&sin_ack_h, 0 ,sizeof(sin_ack_h));
        sin_ack_h.flags |= (SIN | ACK);

        size_t sin_ack_pack_s;
        const char* sin_ack_packet = packet_alloc(&sin_ack_h, NULL, &sin_ack_pack_s);

        int bytes_sent = sendto(sockfd->socket_fd, sin_ack_packet, sin_ack_pack_s, 0, (struct sockaddr*) &client_addr, client_addr_s);
        if (bytes_sent < 0) {
            perror("sendto(2)");
            close(sockfd->socket_fd);
            exit(1); }

        // Wait for ACK from client
        bytes_recv = recvfrom(sockfd->socket_fd, buff, buff_size, 0, (struct sockaddr*) &client_addr, &client_addr_s);
        if (bytes_recv < 0) {
            perror("recvfrom(2)");
            close(sockfd->socket_fd);
            exit(1);     
        }
        if (recv_header->flags & ACK) {
            sockfd->isConnected = 1;
            memcpy(&(sockfd->dest_adrr), &client_addr, client_addr_s);
            free((char*)sin_ack_packet);
            return 1;
        } 
    }
    return 0;

}

const char* packet_alloc(RUDP_Header* header, char* data, size_t* pack_size) {
    const char * ans = (const char *) calloc(1, sizeof(RUDP_Header) + header->length);
    memcpy((char*)ans, header, sizeof(RUDP_Header));
    if (data == NULL) {
        *pack_size = sizeof(RUDP_Header);
        return ans; }
    memcpy((char*) ans + sizeof(RUDP_Header), data, header->length);
    *pack_size = sizeof(RUDP_Header) + header->length;
    return ans;
}


