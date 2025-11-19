// server.c - UDP posrednik (relay)
// Kompajliranje: gcc -Wall -o server server.c
// Pokretanje:    ./server <data_port> <control_port>
// Primjer:       ./server 5000 5001

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 1500

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uporaba: %s <data_port> <control_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int data_port = atoi(argv[1]);
    int ctrl_port = atoi(argv[2]);

    int sock_data, sock_ctrl;
    struct sockaddr_in addr_data, addr_ctrl;

    // 1) Kreiranje data socketa
    if ((sock_data = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket data");
        exit(EXIT_FAILURE);
    }

    memset(&addr_data, 0, sizeof(addr_data));
    addr_data.sin_family = AF_INET;
    addr_data.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_data.sin_port = htons(data_port);

    if (bind(sock_data, (struct sockaddr *)&addr_data, sizeof(addr_data)) < 0) {
        perror("bind data");
        close(sock_data);
        exit(EXIT_FAILURE);
    }

    // 2) Kreiranje control socketa
    if ((sock_ctrl = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket ctrl");
        close(sock_data);
        exit(EXIT_FAILURE);
    }

    memset(&addr_ctrl, 0, sizeof(addr_ctrl));
    addr_ctrl.sin_family = AF_INET;
    addr_ctrl.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_ctrl.sin_port = htons(ctrl_port);

    if (bind(sock_ctrl, (struct sockaddr *)&addr_ctrl, sizeof(addr_ctrl)) < 0) {
        perror("bind ctrl");
        close(sock_data);
        close(sock_ctrl);
        exit(EXIT_FAILURE);
    }

    printf("UDP posrednik pokrenut.\n");
    printf("  Data port   : %d\n", data_port);
    printf("  Control port: %d\n\n", ctrl_port);

    // 3) Držimo podatke o dvije strane
    struct sockaddr_in peer1_addr, peer2_addr;
    socklen_t peer1_len = 0, peer2_len = 0;
    int have_peer1 = 0, have_peer2 = 0;
    int forwarding_enabled = 0;

    char buf[BUF_SIZE];

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock_data, &readfds);
        FD_SET(sock_ctrl, &readfds);
        int maxfd = (sock_data > sock_ctrl ? sock_data : sock_ctrl) + 1;

        int rv = select(maxfd, &readfds, NULL, NULL, NULL);
        if (rv < 0) {
            perror("select");
            break;
        }

        // 4) Kontrolni socket - ON/OFF/RESET
        if (FD_ISSET(sock_ctrl, &readfds)) {
            struct sockaddr_in caddr;
            socklen_t clen = sizeof(caddr);
            ssize_t n = recvfrom(sock_ctrl, buf, sizeof(buf) - 1, 0,
                                 (struct sockaddr *)&caddr, &clen);
            if (n > 0) {
                buf[n] = '\0';
                if (strncmp(buf, "ON", 2) == 0) {
                    forwarding_enabled = 1;
                    printf("[CTRL] Forwarding: ON\n");
                } else if (strncmp(buf, "OFF", 3) == 0) {
                    forwarding_enabled = 0;
                    printf("[CTRL] Forwarding: OFF\n");
                } else if (strncmp(buf, "RESET", 5) == 0) {
                    have_peer1 = have_peer2 = 0;
                    peer1_len = peer2_len = 0;
                    printf("[CTRL] Resetirani peerovi.\n");
                } else {
                    printf("[CTRL] Nepoznata naredba: '%s'\n", buf);
                }
            }
        }

        // 5) Data socket - promet između dva klijenta
        if (FD_ISSET(sock_data, &readfds)) {
            struct sockaddr_in src_addr;
            socklen_t src_len = sizeof(src_addr);
            ssize_t n = recvfrom(sock_data, buf, sizeof(buf), 0,
                                 (struct sockaddr *)&src_addr, &src_len);
            if (n < 0) {
                perror("recvfrom data");
                continue;
            }

            // Pamćenje peerova
            if (!have_peer1) {
                peer1_addr = src_addr;
                peer1_len = src_len;
                have_peer1 = 1;
                printf("[DATA] Registriran peer1: %s:%d\n",
                       inet_ntoa(peer1_addr.sin_addr),
                       ntohs(peer1_addr.sin_port));
            } else if (!have_peer2 &&
                       (src_addr.sin_addr.s_addr != peer1_addr.sin_addr.s_addr ||
                        src_addr.sin_port != peer1_addr.sin_port)) {
                peer2_addr = src_addr;
                peer2_len = src_len;
                have_peer2 = 1;
                printf("[DATA] Registriran peer2: %s:%d\n",
                       inet_ntoa(peer2_addr.sin_addr),
                       ntohs(peer2_addr.sin_port));
            }

            // Ako forwarding nije uključen, ignoriraj
            if (!forwarding_enabled) {
                continue;
            }

            // Ako imamo oba peer-a, proslijedi
            if (have_peer1 && have_peer2) {
                struct sockaddr_in *dst_addr = NULL;
                socklen_t dst_len = 0;

                if (src_addr.sin_addr.s_addr == peer1_addr.sin_addr.s_addr &&
                    src_addr.sin_port == peer1_addr.sin_port) {
                    // iz peer1 → peer2
                    dst_addr = &peer2_addr;
                    dst_len = peer2_len;
                } else if (src_addr.sin_addr.s_addr == peer2_addr.sin_addr.s_addr &&
                           src_addr.sin_port == peer2_addr.sin_port) {
                    // iz peer2 → peer1
                    dst_addr = &peer1_addr;
                    dst_len = peer1_len;
                } else {
                    // Nepoznat izvor (treća strana) - ignoriraj
                    continue;
                }

                if (sendto(sock_data, buf, n, 0,
                           (struct sockaddr *)dst_addr, dst_len) < 0) {
                    perror("sendto data");
                }
            }
        }
    }

    close(sock_data);
    close(sock_ctrl);
    return 0;
}
