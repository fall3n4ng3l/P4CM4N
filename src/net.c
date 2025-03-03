#include <ctype.h>
#include <stdbool.h>
#include <ncurses.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include "pacman.h"
#include "net.h"

void broadcast_dir_pkt(int sd, uint8_t direction, char *name){
    uint32_t namelength = strlen(name);
    pkt_header pkt = {htonl(MAGIC), htonl(0xffffffff), htonl(namelength + sizeof(uint8_t))};
    send(sd, &pkt, sizeof(pkt), 0);
    send(sd, &direction, sizeof(uint8_t), 0);
    send(sd, name, namelength, 0);
}

void send_to_server_dir_pkt(int sd, uint8_t direction){
    pkt_header pkt = {htonl(MAGIC), htonl(0x00000000), htonl(0x00000001)};
    send(sd, &pkt, sizeof(pkt), 0);
    send(sd, &direction, sizeof(uint8_t), 0);
}

uint8_t recv_dir_pkt_on_server(int sd){
    pkt_header pkt;
    if (!recv(sd, (void*)&pkt.magic, 4, 0) || ntohl(pkt.magic)!=MAGIC) print_error("unknown magic in recv_dir_pkt_on_server");
    if (!recv(sd, &pkt.ptype, 4, 0) || ntohl(pkt.ptype)!=0x00)  print_error("recv_dir_pkt_on_server packet structure error");
    if (!recv(sd, &pkt.datasize, 4, 0)) print_error(" recv_dir_pkt_on_server packet structure error");
    uint8_t direction;
    if (!recv(sd, &direction, sizeof(uint8_t), 0)) print_error(" recv_dir_pkt_on_server packet structure error");
    return direction;
}

struct player_dir_pkt recv_from_server_broadcast_pkt(int sd){
    pkt_header pkt;
    struct player_dir_pkt value;
    if (!recv(sd, (void*)&pkt.magic, 4, 0) || ntohl(pkt.magic)!=MAGIC) print_error("unknown magic in recv_from_server_broadcast_pkt");
    if (!recv(sd, &pkt.ptype, 4, 0) || ntohl(pkt.ptype)!=0xffffffff) print_error("brd packet structure error");
    if (!recv(sd, &pkt.datasize, 4, 0)) print_error("brd packet structure error");

    int namelength = ntohl(pkt.datasize)-sizeof(uint8_t);
    uint8_t direction;
    if (!recv(sd, &direction, sizeof(uint8_t), 0)) print_error("brd recv dir error");
    char* playername = (char*)malloc(namelength);
    if (!recv(sd, playername, namelength, 0)) print_error("brd recv name error");

    value.direction = direction;
    value.playername = playername;
    return value;
}

void send_map_pkt(uint8_t* map, int sd){
    pkt_header pkt = {htonl(MAGIC), htonl(0x00000010), htonl(QUARTER_HEIGHT * QUARTER_WIDTH)};
    ssize_t sent_bytes = send(sd, &pkt, sizeof(pkt), 0);
    if (sent_bytes != sizeof(pkt)) {
        print_error("Failed to send packet header");
    }
    sent_bytes = send(sd, map, ntohl(pkt.datasize), 0);
    if (sent_bytes != ntohl(pkt.datasize)) {
        print_error("Failed to send map");
    }
}

uint8_t* recv_map_pkt(int sd){
    pkt_header pkt;
    if (!recv(sd, (void*)&pkt.magic, 4, 0) || ntohl(pkt.magic)!=MAGIC) print_error("unknown magic");
    if (!recv(sd, &pkt.ptype, 4, 0) || ntohl(pkt.ptype)!=0x10) print_error("map packet structure error");
    if (!recv(sd, &pkt.datasize, 4, 0)) print_error("map packet structure error");
    pkt.datasize = ntohl(pkt.datasize);
    uint8_t* map = (uint8_t*)malloc(pkt.datasize);
    if (!recv(sd, map, pkt.datasize, 0)) print_error("map recv packet error");
    return map;
}  

void send_init_pkt(char* name, int sd){
    uint32_t namelength = strlen(name);
    pkt_header pkt = {htonl(MAGIC), htonl(0x00000001), htonl(namelength)};
    send(sd, &pkt, sizeof(pkt), 0);
    send(sd, name, namelength, 0);
}

void send_rdy_pkt(int sd){
    pkt_header pkt = {htonl(MAGIC), htonl(0x00000002), 0x00000000};
    send(sd, &pkt, sizeof(pkt), 0);
}

int recv_rdy_pkt(int sd){
    pkt_header pkt;
    if (!recv(sd, (void*)&pkt.magic, 4, 0) || ntohl(pkt.magic)!=MAGIC) return 0;
    if (!recv(sd, &pkt.ptype, 4, 0) || ntohl(pkt.ptype)!=0x02) return 0;
    if (!recv(sd, &pkt.datasize, 4, 0)) return 0;
    return 1;
}  

char* recv_init_pkt(int sd){
    pkt_header pkt;
    if (!recv(sd, (void*)&pkt.magic, 4, 0) || ntohl(pkt.magic)!=MAGIC) print_error("unknown magic");
    if (!recv(sd, &pkt.ptype, 4, 0) || ntohl(pkt.ptype)!=0x01) print_error("rdy packet structure error");
    if (!recv(sd, &pkt.datasize, 4, 0)) print_error("rdy packet structure error");
    pkt.datasize = ntohl(pkt.datasize);
    char* playername = (char*)malloc(pkt.datasize);
    if (!recv(sd, playername, pkt.datasize, 0)) print_error("rdy recv packet error");
    return playername;
}

uint8_t default_map[QUARTER_HEIGHT * QUARTER_WIDTH] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xff,
    0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff,
    0xff, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xff,
    0xff, 0xaa, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xaa, 0xaa, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xaa, 0xff,
    0xff, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xff,
    0xaa, 0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xaa,
    0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xff, 0x22, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xaa,
    0xaa, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xaa,
    0xff, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xff,
    0xff, 0xaa, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xaa, 0xaa, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xaa, 0xff,
    0xff, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xff, 0xaa, 0xff,
    0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff, 0xff, 0xff, 0xaa, 0xff,
    0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};