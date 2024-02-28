#include "gameserver.h"
#include "syscall.h"
#include "nameserver.h"
#include "rpi.h"
#include "util.h"

char gameserver_name[] = "gameserver";

Move moves[] = {ROCK, PAPER, SCISSOR};
char rock_str[] = "rock";
char paper_str[] = "paper";
char scissor_str[] = "scissor";
char *move_str[] = {rock_str, paper_str, scissor_str};

char win_str[] = "win";
char lose_str[] = "lose";
char tie_str[] = "tie";
char quit_str[] = "opponent quit";
char *result_str[] = {win_str, lose_str, tie_str, quit_str};

int gameserver_signup(int tid) {
    GameMsg msg;
    set_response(&msg, SIGNUP, "", 0);

    // serialization
    char* byte_array = (char*) &msg;

    char reply[sizeof(GameMsg)];

    Send(tid, byte_array, sizeof(GameMsg), reply, sizeof(GameMsg));

    GameMsg* response = (GameMsg*) reply;

    if (response->code == OK) {
        int game_id = response->body[0];
        return game_id;
    } else {
        uart_printf(CONSOLE, "received error: %s\r\n", response->body);
        return -1;
    }
}

int gameserver_play(int tid, int match, int action) {

    char body[2] = {match, action};
    GameMsg msg;

    set_response(&msg, PLAY, body, sizeof(body));

    // serialization
    char* byte_array = (char*) &msg;

    char reply[sizeof(GameMsg)];

    Send(tid, byte_array, sizeof(GameMsg), reply, sizeof(GameMsg));

    GameMsg* response = (GameMsg*) reply;
    if (response->code == OK) {
        int result = response->body[0];
        return result;
    } else {
        uart_printf(CONSOLE, "received error: %s\r\n", response->body);
        return -1;
    }
}

int gameserver_quit(int tid, int match) {
    char body[] = {match};

    GameMsg msg;

    set_response(&msg, QUIT, body, sizeof(body));

    // serialization
    char* byte_array = (char*) &msg;

    char reply[sizeof(GameMsg)];

    Send(tid, byte_array, sizeof(GameMsg), reply, sizeof(GameMsg));

    GameMsg* response = (GameMsg*) reply;
    if (response->code == OK) {
        return 0;
    } else {
        #if DEBUG
            uart_dprintf(CONSOLE, "received error: %s\r\n", response->body);
        #endif 
        return -1;
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

void set_response(GameMsg* gm, int code, char* body, int body_length) {
    memset(gm->body, 0, MAX_MSG_BODY_SIZE);
    int min_length = (body_length < MAX_MSG_BODY_SIZE) ? body_length : MAX_MSG_BODY_SIZE;
    gm->code = code;
    gm->body_length = body_length;
    memcpy(gm->body, body, min_length);
}

void gameserver() {
    int gameserver_tid = MyTid();
    uart_printf(CONSOLE, "Starting gameserver tid %u\r\n", gameserver_tid);

    RegisterAs(gameserver_name);
    
    // initialize max number of games
    Game games[MAX_GAMES];
    int game_idx = 0;

    for (int i = 0; i < MAX_GAMES; i++) {
        games[i].p1 = -1;
        games[i].p1_move = -1;
        games[i].p2 = -1;
        games[i].p2_move = -1;
    }


    for (;;) {
        int tid;
        char byte_msg[sizeof(GameMsg)];
        #if DEBUG
            uart_dprintf(CONSOLE, "gameserver tid %u receiving\r\n", gameserver_tid);
        #endif 
        int response = Receive(&tid, byte_msg, sizeof(GameMsg));

        GameMsg* msg = (GameMsg *) byte_msg;
        
        if (msg->code == SIGNUP) {
            #if DEBUG
                uart_dprintf(CONSOLE, "[SERVER] tid %u signing up\r\n", tid);
            #endif 

            // check if game is full    
            if (games[game_idx].p1 != -1 && games[game_idx].p2 != -1) {
                uart_printf(CONSOLE, "Game %u is full\r\n", game_idx);
                char body[] = "game is full";
                GameMsg res;
                set_response(&res, ERROR, body, sizeof(body));
                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                continue;
            } else {
                // game is not full, try to set player 1
                if (games[game_idx].p1 == -1) {
                    games[game_idx].p1 = tid;
                    uart_printf(CONSOLE, "[SERVER] : CLIENT %u signup GAME %u (1/2 READY)\r\n", tid, game_idx);
                    continue;
                // try to set player2
                } else if (games[game_idx].p2 == -1) {
                    uart_printf(CONSOLE, "[SERVER] : CLIENT %u signup GAME %u (2/2 READY)\r\n", tid, game_idx);
                    games[game_idx].p2 = tid;
                    char body[] = {game_idx};
                    GameMsg res;
                    set_response(&res, OK, body, sizeof(body));

                    // uart_printf(CONSOLE, "[SERVER] : RESPONDING TO CLIENTS %u, %u with GAME %u GameMsg %x\r\n", games[game_idx].p1, games[game_idx].p2, res.body[0], (char*)&res);
                    // uart_printf(CONSOLE, "game idx %u game 0 tid 1 %u, tid2 %u, game 1, tid1 %u, tid2 %u\r\n", game_idx, games[0].p1, games[0].p2, games[1].p1, games[1].p2);

                    response = Reply(games[game_idx].p1, (char*)&res, sizeof(res));
                    response = Reply(games[game_idx].p2, (char*)&res, sizeof(res));
                    game_idx = (game_idx + 1) % MAX_GAMES;
                    continue;
                }
            }

        } else if (msg->code == PLAY) {
            #if DEBUG
                uart_dprintf(CONSOLE, "[SERVER] tid %u playing\r\n", tid);
            #endif 
            // TODO: assert body is right length
            int game = msg->body[0];
            int move = msg->body[1];

            // check if game is not ready and no one quit
            if (games[game].p1 == -1 && games[game].p1_move != QUIT && games[game].p2 == -1 && games[game].p2_move != QUIT) {
                // TODO: respond game is not ready
                char body[] = "game not ready";

                GameMsg res;
                set_response(&res, ERROR, body, sizeof(body));

                uart_printf(CONSOLE, "Game %u not ready \r\n", game);
                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                continue;
            }
            // game is ready or a player as quit

            if (games[game].p1 == tid) {
                // check if other person quit
                if (games[game].p2 == -1 && games[game].p2_move == QUIT) {

                    // response to p1 to quit
                    char body[] = {OPPONENT_QUIT};

                    GameMsg res;
                    set_response(&res, OK, body, sizeof(body));

                    uart_printf(CONSOLE, "[SERVER] : CLIENT %u PLAY IGNORED, OPPONENT QUIT, GAME %u, \r\n", tid, game);
                    response = Reply(tid, (char*)&res, sizeof(GameMsg));
                    continue;
                } else if (games[game].p1_move != -1) {
                    // TODO: error check, already made a move
                    continue;
                }

                games[game].p1_move = move;
                uart_printf(CONSOLE, "[SERVER] : CLIENT %u PLAYED %s, GAME %u\r\n", tid, move_str[move], game);
                
            } else if (games[game].p2 == tid) {
                // check if other person quit
                if (games[game].p1 == -1 && games[game].p1_move == QUIT) {

                    // response to p2 to quit
                    char body[] = {OPPONENT_QUIT};
                    GameMsg res;

                    set_response(&res, OK, body, sizeof(body));

                    uart_printf(CONSOLE, "[SERVER] : CLIENT %u PLAY IGNORED, OPPONENT QUIT, GAME %u, \r\n", tid, game);
                    response = Reply(tid, (char*)&res, sizeof(GameMsg));
                    continue;
                } else if (games[game].p2_move != -1) {
                    // TODO: error check, already made a move
                    continue;
                }

                games[game].p2_move = move;
                uart_printf(CONSOLE, "[SERVER] : CLIENT %u PLAYED %s, GAME %u\r\n", tid, move_str[move], game);
            } else {
                // TODO: throw error

                char body[] = "not registered for this game";
                GameMsg res;
                set_response(&res, ERROR, body, sizeof(body));

                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                continue;
            }

            // GAME IS READY, maybe add a non quit check?
            if (games[game].p1 != -1 && games[game].p1_move != -1 && games[game].p2 != -1 && games[game].p2_move !=-1) {
                int p1_move = games[game].p1_move;
                int p2_move = games[game].p2_move;

                GameMsg p1_res;
                GameMsg p2_res;

                int win = winner(games[game].p1_move, games[game].p2_move);

                if (win == 0) {
                    char p1_body[] = {TIE};
                    char p2_body[] = {TIE};
                    set_response(&p1_res, OK, p1_body, sizeof(p1_body));
                    set_response(&p2_res, OK, p2_body, sizeof(p2_body));
                    uart_printf(CONSOLE, "[SERVER] : CLIENT %u, CLIENT %u, TIED in GAME %u \r\n", games[game].p1, games[game].p2, game);
                } else if (win == 1) {
                    char p1_body[] = {WIN};
                    char p2_body[] = {LOSE};
                    set_response(&p1_res, OK, p1_body, sizeof(p1_body));
                    set_response(&p2_res, OK, p2_body, sizeof(p2_body));
                    uart_printf(CONSOLE, "[SERVER] : CLIENT %u WON in GAME %u\r\n", games[game].p1, game);
                } else if (win == 2) {
                    char p1_body[] = {LOSE};
                    char p2_body[] = {WIN};
                    set_response(&p1_res, OK, p1_body, sizeof(p1_body));
                    set_response(&p2_res, OK, p2_body, sizeof(p2_body));
                    uart_printf(CONSOLE, "[SERVER] : CLIENT %u WON in GAME %u\r\n", games[game].p2, game);
                }

                // reset moves to allow future moves
                games[game].p1_move = -1;
                games[game].p2_move = -1;

                response = Reply(games[game].p1, (char*)&p1_res, sizeof(GameMsg));
                response = Reply(games[game].p2, (char*)&p2_res, sizeof(GameMsg));
                continue;
            }
        } else if (msg->code == QUIT) {
            #if DEBUG
                uart_dprintf(CONSOLE, "[SERVER] tid %u quitting\r\n", tid);
            #endif 
            int game = msg->body[0];
            
            // check if game is ready and both players haven't quit already
            if (games[game].p1 == -1 && games[game].p1_move != QUIT && games[game].p2 == -1 && games[game].p2_move != QUIT) {
                // TODO: respond game is not ready
                char body[] = "game not ready\0";
                GameMsg res;
                set_response(&res, ERROR, body, sizeof(body));

                uart_printf(CONSOLE, "Game %u not ready \r\n", game);
                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                continue;
            }

            if (games[game].p1 == tid) {
                games[game].p1 = -1;
                games[game].p1_move = QUIT;

                // check if other player made a move before we quit
                if (games[game].p2 != -1 && games[game].p2_move != -1) {
                    char body[] = {OPPONENT_QUIT};
                    GameMsg res;

                    set_response(&res, OK, body, sizeof(body));

                    uart_printf(CONSOLE, "[SERVER] : CLIENT %u QUIT REQUEST, ENDING GAME %u, RESPONDING TO OPPONENT CLIENT %u \r\n", tid, game, games[game].p2);
                    response = Reply(games[game].p2, (char*)&res, sizeof(GameMsg));
                }

                GameMsg res;
                set_response(&res, OK, "", 0);


                int other_player_quit = (games[game].p2 == -1 && games[game].p2_move == QUIT);

                // check if other player quit
                if (other_player_quit) {
                    // reset game
                    games[game].p1 = -1;
                    games[game].p2 = -1;
                    games[game].p1_move = -1;
                    games[game].p2_move = -1;
                }

                uart_printf(CONSOLE, "[SERVER] : CLIENT %u QUIT, GAME %u (%u/2) remaining\r\n", tid, game, !other_player_quit);
                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                continue;
            } else if (games[game].p2 == tid) {
                games[game].p2 = -1;
                games[game].p2_move = QUIT;

                // check if other player made a move before we quit
                if (games[game].p1 != -1 && games[game].p1_move != -1) {
                    char body[] = {OPPONENT_QUIT};
                    GameMsg res;

                    set_response(&res, OK, body, sizeof(body));

                    uart_printf(CONSOLE, "[SERVER] : CLIENT %u QUIT REQUEST, ENDING GAME %u, RESPONDING TO OPPONENT CLIENT %u \r\n", tid, game, games[game].p1);
                    response = Reply(games[game].p1, (char*)&res, sizeof(GameMsg));
                }

                GameMsg res;
                set_response(&res, OK, "", 0);

                int other_player_quit = (games[game].p1 == -1 && games[game].p1_move == QUIT);

                // check if other player quit
                if (other_player_quit) {
                    // reset game
                    games[game].p1 = -1;
                    games[game].p2 = -1;
                    games[game].p1_move = -1;
                    games[game].p2_move = -1;
                }

                uart_printf(CONSOLE, "[SERVER] : CLIENT %u QUIT, GAME %u (%u/2) remaining\r\n", tid, game, !other_player_quit);
                response = Reply(tid, (char*)&res, sizeof(GameMsg));
                continue;
            }
        } else {
            // TODO: error
            uart_printf(CONSOLE, "Invalid code %u from %u \r\n", msg->code, tid);    
        }
    }
}
