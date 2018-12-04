#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <signal.h>
#include <stdlib.h>

#include <sys/wait.h>

#define DEBUG_ON

// basefun.c
int getlocalip(int sockfd, char* myIP);
int read_into_sentence(int connfd, char* sentence, int *len);
int send_sentence(int connfd, char *sentence, int len);
int startswith(char *str, const char *starter);
int sentence_is_command(char* sentence, char *command, int *commandlen);
void change_port(int* port_port);
int contains(char *str, const char *a);

// listfile_helper.c
const char *statbuf_get_perms(struct stat *mstat);
const char *statbuf_get_date(struct stat *mstat);
const char *statbuf_get_filename(struct stat *mstat, const char *name);
const char *statbuf_get_user_info(struct stat *mstat);
const char *statbuf_get_size(struct stat *mstat);

#endif