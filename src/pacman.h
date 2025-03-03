#ifndef PACMAN_H
#define PACMAN_H
#define QUARTER_WIDTH 20
#define QUARTER_HEIGHT 15
#define FULL_WIDTH 40
#define FULL_HEIGHT 30
#define RATIO 1000
#define MAGIC 0xabcdfe01
#define SERVER_NAME "Server"
#define DEFAULT_FRAME_RATE 500

#define msleep(usec) usleep(usec*1000)

enum DIRECTION {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

typedef struct {
    int x;
    int y;
} point;

struct player_dir_pkt{
    uint8_t direction;
    char* playername;
};

typedef struct {
    const uint32_t magic;
    uint32_t ptype;
    uint32_t datasize;
} pkt_header;

struct player {
    uint32_t start_x;
    uint32_t start_y;
    uint32_t start_direction;
    uint32_t player_name_len;
    uint32_t score;
    uint32_t sd;
    uint8_t player_name[256];
};

typedef struct {
    struct player* players;
    uint32_t frame_timeout;
    uint32_t p_count;
} start_game_pkt;
#endif


