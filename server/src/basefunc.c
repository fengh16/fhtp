// Basic functions for network and string operations.
#include "server.h"
#include <ifaddrs.h>

int getlocalip(int sockfd, char* myIP)
{
    struct sockaddr_in addrMy;
    memset(&addrMy,0,sizeof(addrMy));
    socklen_t len = sizeof(addrMy);
    int ret = getsockname(sockfd,(struct sockaddr*)&addrMy,&len);

    if (ret != 0)
    {
        strcpy(myIP, "127.0.0.1");
        return 0;
    }

    strncpy(myIP, inet_ntoa(addrMy.sin_addr), 49);
    return 0;
}

int read_into_sentence(int connfd, char* sentence, int *len)
{
    int p = 0;
    while (1)
    {
        int n = read(connfd, sentence + p, 8191 - p);
        if (n < 0)
        {
            printf("Error read(): %s(%d)\n", strerror(errno), errno);
            return 1;
        }
        else if (n == 0)
        {
            return 2;
        }
        else
        {
            p += n;
            if (p > 1 && sentence[p - 1] == '\n' && sentence[p - 2] == '\r')
            {
                break;
            }
        }
    }
    if (p > 1)
    {
        sentence[p - 2] = 0;
        *len = p - 2;
#ifdef DEBUG_ON
        printf("Returned message: %s\n", sentence);
#endif
        return 0;
    }
    return 1;
}

int send_sentence(int connfd, char *sentence, int len)
{
    int p = 0;
    while (p < len)
    {
        int n = write(connfd, sentence + p, len - p);
        if (n < 0)
        {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return 1;
        }
        else
        {
            p += n;
        }
    }
#ifdef DEBUG_ON
    printf("Sent%d: %s\n", len, sentence);
#endif
    return 0;
}

int contains(char *str, const char *a)
{
    while (*str) {
        int i = 0;
        while (a[i] && (str[i] == a[i])) {
            i++;
        }
        if (!a[i]) {
            return 1;
        }
        str++;
    }
    return 0;
}

int startswith(char *str, const char *starter)
{
    while (1)
    {
        if (*starter == 0)
        {
            return 1;
        }
        if (*str == 0 || *str != *starter)
        {
            return 0;
        }
        str++;
        starter++;
    }
    return 0;
}

int sentence_is_command(char* sentence, char *command, int *commandlen)
{
    *commandlen = strlen(command);
    for (int i = 0; i < *commandlen; i++)
    {
        if (sentence[i] != command[i] && sentence[i] + 'A' - 'a' != command[i])
        {
            return 0;
        }
    }
    (*commandlen)++; // for <sp>
    return 1;
}

void change_port(int* port_port)
{
    port_port[1]++;
    if (port_port[1] > 255)
    {
        port_port[0]++;
        port_port[1] -= 256;
    }
    if (port_port[0] * 256 + port_port[1] >= 65535)
    {
        port_port[0] = 2000 / 256;
        port_port[1] = 2000 % 256;
    }
}
