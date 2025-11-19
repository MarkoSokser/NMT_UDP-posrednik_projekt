// control_client.c - pomoćni klijent za ON/OFF
// Kompajliranje: gcc -Wall -o ctrl control_client.c
// Pokretanje:    ./ctrl <server_ip> <control_port> <ON|OFF|RESET>
// Primjer:       ./ctrl 127.0.0.1 5001 ON

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uporaba: %s <server_ip> <control_port> <ON|OFF|RESET>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int ctrl_port = atoi(argv[2]);
    const char *cmd = argv[3];

    int sock;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ctrl_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (sendto(sock, cmd, strlen(cmd), 0,
               (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("sendto");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Poslana naredba '%s' na %s:%d\n", cmd, server_ip, ctrl_port);

    close(sock);
    return 0;
}
