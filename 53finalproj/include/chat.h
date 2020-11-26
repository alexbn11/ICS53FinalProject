#ifndef CHAT_H
#define CHAT_H

#include <sys/types.h>

ssize_t getdelimfd(char **lineptr, size_t *n, int delim, int fd);

#endif