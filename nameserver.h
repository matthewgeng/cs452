#ifndef NAMESERVER_H
#define NAMESERVER_H

#define MAX_TASK_NAME_CHAR 20

#include "constants.h"

typedef struct NameMap{
    char mapping[MAX_NUM_TASKS][MAX_TASK_NAME_CHAR+1];
} NameMap;

int RegisterAs(const char *name);
int WhoIs(const char *name);
void nameserver();

#endif
