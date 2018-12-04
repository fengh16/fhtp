#include "client.h"
#include <ifaddrs.h>


int getlocalip(int sockfd, int* myIPnum, char* myIP)
{
    sockaddr_in addrMy;
    memset(&addrMy,0,sizeof(addrMy));
    socklen_t len = sizeof(addrMy);
    int ret = getsockname(sockfd,(sockaddr*)&addrMy,&len);

    if (ret != 0)
    {
        show_info_red("Getsockname Error!");
    }

    strncpy(myIP, inet_ntoa(addrMy.sin_addr), 49);
    sscanf(myIP, "%d.%d.%d.%d", &myIPnum[0], &myIPnum[1], &myIPnum[2], &myIPnum[3]);
    return 0;
}

int read_into_sentence(int sockfd, char *sentence, int *len)
{
    int p = 0, i;
    int check_start = 0;
    char code[4];
    while (1)
    {
        int n = read(sockfd, sentence + p, 8191 - p);
        if (n < 0)
        {
            return 1;
        }
        else if (n == 0)
        {
            break;
        }
        else
        {
            p += n;
            if (p > 1 && sentence[p - 1] == '\n' && sentence[p - 2] == '\r')
            {
                if (check_start == 0) {
                    if (sentence[3] == ' ') {
                        break;
                    }
                    else {
                        check_start = p;
                        for (i = 0; i < 3; i++) {
                            code[i] = sentence[i];
                        }
                        code[i] = 0;
                    }
                }
                int ok = 0;
                for (i = 0; i < p; i++) {
                    if (sentence[i] == '\n') {
                        if (i < p - 5 && sentence[i + 1] == code[0] && sentence[i + 2] == code[1] && sentence[i + 3] == code[2] && sentence[i + 4] == ' ') {
                            ok = 1;
                            break;
                        }
                    }
                }
                if (ok) {
                    break;
                }
            }
        }
    }
    if (p > 1 && sentence[p - 1] == '\n')
    {
        sentence[p - 2] = 0;
        *len = p - 2;
        return 0;
    }
    return 1;
}

int read_data_into_sentence(int sockfd, char *sentence, int *len)
{
    int p = 0;
    while (1)
    {
        int n = read(sockfd, sentence + p, 8191 - p);
        if (n < 0)
        {
            show_info_red(QString("Error read(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 1;
        }
        else if (n == 0)
        {
            break;
        }
        else
        {
            p += n;
        }
    }
    sentence[p] = 0;
    *len = p;
    return 0;
}

int read_data_into_sentence(int sockfd, QString& target, int *len)
{
    target.clear();
    char sentence[8192];
    int p = 0;
    *len = 0;
    while (1)
    {
        int n = read(sockfd, sentence + p, 8191 - p);
        if (n < 0)
        {
            show_info_red(QString("Error read(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 1;
        }
        else if (n == 0)
        {
            break;
        }
        else
        {
            p += n;
        }
        if (p == 8191) {
            sentence[p] = 0;
            target += QString(sentence);
            *len += p;
            p = 0;
        }
    }
    sentence[p] = 0;
    target += QString(sentence);
    *len += p;
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

int send_sentence(int connfd, char *sentence, int len)
{
    int p = 0;
    while (p < len)
    {
        int n = write(connfd, sentence + p, len - p);
        if (n < 0)
        {
            show_info_red(QString("Error write(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 1;
        }
        else
        {
            p += n;
        }
    }
#ifdef DEBUG_ON
    if (startswith(sentence, "PASS"))
    {
        show_info_gray(QString("Sent password! Length: ") + QString::number(len));
    }
    else
    {
        show_info_gray(QString("Sent ") + QString::number(len) + ": " + QString(sentence).left(len-2));
    }
#endif
    return 0;
}

int commandThread::send_sentence(int connfd, char *sentence, int len)
{
    int p = 0;
    while (p < len)
    {
        int n = write(connfd, sentence + p, len - p);
        if (n < 0)
        {
            show_info_red(QString("Error write(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 1;
        }
        else
        {
            p += n;
        }
    }
#ifdef DEBUG_ON
    if (startswith(sentence, "PASS"))
    {
        show_info_gray(QString("Sent password! Length: ") + QString::number(len));
    }
    else
    {
        show_info_gray(QString("Sent ") + QString::number(len) + ": " + QString(sentence).left(len-2));
    }
#endif
    return 0;
}

void qstring2char(QString& a, char* b, int len) {
    QByteArray c= a.toLocal8Bit();
    strncpy(b, c.data(), (size_t)len);
}
