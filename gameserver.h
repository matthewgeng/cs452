#ifndef GAMESERVER_H
#define GAMESERVER_H

#define MAX_GAMES 10

// TODO: use an enum?
#define SIGNUP 1
#define PLAY 2
#define QUIT 3

#define LOSE 4
#define TIE 5
#define WIN 6

#define ROCK 7
#define PAPER 8
#define SCISSOR 9

typedef enum ResponseCode {
    OK,
    ERROR
} ResponseCode;

typedef struct GameMsg {
    int code;
    char* body;
    int body_length;
} GameMsg; 

typedef struct Game {
    int p1;
    int p1_move;
    int p2;
    int p2_move;
} Game;

void gameserver_init(char* name);
int gameserver_signup();
int gameserver_play(int match, char action);
int gameserver_quit(int match);
void gameserver();

#endif
