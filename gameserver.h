#ifndef GAMESERVER_H
#define GAMESERVER_H

typedef struct GameMsg {
    char type;
    char* body;
    int body_length;
} GameMsg; 

void gameserver_init(char* name);
int gameserver_signup();
int gameserver_play(int match, char action);
int gameserver_quit(int match);

#endif
