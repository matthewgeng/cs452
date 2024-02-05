#include "gameserver.h"
#include "syscall.h"
#include "rpi.h"
#include "gameclient.h"
#include "nameserver.h"

int lookup_gameserver(int tid) {
    uart_printf(CONSOLE, "[client %u] REQUEST : nameserver for gameserver tid \r\n", tid);
    int gs = WhoIs(gameserver_name);
    uart_printf(CONSOLE, "[client %u] receive : gameserver tid %u\r\n", tid, gs);
    return gs;
}

int signup(int tid, int gameserver_tid) {
    uart_printf(CONSOLE, "[client %u] REQUEST : game SIGNUP\r\n", tid);
    int game = gameserver_signup(gameserver_tid);
    if (game == -1) {
        uart_printf(CONSOLE, "\x1b[31m[client %u] RECEIVE : ERROR signing up for game \x1b[0m\r\n", tid);
    } else {
        uart_printf(CONSOLE, "[client %u] RECEIVE : GAME %u signup\r\n", tid, game);
    }
    return game;
}

int move(int tid, int gs, int game, Move move) {
    uart_printf(CONSOLE, "[client %u] REQUEST : PLAY %s, GAME %u \r\n", tid, move_str[move], game);
    int result = gameserver_play(gs, game, move);
    if (result == -1) {
        uart_printf(CONSOLE, "\x1b[31m[client %u] RECEIVE : ERROR playing %s in game %u\x1b[0m\r\n", tid, move_str[move], game);
    } else {
        uart_printf(CONSOLE, "[client %u] RECEIVE : GAME %u, RESULT %s\r\n", tid, game, result_str[result]);
    }
    return result;
}

int quit(int tid, int gs, int game) {
    uart_printf(CONSOLE, "[client %u] REQUEST : QUIT, GAME %u \r\n", tid, game);
    int response = gameserver_quit(gs, game);
    if (response == -1) {
        uart_printf(CONSOLE, "\x1b[31m[client %u] RECEIVE : ERROR trying to QUIT in game %u\x1b[0m\r\n", tid, game);
    } else {
        uart_printf(CONSOLE, "[client %u] RECEIVE : GAME %u QUIT \r\n", tid, game);
    }
    return response;
}

void gameclient1() {
    int tid = MyTid();
    uart_printf(CONSOLE, "[client %u] starting\r\n", tid);

    int gs = lookup_gameserver(tid);
    int game = signup(tid, gs);

    for (int i = 0; i < sizeof(moves)/sizeof(moves[0]); i++) {
        int result = move(tid, gs, game, moves[i]);
        if (result == -1) {
            return;
        }
    }

    quit(tid, gs, game);
}

void gameclient2() {
    int tid = MyTid();
    uart_printf(CONSOLE, "[client %u] starting\r\n", tid);

    int gs = lookup_gameserver(tid);

    int game = signup(tid, gs);

    for (int i = 0; i < sizeof(moves)/sizeof(moves[0]); i++) {
        int result = move(tid, gs, game, moves[i]);
        if (result == -1) {
            return;
        }
    }

    int result = move(tid, gs, game, ROCK);
    if (result == -1) {
        return;
    }

    if (result == OPPONENT_QUIT) {
        quit(tid, gs, game);
    }
}


void gameclient3() {
    int tid = MyTid();
    uart_printf(CONSOLE, "[client %u] starting\r\n", tid);

    int gs = lookup_gameserver(tid);
    int game = signup(tid, gs);

    for (int i = 0; i < sizeof(moves)/sizeof(moves[0]); i++) {
        int result = move(tid, gs, game, moves[i]);
        if (result == -1) {
            return;
        }
    }

    int result = move(tid, gs, game, ROCK);
    if (result == -1) {
        return;
    }

    if (result == OPPONENT_QUIT) {
        quit(tid, gs, game);
    }
}

void gameclient4() {
    int tid = MyTid();
    uart_printf(CONSOLE, "[client %u] starting\r\n", tid);

    int gs = lookup_gameserver(tid);
    int game = signup(tid, gs);

    for (int i = sizeof(moves)/sizeof(moves[0])-1; i >=0 ; i--) {
        int result = move(tid, gs, game, moves[i]);
        if (result == -1) {
            return;
        }
    }

    int result = quit(tid, gs, game);

    if (result == -1) {
        return;
    }
}

void test1() {
    Create(3, &gameclient1);
    Create(3, &gameclient2);
    Create(3, &gameclient3);
    Create(3, &gameclient4);
}

void test2() {
    Create(4, &gameclient1);
    Create(5, &gameclient2);
    Create(6, &gameclient3);
    Create(7, &gameclient4);
}
