#ifndef TRAINSERVER_H
#define TRAINSERVER_H

typedef struct TrainServerMsg{
    char type;
    int arg;
} TrainServerMsg;

void trainserver();


#endif