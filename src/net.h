#ifndef FUNCTIONS_H
#define FUNCTIONS_H
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
#include <locale.h>

void print_error(const char *errorMessage);
void broadcast_dir_pkt(int sd, uint8_t direction, char *name);
void send_to_server_dir_pkt(int sd, uint8_t direction);
uint8_t recv_dir_pkt_on_server(int sd);
struct player_dir_pkt recv_from_server_broadcast_pkt(int sd);
void send_map_pkt(uint8_t* map, int sd);
uint8_t* recv_map_pkt(int sd); 
void send_init_pkt(char* name, int sd);
void send_rdy_pkt(int sd);
int recv_rdy_pkt(int sd);
char* recv_init_pkt(int sd);
#endif