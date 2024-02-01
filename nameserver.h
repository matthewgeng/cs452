#ifndef NAMESERVER_H
#define NAMESERVER_H

#define MAX_TASK_NAME_CHAR 20

int RegisterAs(const char *name);
int WhoIs(const char *name);
void nameserver();

#endif
