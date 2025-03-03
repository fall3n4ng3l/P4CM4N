#include "pacman.h"
#include "net.h"

uint8_t CURRENT_DIRECTION = 0;
int GAME_RUNNING = 0;
int start_x;
int start_y;
int food_eaten = 0;
int count_of_food;
extern uint8_t default_map[QUARTER_HEIGHT * QUARTER_WIDTH];

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -s <exp_players> -p <port> [-r <frame_rate>] [-m] (Server mode) \r\n %s -c -h <hostname> -p <port> (Client mode)\n", prog_name,prog_name);
    exit(EXIT_FAILURE);
}

void print_error(const char *errorMessage) {
    fprintf(stderr, "Error: %s\n", errorMessage);
    exit(EXIT_FAILURE);
}

void* keyboard_handler(void* arg) {
    while (GAME_RUNNING) {
        char ch = getchar();
        switch (ch) {
            case 'w':
            case 'W':
                CURRENT_DIRECTION = UP; 
                break;
            case 's':
            case 'S':
                CURRENT_DIRECTION = DOWN;  
                break;
            case 'a':
            case 'A':
                CURRENT_DIRECTION = LEFT;  
                break;
            case 'd':
            case 'D':
                CURRENT_DIRECTION = RIGHT;  
                break;
            case '\033': 
                getchar(); 
                switch(getchar()) { 
                    case 'A':
                        CURRENT_DIRECTION = UP;
                        break;
                    case 'B':
                        CURRENT_DIRECTION = DOWN; 
                        break;
                    case 'C':
                        CURRENT_DIRECTION = RIGHT; 
                        break;
                    case 'D':
                        CURRENT_DIRECTION = LEFT; 
                        break;
                }
                break;
        }
    }
    return NULL;
}

point generatePoint(int w, int h) {
    point p;
    p.x = rand() % h; 
    p.y = rand() % w;
    return p;
}

int isBuilderSafe(point p, uint8_t* array, int w, int h){
    int is_safe = 1;
    for(int i = -1; i < 2; i++) {
        for(int j = -1; j < 2; j++) {
            if ((p.x + j <= h-1) && (p.x + j >= 0) && (p.y + i <= w-1) && (p.y + i >= 0) && ((i!=0) || (j!=0))){
                if (array[p.y + i + (p.x + j)*w] == 0xff){
                    is_safe = 0;
                }
            } 
        }
    }
    return is_safe;
}

uint8_t* Generate_map(int w, int h, int n) {
    srand(time(NULL));
    point* builders = (point*)malloc(n * sizeof(point));
    uint8_t*array = (uint8_t*)malloc(h * w * sizeof(uint8_t));
    for(int i = 0; i < h; i++) {
        for(int j = 0; j < w; j++) {
            array[j+i*w] = 0xaa;
        }
    }

    int i=0; 
    while (i!=n){ // Placing the "builders"
        point p = generatePoint(QUARTER_WIDTH,QUARTER_HEIGHT);
        if (isBuilderSafe(p, array, QUARTER_WIDTH, QUARTER_HEIGHT)){
            array[p.y + p.x * w] = 0xff;
            builders[i]=p;
            i++;
        } 
    }
    // "builders" logic can be added next...

    point player = generatePoint(QUARTER_WIDTH,QUARTER_HEIGHT);
    while (array[player.y + player.x * w] != 0xaa){
        point player = generatePoint(QUARTER_WIDTH,QUARTER_HEIGHT);
    }
    array[player.y + player.x * w] = 0x22;
    return array;
}

void options_check(char* prog_name, int is_server, int port_set, int exp_players, int host_set, int frame_rate, int frame_rate_set, int map_gen_flag, char* hostname, struct sockaddr_in addr){
    if (is_server == 1) { 
        if (!port_set || exp_players < 0 || host_set) {
            print_usage(prog_name);
        }
        if (exp_players > 4 || exp_players < 2) {
            print_error("The number of players can be only 2-4!");
        }
        else if (frame_rate <= 0){
             printf("Running in server mode on port %d, expecting %d players, packet frame_rate is incorrect or not specified\n", ntohs(addr.sin_port), exp_players);
        }
        else printf("Running in server mode on port %d, expecting %d players, frame_rate %d ms\n", ntohs(addr.sin_port), exp_players, frame_rate);

    } else if (is_server == 0) { 
        if (!port_set || !host_set || map_gen_flag || frame_rate_set) {
            print_usage(prog_name);
        }
        printf("Running in client mode, connecting to %s on port %d.\n", hostname, ntohs(addr.sin_port));
    } else {
        print_usage(prog_name);
        }
}

void create_map(int map_gen_flag, uint8_t** map){
    if (!map_gen_flag){
        *map = default_map;
    }
    else {
        *map = Generate_map(QUARTER_WIDTH, QUARTER_HEIGHT, RATIO);
    }
}

char get_next_field(point* next_step){
    if (next_step->x<0 || next_step->y<0 || next_step->y>=FULL_HEIGHT || next_step->x>=FULL_WIDTH){
        return 'E';
    }
    char symbol = (char) mvinch(start_y + next_step->y, start_x + next_step->x); 
    return symbol;
}

point get_next_step(uint32_t direction, uint32_t current_x, uint32_t current_y){
    point next_position;
    switch (direction) {
        case DOWN: {
            next_position.y = current_y+1;
            next_position.x = current_x;
            break;
        }
        case LEFT: {
            next_position.y = current_y;
            next_position.x = current_x-1;
            break;
        }
        case UP: {
            next_position.y = current_y-1;
            next_position.x = current_x;
            break;
        }
        case RIGHT: {
            next_position.y = current_y;
            next_position.x = current_x+1;
            break;
        }
    }
    return next_position;
}

char uint8ToChar(uint8_t ch) {
    switch (ch) {
        case 0xff: return '#';
        case 0xaa: return '.';
        case 0x22: return '0';
        default: return ' ';
    }
}

void clear_players(uint8_t* array, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (array[i] == 0x22) {
            array[i] = 0xaa;
        }
    }
}

int count_of_food_on_quarter(uint8_t* map, int size) {
    int count = 0;
    uint8_t element = 0xaa;
    for (int i = 0; i < size; i++) {
        if (map[i] == element) {
            count++;
        }
    }
    return count;
}

void init_session(){
    initscr();  
    start_color();    
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);  
    init_color(COLOR_MAGENTA, 1000, 0, 1000);
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width);
    start_y = (term_height - FULL_HEIGHT) / 2;
    start_x = (term_width - FULL_WIDTH) / 2;      
    noecho();            
    curs_set(FALSE); 
    cbreak();            
    keypad(stdscr, TRUE); 
    nodelay(stdscr, TRUE);
    pthread_t keyboard_thread;
    pthread_create(&keyboard_thread, NULL, keyboard_handler, NULL);
}

int get_index_by_sd(int sd, struct player* players, int players_count){
    for (int i=0; i<players_count; i++){
        if (players[i].sd==sd) return i;
    }
    return -1;
}

int get_index_by_name(char* name, struct player* players, int players_count){
    for (int i=0; i<players_count; i++){
        if (strcmp((const char *)players[i].player_name, name)==0) return i;
    }
    return -1;
}

void draw_border(int start_y, int start_x, int height, int width) {
    for (int y = start_y - 1; y <= start_y + height; y++) {
        for (int x = start_x - 1; x <= start_x + width; x++) {
            if (y == start_y - 1 || y == start_y + height || x == start_x - 1 || x == start_x + width) {
                mvaddch(y, x, '#');
            }
        }
    }
}

void get_winner(struct player* players, size_t num_players) {
    struct player* winner = NULL;
    uint32_t max_score = 0;
    for (int i = 0; i < num_players; i++) {
        if (players[i].score > max_score) {
            max_score = players[i].score;
            winner = &players[i];
        }
    }
    clear();            
    if (winner != NULL) {
        char buffer[256];
        memcpy(buffer, winner->player_name, winner->player_name_len);
        buffer[winner->player_name_len] = '\0'; 
        clear();
        printf("Winner is %s with a score of %u", buffer, winner->score);
    } else {
        printf("No winner found.");

    }
        refresh();                     
}

void draw_all_borders(int players_count, int field_start_y, int field_start_x, int field_height, int field_width) {
    int box_height = 6;  
    int box_width = 20;  

    int offset_y = field_height / 2 - box_height / 2;  

    for (int i=0;i<players_count;i++){
        switch (i) {
        case 0: 
            draw_border(field_start_y + offset_y - box_height, field_start_x - box_width - 4, box_height, box_width);
            break;
        case 1: 
            draw_border(field_start_y + offset_y - box_height, field_start_x + field_width + 4, box_height, box_width);
            break;
        case 2:
            draw_border(field_start_y + offset_y + box_height + 2, field_start_x - box_width - 4, box_height, box_width);
            break;
        case 3:
            draw_border(field_start_y + offset_y + box_height + 2, field_start_x + field_width + 4, box_height, box_width);
            break;
        }
    }
}

void write_to_field(int id, const char *str1, int score, int field_start_y, int field_start_x, int field_height, int field_width) {
    int box_height = 6; 
    int box_width = 20;  

    int offset_y = field_height / 2 - box_height / 2; 

    int y, x;

    switch (id) {
        case 0: 
            y = field_start_y + offset_y - box_height + 1;
            x = field_start_x - box_width - 2;
            break;
        case 1:
            y = field_start_y + offset_y - box_height + 1;
            x = field_start_x + field_width + 6;
            break;
        case 2:
            y = field_start_y + offset_y + box_height + 3;
            x = field_start_x - box_width - 2;
            break;
        case 3:
            y = field_start_y + offset_y + box_height + 3;
            x = field_start_x + field_width + 6;
            break;
    }

    char score_str[20]; 
    sprintf(score_str, "%d", score); 

    int str1_x = x + (box_width - strlen(str1)) / 2 - 2;
    int str2_x = x + (box_width - strlen(score_str)) / 2 - 2;

    mvprintw(y, str1_x, "%s", str1);
    mvprintw(y + 2, str2_x, "%s", score_str);
}

void render_framestate(int players_count, struct player* players, int global_index){
    point next_step;
    char next_field;
    for (int i=0; i<players_count; i++){ // tipa otrisovka
        next_step = get_next_step(players[i].start_direction, players[i].start_x, players[i].start_y); 
        next_field = get_next_field(&next_step);
        switch (next_field) {
            case '.': {
                attron(global_index == i ? COLOR_PAIR(3) : COLOR_PAIR(1));
                players[i].score++;
                mvaddch(start_y + players[i].start_y, start_x + players[i].start_x, ' ');
                mvaddch(start_y + next_step.y, start_x + next_step.x, '0');
                players[i].start_y = next_step.y;
                players[i].start_x = next_step.x;
                attroff(A_COLOR);
                food_eaten++;
            }
            case ' ': {
                attron(global_index == i ? COLOR_PAIR(3) : COLOR_PAIR(1));
                mvaddch(start_y + players[i].start_y, start_x + players[i].start_x, ' ');
                mvaddch(start_y + next_step.y, start_x + next_step.x, '0');
                players[i].start_y = next_step.y;
                players[i].start_x = next_step.x;
                attroff(A_COLOR);
            }
        }
        write_to_field(i, (const char *)players[i].player_name, players[i].score, start_y, start_x, FULL_HEIGHT, FULL_WIDTH);
    }
    refresh();
}

void game_loop(char* name, int is_server, int frame_rate, struct player* players, int players_count){
    uint8_t fixed_current_direction;
    struct timespec start, end, start1, end1;
    double elapsed;
    int maxd = 0;
    uint8_t recvd_dir;
    int index;
    int is_first_frame=0;
    int sd = players[0].sd;
    index = get_index_by_name(name, players, players_count);
    int ready_c;
    struct player_dir_pkt player_dir_packet;
    struct pollfd fdset[players_count+3];

    if (is_server){
        for (int i=1; i<players_count; i++){
            int cd = players[i].sd;
            fdset[cd].fd = cd;
            fdset[cd].events = POLLIN;
            if (cd>maxd) maxd=cd;
        }
    } else {
        fdset[sd].fd = sd;
        fdset[sd].events = POLLIN;
        maxd = sd;
    }

    int global_index = get_index_by_name(name, players, players_count);
    CURRENT_DIRECTION = players[global_index].start_direction;
    int previous_direction = CURRENT_DIRECTION;

    while (GAME_RUNNING){ 
        clock_gettime(CLOCK_MONOTONIC, &start1);
        msleep(0.25 * frame_rate);
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        while (TRUE){
            clock_gettime(CLOCK_MONOTONIC, &end);
            elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
            if (elapsed >= 0.5 * frame_rate) break;
            fixed_current_direction = CURRENT_DIRECTION;
            if (fixed_current_direction!=previous_direction){
                if (is_server){
                    for (int i=1; i<players_count; i++){
                        broadcast_dir_pkt(players[i].sd, fixed_current_direction, name);    
                    }
                    players[0].start_direction = fixed_current_direction;
                } else {
                    players[global_index].start_direction = fixed_current_direction;
                    send_to_server_dir_pkt(sd, fixed_current_direction);
                }
                previous_direction=fixed_current_direction;
            }
    
            ready_c = poll(fdset, maxd+1, 0);
            for (int i = 0; i <= maxd; ++i) {
                if (fdset[i].revents & POLLIN) {
                    if (is_server){
                        recvd_dir = recv_dir_pkt_on_server(i);
                        index = get_index_by_sd(i, players, players_count);
                        players[index].start_direction = recvd_dir;
                        for (int j=1; j<players_count; j++){
                            if (players[j].sd != i){
                                broadcast_dir_pkt(players[j].sd, recvd_dir, (char *)players[index].player_name);
                            }   
                        }
                    } else {
                        player_dir_packet = recv_from_server_broadcast_pkt(i);
                        index = get_index_by_name(player_dir_packet.playername, players, players_count);
                        players[index].start_direction = player_dir_packet.direction;
                    }
                    if (--ready_c==0) continue;
                }
            }
        }
        
        while (true){
            clock_gettime(CLOCK_MONOTONIC, &end1);
            elapsed = (end1.tv_sec - start1.tv_sec) * 1000.0 + (end1.tv_nsec - start1.tv_nsec) / 1000000.0;     
            if ((int)elapsed>=frame_rate){
                break;
            }
        }

        render_framestate(players_count, players, global_index);
        if (food_eaten==count_of_food){
            get_winner(players, players_count);
            GAME_RUNNING = 0;
            break;
        } 
    } 
}

void init_map(uint8_t* quarter, struct player* players, int players_count) {
    clear_players(quarter, QUARTER_HEIGHT*QUARTER_WIDTH);
    attron(COLOR_PAIR(4));
    draw_border(start_y, start_x, FULL_HEIGHT, FULL_WIDTH);
    attroff(COLOR_PAIR(4));
    draw_all_borders(players_count, start_y, start_x, FULL_HEIGHT, FULL_WIDTH);
    char fullMap[FULL_WIDTH * FULL_HEIGHT];

    for (int y = 0; y < QUARTER_HEIGHT; y++) {
        for (int x = 0; x < QUARTER_WIDTH; x++) {
            int index = y * QUARTER_WIDTH + x;
            fullMap[y * FULL_WIDTH + x] = uint8ToChar(quarter[index]); // Top-left quadrant
            fullMap[y * FULL_WIDTH + (FULL_WIDTH - 1 - x)] = uint8ToChar(quarter[index]);  // Top-right quadrant
            fullMap[(FULL_HEIGHT - 1 - y) * FULL_WIDTH + x] = uint8ToChar(quarter[index]); // Bottom-left quadrant
            fullMap[(FULL_HEIGHT - 1 - y) * FULL_WIDTH + (FULL_WIDTH - 1 - x)] = uint8ToChar(quarter[index]); // Bottom-right quadrant
        }
    }

    for (int y = 0; y < FULL_HEIGHT; y++) {
        for (int x = 0; x < FULL_WIDTH; x++) {
                mvaddch(start_y + y, start_x + x, fullMap[y * FULL_WIDTH + x]);            
        }
    }

    for (int i=0;i<players_count;i++){
        mvaddch(start_y + players[i].start_y, start_x + players[i].start_x, '0');
    }
    refresh();    
}

point find_element_position(uint8_t* array, int array_size, int width) {
    point result = {-1, -1}; 
    for (int i = 0; i < array_size; i++) {
        if (array[i] == 0x22) { 
            result.x = i % width; 
            result.y = i / width;
            break; 
        }
    }
    return result;
}

int name_exists(struct player players[], const char* playername, int players_count) {
    for (int i = 0; i < players_count; i++) {
        if (strcmp((const char*)players[i].player_name, playername) == 0) {
            return 1;
        }
    }
    return 0; 
}

int check_next_step(point p, uint8_t* array, int width, int height, int direction) {
    int next_x = p.x;
    int next_y = p.y; 
    switch (direction) {
        case UP: 
            next_y -= 1;
            break;
        case RIGHT: 
            next_x += 1;
            break;
        case DOWN:
            next_y += 1;
            break;
        case LEFT: 
            next_x -= 1;
            break;
    }
    if (next_x >= 0 && next_x < width && next_y >= 0 && next_y < height && array[next_y * width + next_x] == 0xaa) {
        return direction;
    }
    return -1; 
}

int get_direction(point p, uint8_t* array, int width, int height) {
    int direction;
    do {
        direction = rand() % 4; 
    } while (check_next_step(p, array, width, height, direction) == -1); 

    return direction;
}

void set_players_positions(int exp_players, struct player (*players)[exp_players], uint8_t* map, int w, int h){
    
    point player_stdpos = find_element_position(map, QUARTER_HEIGHT * QUARTER_WIDTH, QUARTER_WIDTH);
    int n = get_direction(player_stdpos, map, QUARTER_WIDTH, QUARTER_HEIGHT);
    int tmp;
    for (int i=0; i<exp_players; i++){
        switch (i) {
            case 0: { // Top-left player (id = 0) - server
                memcpy(&(*players)[0].start_x, &player_stdpos.x, sizeof(uint32_t)); 
                memcpy(&(*players)[0].start_y, &player_stdpos.y, sizeof(uint32_t));
                memcpy(&(*players)[0].start_direction, &n, sizeof(uint32_t));
                break;
            }
            case 1: { // Top-right player (id = 1) 
                tmp = FULL_WIDTH - player_stdpos.x - 1;
                memcpy(&(*players)[1].start_x, &tmp, sizeof(uint32_t)); 
                memcpy(&(*players)[1].start_y, &player_stdpos.y, sizeof(uint32_t));
                if (n==DOWN || n==UP){
                    memcpy(&(*players)[1].start_direction, &n, sizeof(uint32_t));
                } else {
                    tmp = n ^ 2;
                    memcpy(&(*players)[1].start_direction, &tmp, sizeof(uint32_t));
                }
                break;
            }
            case 2: { // Bot-left player (id = 2)
                tmp = FULL_HEIGHT - player_stdpos.y - 1;
                memcpy(&(*players)[2].start_x, &player_stdpos.x, sizeof(uint32_t)); 
                memcpy(&(*players)[2].start_y, &tmp, sizeof(uint32_t));
                if (n==LEFT || n==RIGHT){
                    memcpy(&(*players)[2].start_direction, &n, sizeof(uint32_t));
                } else {
                    tmp = n ^ 2;
                    memcpy(&(*players)[2].start_direction, &tmp, sizeof(uint32_t));
                }
                break;
            }
            case 3: { // Bot-right player (id = 3)
                tmp = FULL_WIDTH - player_stdpos.x - 1;
                memcpy(&(*players)[3].start_x, &tmp, sizeof(uint32_t));  
                tmp = FULL_HEIGHT - player_stdpos.y - 1;
                memcpy(&(*players)[3].start_y, &tmp, sizeof(uint32_t));
                tmp = n ^ 2;
                memcpy(&(*players)[3].start_direction, &tmp, sizeof(uint32_t)); 
                break;
            }
        }
    }
}

void start_game(int players_count, struct player players[players_count], int frame_rate){
    uint32_t start_pkt_size = players_count * (sizeof(struct player)-256) + sizeof(uint32_t)*2;
    for (int i = 0; i < players_count; i++) {
        start_pkt_size += players[i].player_name_len;
    }
    
    pkt_header pkt = {htonl(MAGIC), htonl(0x00000020), htonl(start_pkt_size)};
    uint32_t frame_timeout = htonl(frame_rate);
    uint32_t p_count = htonl(players_count);

    for (int i = 0; i < players_count; i++) {
        players[i].start_x = htonl(players[i].start_x);
        players[i].start_y = htonl(players[i].start_y);
        players[i].start_direction = htonl(players[i].start_direction);
        players[i].player_name_len = htonl(players[i].player_name_len);
    }

    for (int i = 1; i<players_count; i++){
        send(players[i].sd, &pkt, sizeof(pkt), 0);
        send(players[i].sd, &frame_timeout, sizeof(frame_timeout), 0);
        send(players[i].sd, &p_count, sizeof(p_count), 0);
        for (int j = 0; j<players_count; j++){
            send(players[i].sd, &players[j].start_x, sizeof(uint32_t), 0);
            send(players[i].sd, &players[j].start_y, sizeof(uint32_t), 0);
            send(players[i].sd, &players[j].start_direction, sizeof(uint32_t), 0);
            send(players[i].sd, &players[j].player_name_len, sizeof(uint32_t), 0);
            send(players[i].sd, players[j].player_name,  ntohl(players[j].player_name_len), 0); 
        }
    }     

    for (int i = 0; i < players_count; i++) {
        players[i].start_x = ntohl(players[i].start_x);
        players[i].start_y = ntohl(players[i].start_y);
        players[i].start_direction = ntohl(players[i].start_direction);
        players[i].player_name_len = ntohl(players[i].player_name_len);
    }

}

start_game_pkt recv_players_array(int sd){
    start_game_pkt ret;
    pkt_header pkt;
    uint32_t frame_timeout;
    uint32_t p_count;

    if (!recv(sd, (void*)&pkt.magic, sizeof(uint32_t), 0) || ntohl(pkt.magic)!=MAGIC) print_error("unknown magic");
    if (!recv(sd, &pkt.ptype, sizeof(uint32_t), 0) || ntohl(pkt.ptype)!=0x20) print_error("startgame packet structure error");
    if (!recv(sd, &pkt.datasize, sizeof(uint32_t), 0)) print_error("startgame packet structure error");
    pkt.datasize = ntohl(pkt.datasize);
    if (!recv(sd, &frame_timeout, sizeof(uint32_t), 0)) print_error("startgame packet frame_timeout error");
    if (!recv(sd, &p_count, sizeof(uint32_t), 0)) print_error("startgame packet p_count error");
    pkt.datasize -= sizeof(uint32_t)*2;
    
    struct player* players =  (struct player*)malloc(sizeof(struct player) * ntohl(p_count));

    for (int i = 0; i < ntohl(p_count); i++) {
        if (!recv(sd, &players[i].start_x, sizeof(uint32_t), 0)) print_error("startgame packet start_x error");
        players[i].start_x = ntohl(players[i].start_x);

        if (!recv(sd, &players[i].start_y, sizeof(uint32_t), 0)) print_error("startgame packet start_y error");
        players[i].start_y = ntohl(players[i].start_y);

        if (!recv(sd, &players[i].start_direction, sizeof(uint32_t), 0)) print_error("startgame packet start_direction error");
        players[i].start_direction = ntohl(players[i].start_direction);
        
        if (!recv(sd, &players[i].player_name_len, sizeof(uint32_t), 0)) print_error("startgame packet player_name_len error");
        players[i].player_name_len = ntohl(players[i].player_name_len);

        int nka = recv(sd, players[i].player_name, players[i].player_name_len, 0);
    }
    
    ret.frame_timeout = ntohl(frame_timeout);
    ret.p_count = ntohl(p_count);
    ret.players = players;
    return ret;
}

void init_a_server(struct sockaddr_in* addr, int exp_players, int map_gen_flag, int frame_rate){
    uint8_t* map;
    char* playername;
    int player_name_l;
    struct player players[exp_players];
    int ready_players=1;
    int players_count = 1;
    create_map(map_gen_flag, &map);
    count_of_food = count_of_food_on_quarter(map, QUARTER_HEIGHT*QUARTER_WIDTH) * 4 + (4-exp_players);

    set_players_positions(exp_players, &players, map, QUARTER_WIDTH, QUARTER_HEIGHT);

    playername = SERVER_NAME; // init a server as a player
    player_name_l = strlen(playername);
    memset(players[0].player_name, 0, sizeof(players[0].player_name));
    memcpy(players[0].player_name, playername, player_name_l);
    memcpy(&players[0].player_name_len, &player_name_l, sizeof(int));
    memset(&players[0].score, 0, sizeof(players[0].score));

    int sd, cd, maxd;
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) print_error("Setsockopt error");
    if (bind(sd, (const struct sockaddr *) addr, sizeof(*addr)) < 0) print_error("bind error");
    listen(sd, 1);
   
    struct pollfd fdset[exp_players+3];
    fdset[sd].fd = sd;
    fdset[sd].events = POLLIN;
    maxd = sd;
    memcpy(&players[0].sd, &sd, sizeof(int));
    int n;

    while (TRUE) { 
        int ready_c = poll(fdset, maxd + 1, -1);
        
        if (fdset[sd].revents & POLLIN) { // init a new player 
            
            cd = accept(sd, NULL, NULL);
            fdset[cd].fd = cd;
            fdset[cd].events = POLLIN;
            
            memcpy(&players[players_count].sd, &cd, sizeof(int));
            if (cd > maxd) maxd = cd;

            playername = recv_init_pkt(cd);
            if (name_exists(players, playername, players_count)){
                close(cd);
                printf("This name already exists! dropping!"); 
                continue;
            }

            player_name_l = strlen(playername);
            memset(players[players_count].player_name, 0, sizeof(players[players_count].player_name));
            memcpy(players[players_count].player_name, playername, player_name_l);
            memcpy(&players[players_count].player_name_len, &player_name_l, sizeof(uint32_t));
            memcpy(&players[players_count].sd, &cd, sizeof(uint32_t));
            memset(&players[players_count].score, 0, sizeof(players[players_count].score));
            players_count++;

            send_map_pkt(map, cd);

            if (players_count==exp_players){
                close(sd);
                continue;
            }
            if (--ready_c == 0) continue;
        }

        for (int i = 0; i <= maxd; ++i) {
            if (fdset[i].revents & POLLIN) {
                if (GAME_RUNNING == 0){
                    int a = recv_rdy_pkt(i);
                    if (a) {
                        ready_players++;
                        printf("%d/%d players ready\n", ready_players, exp_players);
                    }
                    if (ready_players==exp_players){
                        char* server_tmp_name = SERVER_NAME;
                        GAME_RUNNING=1;
                        init_session();
                        init_map(map, players, players_count);
                          
                        start_game(players_count, players, frame_rate); // sending start_pkts to all players    
                        game_loop(server_tmp_name, 1, frame_rate, players, players_count);
                        endwin(); 
                        break;
                    }
                }
                if (--ready_c == 0) continue;
            }
        }
    }

    if (map_gen_flag){
        free(map);
    }
}

char* get_client_name(){
    char* playername = (char*)malloc(256);
    printf("Enter your name (up to 255 characters): ");
    fgets(playername, 257, stdin);
    size_t namelength = strlen(playername);
    if (namelength <= 1 || playername[namelength - 1] != '\n'){
        print_error("name length is invalid");
    }    
    playername[namelength-1] = '\0'; 
    return playername;
}

void init_a_client(struct sockaddr_in* addr){
    char* playername = get_client_name();
    int sd;
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sd, (const struct sockaddr *) addr, sizeof(struct sockaddr_in)) < 0) print_error("Connect to server error");
    send_init_pkt(playername, sd);
    uint8_t* map = recv_map_pkt(sd);

    printf("Press any button if you are ready!!!\n");
    getchar();
    send_rdy_pkt(sd);

    struct pollfd fdset[4];
    fdset[sd].fd = sd;
    fdset[sd].events = POLLIN;
    int maxd = sd;
    start_game_pkt start_pkt;
    struct player* players;
    int players_count;
    int frame_rate;

    while (true){
        poll(fdset, maxd+1, -1);
        if (fdset[sd].revents & POLLIN) {
            start_pkt = recv_players_array(sd);
            players = start_pkt.players;
            players_count = start_pkt.p_count;
            frame_rate = start_pkt.frame_timeout;
            GAME_RUNNING=1;
            break;
        }
    }

    for (int i=0; i<players_count; i++){
        if (i==0){
            memcpy(&players[i].sd, &sd, sizeof(uint32_t));
        } else {
            memset(&players[i].sd, 0, sizeof(players[i].sd));
        } 
        memset(&players[i].score, 0, sizeof(players[i].score));
    }

    count_of_food = count_of_food_on_quarter(map, QUARTER_HEIGHT*QUARTER_WIDTH) * 4 + (4-players_count);
    setvbuf(stdout, NULL, _IONBF, 0);
    init_session();
    

    init_map(map, players, players_count);
    
    game_loop(playername, 0, frame_rate, players, players_count);
    
    close(sd);
    free(playername);
}

int main(int argc, char *argv[]){
    int is_server = -1;
    int frame_rate = DEFAULT_FRAME_RATE;
    int exp_players = -1;
    int map_gen_flag = 0;
    char host_set = 0;
    int port_set = 0;
    int frame_rate_set = 0;
    char* hostname;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    setvbuf(stdout, NULL, _IONBF, 0);

    int opt;
    while ((opt = getopt(argc, argv, "s:cp:h:r:m")) != -1) {
        switch (opt) {
            case 's':
                exp_players = atoi(optarg);
                is_server = true;
                break;
            case 'c':
                is_server = false;
                break;
            case 'p':
                addr.sin_port = htons(atoi(optarg));
                port_set = true;
                break;
            case 'h':
                addr.sin_addr.s_addr = inet_addr(optarg);
                hostname = optarg;
                host_set = true;
                break;
            case 'r':
                frame_rate = atoi(optarg);
                frame_rate_set = true;
                break;
            case 'm':
                map_gen_flag = true;
                break;
            default:
                print_usage(argv[0]);
        }
    }

    options_check(argv[0], is_server, port_set, exp_players, host_set, frame_rate, frame_rate_set, map_gen_flag, hostname, addr);
    
    switch (is_server) {
        case TRUE:
            init_a_server(&addr,exp_players,map_gen_flag, frame_rate);
            printf("GAME END");
            break;
        case FALSE:
            init_a_client(&addr);
            printf("GAME END");
            break;
    }
    return 0;
}