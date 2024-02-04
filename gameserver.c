#include "gameserver.h"
#include "syscall.h"
#include "nameserver.h"
#include "rpi.h"

char gameserver_name[] = "gameserver";

int gameserver_signup(int tid) {
    GameMsg msg = {
        SIGNUP,
        "",
        0
    };

    // serialization
    char* byte_array = (char*) &msg;

    char reply[sizeof(msg)];

    Send(tid, byte_array, sizeof(msg), reply, sizeof(msg));

    GameMsg* response = (GameMsg*) reply;

    if (response->code == OK) {
        int game_id = response->body[0];
        return game_id;
    } else {
        uart_printf(CONSOLE, "Recieved error\r\n");
    }
}

int gameserver_play(int tid, int match, int action) {

    char body[2] = {match, action};

    GameMsg msg = {
        PLAY,
        body,
        2,
    };

    // serialization
    char* byte_array = (char*) &msg;

    char reply[sizeof(msg)];

    Send(tid, byte_array, sizeof(msg), reply, sizeof(msg));

    GameMsg* response = (GameMsg*) reply;
    if (response->code == OK) {
        int result = response->body[0];
        return result;
    } else {
        uart_printf(CONSOLE, "Recieved error\r\n");
    }
}

int gameserver_quit(int tid, int match) {
    char body = match;

    GameMsg msg = {
        QUIT,
        &body,
        1,
    };

    // serialization
    char* byte_array = (char*) &msg;

    char reply[sizeof(msg)];

    Send(tid, byte_array, sizeof(msg), reply, sizeof(msg));

    GameMsg* response = (GameMsg*) reply;
    if (response->code == OK) {

    } else {
        uart_printf(CONSOLE, "Recieved error\r\n");
    }
}

int winner(int p1move, int p2move) {
    if (p1move == p2move) {
        return 0;
    } else if (p1move == ROCK && p2move == SCISSOR || p1move == PAPER && p2move == ROCK || p1move == SCISSOR && p2move == PAPER) {
        return 1;
    } else {
        return 2;
    }
}

void gameserver() {
    int gameserver_tid = MyTid();
    uart_printf(CONSOLE, "Starting gameserver tid %u\r\n", gameserver_tid);

    RegisterAs(gameserver_name);
    
    // initialize max number of games
    Game games[MAX_GAMES];
    int game_idx = 0;

    for (int i = 0; i < MAX_GAMES; i++) {
        games[i].p1 = NULL;
        games[i].p1_move = NULL;
        games[i].p2 = NULL;
        games[i].p2_move = NULL;
    }

    for (;;) {
        int tid;
        char byte_msg[sizeof(GameMsg)];
        uart_dprintf(CONSOLE, "gameserver tid %u recieving\r\n", gameserver_tid);
        int response = Receive(&tid, byte_msg, sizeof(GameMsg));

        GameMsg* msg = (GameMsg *) byte_msg;

        if (msg->code == SIGNUP) {
            uart_printf(CONSOLE, "tid %u signing up\r\n", tid);

            // check if game is full    
            if (games[game_idx].p1 != NULL && games[game_idx].p2 != NULL) {
                GameMsg res = {
                    ERROR,
                    "",
                    0
                };
                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                continue;
            } else {
                // game is not full, try to set player 1
                if (games[game_idx].p1 == NULL) {
                    games[game_idx].p1 = tid;
                    uart_printf(CONSOLE, "Player 1: %u signed up to game %u\r\n", tid, game_idx);
                    continue;
                // try to set player2
                } else if (games[game_idx].p2 == NULL) {
                    uart_printf(CONSOLE, "Player 2: %u signed up to game %u ready\r\n", tid, game_idx);
                    games[game_idx].p2 = tid;
                    char game = game_idx;
                    GameMsg res = {
                        OK,
                        &game,
                        1
                    };
                    response = Reply(games[game].p1, (char*)&res, sizeof(GameMsg));
                    response = Reply(games[game].p2, (char*)&res, sizeof(GameMsg));

                    game_idx += 1 % MAX_GAMES;
                    continue;
                }
            }

        } else if (msg->code == PLAY) {
            // TODO: assert body is right length
            int game = msg->body[0];
            int move = msg->body[1];

            // check if game is not ready and no one quit
            if (games[game].p1 == NULL && games[game].p1_move != QUIT && games[game].p2 == NULL && games[game].p2_move != QUIT) {
                // TODO: respond game is not ready
                char body[] = "game not ready\0";

                GameMsg res = {
                    ERROR,
                    &body,
                    sizeof(body)
                };

                uart_printf(CONSOLE, "Game %u not ready \r\n", game);
                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                continue;
            }
            // game is ready or a player as quit

            if (games[game].p1 == tid) {
                // check if other person quit
                if (games[game].p2 == NULL && games[game].p2_move == QUIT) {

                    // response to p1 to quit
                    char body = QUIT;

                    GameMsg res = {
                        OK,
                        &body,
                        sizeof(body)
                    };

                    uart_printf(CONSOLE, "Responded to Player 1: %u to quit in game %u\r\n", tid, game);
                    response = Reply(tid, (char*)&res, sizeof(GameMsg));
                    continue;
                } else if (games[game].p1_move != NULL) {
                    // TODO: error check, already made a move
                    continue;
                }

                games[game].p1_move = move;
                uart_printf(CONSOLE, "Player 1: %u played %u in game %u\r\n", games[game].p1, move, game);
                
            } else if (games[game].p2 == tid) {
                // check if other person quit
                if (games[game].p1 == NULL && games[game].p1_move == QUIT) {

                    // response to p2 to quit
                    char body = QUIT;

                    GameMsg res = {
                        OK,
                        &body,
                        sizeof(body)
                    };

                    uart_printf(CONSOLE, "Responded to Player 2: %u to quit in game %u\r\n", tid, game);
                    response = Reply(tid, (char*)&res, sizeof(GameMsg));
                    continue;
                } else if (games[game].p2_move != NULL) {
                    // TODO: error check, already made a move
                    continue;
                }

                games[game].p2_move = move;
                uart_printf(CONSOLE, "Player 2: %u played %u in game %u\r\n", games[game].p2, move, game);
            } else {
                // TODO: throw error
            }

            // GAME IS READY, maybe add a non quit check?
            if (games[game].p1 != NULL && games[game].p1_move != NULL && games[game].p2 != NULL && games[game].p2_move !=NULL) {
                int p1_move = games[game].p1_move;
                int p2_move = games[game].p2_move;

                char p1;
                GameMsg p1_res = {
                    OK,
                    &p1,
                    1
                };

                char p2;
                GameMsg p2_res = {
                    OK,
                    &p2,
                    1
                };

                int win = winner(games[game].p1_move, games[game].p2_move);

                if (win == 0) {
                    p1 = TIE;
                    p2 = TIE;
                    uart_printf(CONSOLE, "Player 1: %u, Player 2: %u in game %u tied\r\n", games[game].p1, games[game].p2, game);
                } else if (win == 1) {
                    p1 = WIN;
                    p2 = LOSE;
                    uart_printf(CONSOLE, "Player 1: %u in game %u won\r\n", games[game].p1, game);
                } else if (win == 2) {
                    p1 = LOSE;
                    p2 = WIN;
                    uart_printf(CONSOLE, "Player 2: %u in game %u won\r\n", games[game].p2, game);
                }

                response = Reply(games[game].p1, (char*)&p1_res, sizeof(GameMsg));
                response = Reply(games[game].p2, (char*)&p2_res, sizeof(GameMsg));
                continue;
            }
        } else if (msg->code == QUIT) {
            int game = msg->body[0];
            
            // check if game is ready and both players haven't quit already
            if (games[game].p1 == NULL && games[game].p1_move != QUIT && games[game].p2 == NULL && games[game].p2_move != QUIT) {
                // TODO: respond game is not ready
                char body[] = "game not ready\0";

                GameMsg res = {
                    ERROR,
                    &body,
                    sizeof(body)
                };

                uart_printf(CONSOLE, "Game %u not ready \r\n", game);
                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                continue;
            }

            if (games[game].p1 == tid) {
                games[game].p1 = NULL;
                games[game].p1_move = QUIT;

                // check if other player quit
                if (games[game].p2 == NULL && games[game].p2_move == QUIT) {
                    games[game].p1_move = NULL;
                    games[game].p2_move = NULL;
                }

                GameMsg res = {
                    OK,
                    "",
                    0
                };

                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                uart_printf(CONSOLE, "Player 1: %u quit in game %u\r\n", tid, game);
                continue;
            } else if (games[game].p2 == tid) {
                games[game].p2 = NULL;
                games[game].p2_move = QUIT;

                // check if other player quit
                if (games[game].p1 == NULL && games[game].p1_move == QUIT) {
                    games[game].p1_move = NULL;
                    games[game].p2_move = NULL;
                }

                GameMsg res = {
                    OK,
                    "",
                    0
                };
                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                uart_printf(CONSOLE, "Player 2: %u quit in game %u\r\n", tid, game);
                continue;
            }
        } else {
            // TODO: error
            uart_printf(CONSOLE, "Invalid code %u from %u \r\n", msg->code, tid);    
        }
    }
}
