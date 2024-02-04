#include "gameserver.h"
#include "syscall.h"
#include "rpi.h"
#include "gameclient.h"
#include "nameserver.h"

int moves[] = {ROCK, PAPER, SCISSOR};
char rock_str[] = "rock";
char paper_str[] = "paper";
char scissor_str[] = "scissor";

char win_str[] = "win";
char lose_str[] = "lose";
char tie_str[] = "tie";

void gameclient1() {
    int tid = MyTid();
    uart_printf(CONSOLE, "starting game client tid %u\r\n", tid);

    uart_printf(CONSOLE, "gc %u requesting gameserver tid\r\n", tid);
    int gs = WhoIs(gameserver_name);
    uart_printf(CONSOLE, "gc %u recieved gameserver tid %u\r\n", tid, gs);
    
    uart_printf(CONSOLE, "gc %u requesting signup\r\n", tid);
    int game = gameserver_signup(gs);
    uart_printf(CONSOLE, "gc %u signed up for game %u\r\n", tid, game);

    uart_printf(CONSOLE, "gc %u requesting to play \r\n", tid, game);
    int result = gameserver_play(gs, game, ROCK);
    uart_printf(CONSOLE, "gc %u result in game %u = %u\r\n", tid, game, result);
}

void gameclient2() {
    int tid = MyTid();
    uart_printf(CONSOLE, "starting game client tid %u\r\n", tid);

    uart_printf(CONSOLE, "gc %u requesting gameserver tid\r\n", tid);
    int gs = WhoIs(gameserver_name);
    uart_printf(CONSOLE, "gc %u recieved gameserver tid %u\r\n", tid, gs);
    
    uart_printf(CONSOLE, "gc %u requesting signup\r\n", tid);
    int game = gameserver_signup(gs);
    uart_printf(CONSOLE, "gc %u signed up for game %u\r\n", tid, game);

    uart_printf(CONSOLE, "gc %u requesting to play \r\n", tid, game);
    int result = gameserver_play(gs, game, PAPER);
    uart_printf(CONSOLE, "gc %u result in game %u = %u\r\n", tid, game, result);
}
