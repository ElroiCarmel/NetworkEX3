#include "RUDP_API.h"

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Error! Usage of program is '-p' <PORT>\n");
        return 1;
    }

    int server_port = atoi(argv[2]);
    if (server_port == 0) {
        fprintf(stdout, "Error! port sytnax failure...\n");
        return 1;
    }

    char buffer[BUFF_SIZE] = {0};
    
    RUDP_Socket* sock = RUDP_Socket_alloc(1, server_port, 0);

    fprintf(stdout, "Starting Receiver...\nWaiting for RUDP Connection..\n");

    rudp_accept(sock, buffer, BUFF_SIZE);

    struct timespec start_time = {0}, end_time = {0}, interval_time = {0};
    size_t total_fs = 0;
    FILE* stats = fopen("stats.txt", "w");
    if (stats == NULL) {
        perror("Error opening file to save statistics\n");
        rudp_close(sock);
        return 1;
    }
    fclose(stats);

    stats = fopen("stats.txt", "a");
    if (stats == NULL) {
        perror("Error opening file to save statistics\n");
        rudp_close(sock);
        return 1;
    }

    
    while(1) {

        int bytes_recv = rudp_recv(sock, buffer, BUFF_SIZE);
        if (bytes_recv == -1) {
            fprintf(stdout, "Error receiving... exiting..\n");
            rudp_close(sock);
            return 1;
        } else if (bytes_recv == 0) {
            break;
        } else {
            RUDP_Header* header = (RUDP_Header*) buffer;
            total_fs += header->length;
            if (header->flags & STOF) {
                fprintf(stdout, "Beginning file receiveing...");
                timespec_get(&start_time, TIME_UTC);
            } 
            if (header->flags & ENOF) {

                size_t temp_s = total_fs / (1024.0*1024.0); // In MB
                total_fs = 0;
                timespec_get(&end_time, TIME_UTC);
                timespec_substract(&interval_time, &start_time, &end_time);
                double tms = timespec_to_ms(&interval_time);
                fprintf(stats, "File received! File size: %ldMB. Time for receiveing: %fms. Average speed: %fMB/s\n",temp_s, tms, temp_s/(tms/1e3));
        }
    }
    }
    fprintf(stdout, "Sender disconnected. Closing Receiver..\n");
    rudp_close(sock);
    fclose(stats);

    fprintf(stdout, "Printing statistics:\n");
    stats = fopen("stats.txt", "r");
    if (stats == NULL) {
        perror("Error opening file to read statistics\n");
        rudp_close(sock);
        return 1;
    }

    while (!feof(stats)){
        fprintf(stdout, "%c", fgetc(stats));
    }
    
    
    fclose(stats);
    return 0;
}