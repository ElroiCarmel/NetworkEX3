#include "RUDP_API.h"



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

        int opt = 1;
        if (setsockopt(udp_sock, SOL_SOCKET,SO_REUSEADDR, &opt, sizeof(opt))< 0)
            {
                perror("setsockopt(2)");
                close(udp_sock);
                exit(1);      
            }

        if (bind(udp_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
            perror("bind(2)");
            close(udp_sock);
            exit(1);
        }
        sock->isServer = 1;
    }
    else {
        struct timeval sec = {sec_tout, 0};
         if (setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (char*) &sec, sizeof(sec)) < 0)
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
    RUDP_Header sin;
    memset(&sin, 0, sizeof(sin));
    sin.flags |= SIN;
    

    // Send it
    long byte_sent = sendto(sockfd->socket_fd, &sin, sizeof(sin), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
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
        } else {
            perror("recvfrom(2)");
            close(sockfd->socket_fd);
        }
    }
    
    // Recieved answer
    RUDP_Header* recv_header = (RUDP_Header*) buff;
    int is_sin_ack = (recv_header->flags & SIN) && (recv_header->flags & ACK);
    if (is_sin_ack == 0) {
        fprintf(stdout, "Server didn't respond with SYN+ACK\n");
        return 0;
    }

    fprintf(stdout, "Server replied with SYN+ACK!\n");

    
    

    // connect the udp and set isConnected to 1
    memcpy(&(sockfd->dest_adrr), &serv_addr, sizeof(serv_addr));
    sockfd->isConnected = 1;

    // Send ACK
    RUDP_sendACK(sockfd);
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
        RUDP_Header sin_ack;
        memset(&sin_ack, 0 ,sizeof(sin_ack));
        sin_ack.flags |= (SIN | ACK);

       
        int bytes_sent = sendto(sockfd->socket_fd, &sin_ack, sizeof(sin_ack), 0, (struct sockaddr*) &client_addr, client_addr_s);
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
            return 1;
        } 
    }
    return 0;

}

int rudp_recv(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size) {
    if (sockfd->isConnected == 0) return 0;
    // Deal with FIN packet
    struct sockaddr_in recv_addr;
    socklen_t recv_addr_s = sizeof(recv_addr);
    int bytes_recv = recvfrom(sockfd->socket_fd, buffer, buffer_size, 0, (struct sockaddr*)&recv_addr, &recv_addr_s);
    if (bytes_recv < 0) {
        perror("recvfrom(2)");
        close(sockfd->socket_fd);
        return -1;
    }

    RUDP_Header* recv_header = (RUDP_Header*) buffer;
    if (recv_header->flags & FIN) {
        rudp_disconnect(sockfd, (char*)buffer, buffer_size); // Continue the connection closing scheme
        return 0;
    }

    unsigned short mess_len = recv_header->length;
    if (mess_len) {
        char* data = (char*) (recv_header+1);
        // calculate checksum to check for data-integrity
        unsigned short checksum = calculate_checksum(data, mess_len);
        if (checksum == recv_header->checksum) {
            // send ACK
            RUDP_sendACK(sockfd);
        }
    }
    return bytes_recv;
}


int rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size) {

    if (sockfd->isConnected == 0) return -1;
    const size_t MAX_LEN = MAX_DGRAM_SIZE - sizeof(RUDP_Header);
    // If data too large divide it to smaller chunks
    int total_sent = 0;
    for (char* data_start = (char*)buffer; data_start < (char*)buffer + buffer_size; data_start+=MAX_LEN) {
        // Create header
        RUDP_Header mess_header;
        memset(&mess_header, 0, sizeof(mess_header));
        if (data_start == buffer) {
            mess_header.flags |= STOF; // Turn on start of file
        }
        if (data_start + MAX_LEN >= (char*)buffer+buffer_size) {
            mess_header.flags |= ENOF; // Turn on end of file
        }
        
        int end_to_current_start = (char*)buffer+buffer_size - data_start;
        mess_header.length = (end_to_current_start < MAX_LEN) ? end_to_current_start : MAX_LEN;
        
        mess_header.checksum = calculate_checksum(data_start, mess_header.length);

        size_t pack_size;
        const char* to_send = packet_alloc(&mess_header, data_start, &pack_size);
        // remember to free after ack received!
        int is_acked = 0;
        while (is_acked == 0) {
            int bytes_sent = sendto(sockfd->socket_fd, to_send, pack_size, 0, (struct sockaddr*) &(sockfd->dest_adrr), sizeof(sockfd->dest_adrr));
            if (bytes_sent < 0) {
                perror("sendto(2)");
                close(sockfd->socket_fd);
                free((char*)to_send);
                exit(1);
            }
        
            // Wait for ACK
            struct sockaddr_in recv_addr;
            socklen_t recv_addr_s = sizeof(recv_addr);
            int bytes_recv = recvfrom(sockfd->socket_fd, buffer, buffer_size, 0, (struct sockaddr*)&recv_addr, &recv_addr_s);
            if (bytes_recv < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    fprintf(stdout, "Time out ocurred!\n");
                } else {
                perror("recvfrom(2)");
                close(sockfd->socket_fd);
                free((void*)to_send);
                return -1;
                }
            }
            else if (bytes_recv > 0) {
                RUDP_Header* recv_header = (RUDP_Header*) buffer;
                if (recv_header->flags & ACK) {
                    is_acked = 1;
                    total_sent += mess_header.length;
                    free((void*)to_send);
                }
            }
        }
    }
    return total_sent;
}


int rudp_disconnect(RUDP_Socket *sockfd, char* buff, unsigned int buff_size) {
    if (sockfd->isConnected == 0) return 0;
    if (sockfd->isServer) {
        // Send FIN+ACK
        RUDP_Header fin_ack;
        memset(&fin_ack, 0, sizeof(fin_ack));
        fin_ack.flags = FIN | ACK;

        int bytes_sent = sendto(sockfd->socket_fd, &fin_ack, sizeof(fin_ack), 0, (struct sockaddr*) &(sockfd->dest_adrr), sizeof(sockfd->dest_adrr));
        if (bytes_sent < 0) {
            perror("sendto(2)");
            close(sockfd->socket_fd);
            exit(1);
        }
   
        // Wait for ACK from client
        struct sockaddr_in recv_addr;
        socklen_t addr_s = sizeof(recv_addr);

        int bytes_recv = recvfrom(sockfd->socket_fd, buff, buff_size, 0, (struct sockaddr*)&recv_addr, &addr_s);
        if (bytes_recv < 0) {
            perror("recvfrom(2)");
            close(sockfd->socket_fd);
            exit(1);
        }

        RUDP_Header * recv_h = (RUDP_Header*) buff;
        if (recv_h->flags & ACK) {
            sockfd->isConnected = 0;
            return 1;} else return 0;
    } else { // If it's client
        int flag = 0;
        while (flag != 1) {
        // Send FIN
        RUDP_Header fin;
        memset(&fin, 0, sizeof(fin));
        fin.flags |= FIN;

        int bytes_sent = sendto(sockfd->socket_fd, &fin, sizeof(fin), 0,(struct sockaddr*) &(sockfd->dest_adrr), sizeof(sockfd->dest_adrr));
        if (bytes_sent < 0) {
            perror("sendto(2)");
            close(sockfd->socket_fd);
            exit(1);
        }
        // Wait for FIN+ACK from server
        
        
        struct sockaddr_in recv_addr;
        socklen_t recv_addr_s = sizeof(recv_addr);
        int bytes_recv = recvfrom(sockfd->socket_fd, buff, buff_size, 0, (struct sockaddr*)&recv_addr, &recv_addr_s);
        if (bytes_recv < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {                    
                fprintf(stdout, "Timeout occured!!, Server didn't respond..\n");
                } else {
                perror("recvfrom(2)");
                close(sockfd->socket_fd);
                exit(1);
            }
        } else if (bytes_recv > 0) {
            RUDP_Header* recv_h = (RUDP_Header*) buff;
            if (recv_h->flags & (FIN|ACK)) {
                flag = 1;
            }
        }
        }
        // Send ACK
        RUDP_sendACK(sockfd);
        sockfd->isConnected = 0;
        return 1;
    }
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

int RUDP_sendACK(RUDP_Socket* sockfd) {
    if (!sockfd->isConnected) return 0;
    RUDP_Header ack;
    memset(&ack, 0, sizeof(ack));
    ack.flags |= ACK;

    int bytes_sent = sendto(sockfd->socket_fd, &ack, sizeof(ack), 0, (struct sockaddr*)&(sockfd->dest_adrr), sizeof(sockfd->dest_adrr));
    if (bytes_sent <= 0) {
        perror("sendto(2)");
        close(sockfd->socket_fd);
        exit(1);
    }
    return 1;
}

int rudp_close(RUDP_Socket *sockfd) {
    close(sockfd->socket_fd);
    free(sockfd);
    return 1;
}

unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;
    // Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
        }
    // Add left-over byte, if any
    if (bytes > 0)
    total_sum += *((unsigned char *)data_pointer);
    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16)
    total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    return (~((unsigned short int)total_sum));
}

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

void timespec_substract(struct timespec *result, struct timespec *earlier, struct timespec *later) {
    result->tv_sec = later->tv_sec - earlier->tv_sec;
    if (later->tv_nsec >= earlier->tv_nsec) result->tv_nsec = later->tv_nsec - earlier->tv_nsec;
    else result->tv_nsec = later->tv_nsec - earlier->tv_nsec + ((long)1e9);
}

double timespec_to_ms(struct timespec* target){
    double ans = target->tv_sec*1e3 + target->tv_nsec*1e-6;
    return ans;
}