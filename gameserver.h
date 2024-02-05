#ifndef GAMESERVER_H
#define GAMESERVER_H

#define MAX_GAMES 5
#define MAX_MSG_BODY_SIZE 128


typedef enum RequestCode {
    SIGNUP = 1,
    PLAY,
    QUIT
} RequestCode;

typedef enum ResponseCode {
    OK = 1,
    ERROR
} ResponseCode;

typedef enum Result {
    WIN,
    LOSE,
    TIE,
    OPPONENT_QUIT
} Result;

typedef enum Move {
    ROCK,
    PAPER,
    SCISSOR
} Move;

extern char gameserver_name[11];
extern Move moves[3];
extern char* move_str[3];
extern char* result_str[4];


typedef struct GameMsg {
    int code;
    char body[MAX_MSG_BODY_SIZE];
    int body_length;
} GameMsg; 

typedef struct Game {
    int p1;
    int p1_move;
    int p2;
    int p2_move;
} Game;

int gameserver_signup(int tid);
int gameserver_play(int tid, int match, int action);
int gameserver_quit(int tid, int match);
void gameserver();

#endif
