#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "client.h"

int sockfd = -1, transferfd = -1, transfer_listenfd = -1;
struct sockaddr_in addr, transfer_addr;
char sentence[8192];
char buff[8192];
int len;
int p;
char targetIP[50] = "127.0.0.1";
int tarPort = 21;
char myIPstr[100];
int myIP[4] = {127, 0, 0, 1};
int port_port[2] = {7843 / 256, 7843 % 256};

void change_port()
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

#define READ_SOCKFD_TO_SENTENCE_AND_LEN                             \
    if (read_into_sentence(sockfd, sentence, &len))                 \
    {                                                               \
        show_info_red(QString("Connection error."));                \
        return false;                                               \
    }                                                               \
    show_info_gray(QString(sentence));


char fromname[256];
char toname[256];


bool connect_now(QString IP, QString username, QString password, int port) {
    qstring2char(IP, targetIP, 46);
    tarPort = port;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        show_info_red(QString("Error socket(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
        return false;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tarPort);

    if (inet_pton(AF_INET, targetIP, &addr.sin_addr) <= 0)
    {
        show_info_red(QString("Error inet_pton(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
        return false;
    }

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        show_info_red(QString("Error connect(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
        return false;
    }

    READ_SOCKFD_TO_SENTENCE_AND_LEN

    // Cycle to log in.

    if (!startswith(sentence, "220"))
    {
        show_info_red("Not receiving 220!");
        return false;
    }

    sprintf(sentence, "USER %s\r\n", username.toStdString().c_str());
    send_sentence(sockfd, sentence, strlen(sentence));
    READ_SOCKFD_TO_SENTENCE_AND_LEN
    if (!startswith(sentence, "331"))
    {
        if (startswith(sentence, "230"))
        {
            show_info("Logged in!");
            return true;
        }
        show_info_red("Unexpected response");
        return false;
    }

    sprintf(sentence, "PASS %s\r\n", password.toStdString().c_str());
    send_sentence(sockfd, sentence, strlen(sentence));
    READ_SOCKFD_TO_SENTENCE_AND_LEN
    if (startswith(sentence, "230"))
    {
        show_info("Logged in!");
        return true;
    }
    else if (startswith(sentence, "503"))
    {
        show_info_red("Username incorrect.");
        return false;
    }
    else if (startswith(sentence, "530"))
    {
        show_info_red("Username or password incorrect.");
        return false;
    }
    else
    {
        show_info_red("Unexpected response");
        return false;
    }

    return true;
}

int commands(QString& ans, QString m, QString param1, QString param2)
{
    strncpy(sentence, m.toStdString().c_str(), 8192);
    int command_id = 0;

    if (startswith(sentence, "cdup"))
    {
        sprintf(buff, "CWD ..\r\n");
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "cd"))
    {

        sprintf(buff, "CWD %s\r\n", param1.toStdString().c_str());
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "abor"))
    {

        sprintf(buff, "ABOR\r\n");
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "image") || startswith(sentence, "binary"))
    {
        sprintf(buff, "TYPE I\r\n");
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "quit") || startswith(sentence, "exit") || startswith(sentence, "bye"))
    {
        sprintf(buff, "QUIT\r\n");
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "type"))
    {
        sprintf(buff, "TYPE I\r\n");
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "system"))
    {
        sprintf(buff, "SYST\r\n");
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "pwd"))
    {
        sprintf(buff, "PWD\r\n");
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        int i = 0, j;
        for (; i < len; i++) {
            if (sentence[i] == '"') {
                break;
            }
        }
        bool last_splash = false;
        for (j = i + 1; j < len; j++) {
            if (!last_splash && sentence[j] == '"') {
                sentence[j] = 0;
                break;
            }
            if (sentence[j] == '\\') {
                last_splash = true;
            }
            else {
                last_splash = false;
            }
        }
        if (j < len) {
            ans = QString(sentence + i + 1);
        }
        return 0;
    }
    else if (startswith(sentence, "mkdir"))
    {
        sprintf(buff, "MKD %s\r\n", param1.toStdString().c_str());
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "rmd"))
    {
        sprintf(buff, "RMD %s\r\n", param1.toStdString().c_str());
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "delete"))
    {
        sprintf(buff, "DELE %s\r\n", param1.toStdString().c_str());
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        return 0;
    }
    else if (startswith(sentence, "rename"))
    {
        strncpy(fromname, param1.toStdString().c_str(), 256);
        strncpy(toname, param2.toStdString().c_str(), 256);
        sprintf(buff, "RNFR %s\r\n", fromname);
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        if (startswith(sentence, "350"))
        {
            sprintf(buff, "RNTO %s\r\n", toname);
            send_sentence(sockfd, buff, strlen(buff));
            READ_SOCKFD_TO_SENTENCE_AND_LEN
        }
        return 0;
    }
    else if (startswith(sentence, "dir") || startswith(sentence, "ls"))
    {
        command_id = 0;
        strcpy(buff, "LIST\r\n");
    }
    else
    {
        return 0;
    }

    transfer_listenfd = -1;
    transferfd = -1;
    // choose passive or port
    if (passive_mode)
    {
        // choose PASV
        send_sentence(sockfd, "PASV\r\n", 6);
        READ_SOCKFD_TO_SENTENCE_AND_LEN

        if (!startswith(sentence, "227"))
        {
            show_info_red(QString("Unexpected ") + sentence);
            return 1;
        }

        int nums[6];
        int i = len - 1;
        int base = 1;
        int started = 0;
        for (int j = 0; j < 6; j++)
        {
            nums[j] = 0;
            started = 0;
            base = 1;
            for (; i >= 0; i--)
            {
                if (sentence[i] >= '0' && sentence[i] <= '9')
                {
                    started = 1;
                    nums[j] += base * (sentence[i] - '0');
                    base *= 10;
                }
                else if (started)
                {
                    break;
                }
            }
        }

        if ((transferfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            show_info_red(QString("Error socket(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 0;
        }
        sprintf(targetIP, "%d.%d.%d.%d", nums[5], nums[4], nums[3], nums[2]);
        tarPort = nums[1] * 256 + nums[0];

        memset(&transfer_addr, 0, sizeof(transfer_addr));
        transfer_addr.sin_family = AF_INET;
        transfer_addr.sin_port = htons(tarPort);

        if (inet_pton(AF_INET, targetIP, &transfer_addr.sin_addr) <= 0)
        {
            show_info_red(QString("Error inet_pton(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 0;
        }
        if (connect(transferfd, (struct sockaddr *)&transfer_addr, sizeof(transfer_addr)) < 0)
        {
            show_info_red(QString("Error connect(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 0;
        }
    }
    else
    {
        // choose PORT
        if ((transfer_listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            show_info_red(QString("Error socket(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 0;
        }
        while (1)
        {
            getlocalip(sockfd, myIP, myIPstr);
            sprintf(sentence, "PORT %d,%d,%d,%d,%d,%d\r\n", myIP[0], myIP[1], myIP[2], myIP[3], port_port[0], port_port[1]);

            memset(&transfer_addr, 0, sizeof(transfer_addr));
            transfer_addr.sin_family = AF_INET;
            transfer_addr.sin_port = htons(port_port[0] * 256 + port_port[1]);
            transfer_addr.sin_addr.s_addr = htonl(INADDR_ANY);

            if (bind(transfer_listenfd, (struct sockaddr *)&transfer_addr, sizeof(transfer_addr)) < 0)
            {
                show_info_red(QString("Error bind(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
                change_port();
            }
            else {
                if (listen(transfer_listenfd, 64) < 0)
                {
                    show_info_red(QString("Error listen(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
                    change_port();
                }
                else
                {
                    break;
                }
            }
        }

        send_sentence(sockfd, sentence, strlen(sentence));

        READ_SOCKFD_TO_SENTENCE_AND_LEN

        transferfd = -1;
    }

    if (!(command_id & 1))
    { // is list
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        if (!startswith(sentence, "150"))
        {
            show_info_red("Error! Can't list files.");
        }
        else
        {
            if (transferfd < 0)
            {
                if ((transferfd = accept(transfer_listenfd, NULL, NULL)) < 0)
                {
                    show_info_red(QString("Error accept(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
                    return 0;
                }
            }
            read_data_into_sentence(transferfd, ans, &len);
            READ_SOCKFD_TO_SENTENCE_AND_LEN
            if (!startswith(sentence, "226"))
            {
                show_info_red("Error! Didn't receive 226.");
            }
        }
    }

    if (transfer_listenfd >= 0)
    {
        close(transfer_listenfd);
        transfer_listenfd = -1;
    }
    if (transferfd >= 0)
    {
        close(transferfd);
        transferfd = -1;
    }
    return 0;
}

int commandThread::commands_files(QString& ans, QString m, QString param1, QString param2)
{
    if (offset > 0) {
        sprintf(buff, "REST %d\r\n", offset);
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
    }
    strncpy(sentence, m.toStdString().c_str(), 8192);
    int command_id = 0;

    if (startswith(sentence, "get"))
    {
        command_id = 1;
        sprintf(buff, "RETR %s\r\n", param1.toStdString().c_str());
        strncpy(fromname, param1.toStdString().c_str(), 256);
        strncpy(toname, param1.toStdString().c_str(), 256);
    }
    else if (startswith(sentence, "recv"))
    {
        command_id = 3;
        strncpy(fromname, param1.toStdString().c_str(), 256);
        strncpy(toname, param2.toStdString().c_str(), 256);
        sprintf(buff, "RETR %s\r\n", fromname);
    }
    else if (startswith(sentence, "put"))
    {
        command_id = 5;
        sprintf(buff, "STOR %s\r\n", param1.toStdString().c_str());
        strncpy(fromname, param1.toStdString().c_str(), 256);
        strncpy(toname, param1.toStdString().c_str(), 256);
    }
    else if (startswith(sentence, "send"))
    {
        command_id = 7;
        strncpy(fromname, param1.toStdString().c_str(), 256);
        strncpy(toname, param2.toStdString().c_str(), 256);
        sprintf(buff, "STOR %s\r\n", toname);
    }
    else if (startswith(sentence, "size")) {
        sprintf(buff, "STAT %s\r\n", param1.toStdString().c_str());
        send_sentence(sockfd, buff, strlen(buff));
        READ_SOCKFD_TO_SENTENCE_AND_LEN
        QStringList fileData;
        int index = 0;
        QStringList t = QString(sentence).split("\n");
        if (t.length() < 2) {
            return 0;
        }
        for (QString m : t[1].split(" ")) {
            if (m != "") {
                fileData.append(m);
                index ++;
                if (index == 5) {
                    break;
                }
            }
        }
        if (index == 5) {
            size = fileData[4].toInt();
        }
        return 0;
    }
    else
    {
        return 0;
    }

    transfer_listenfd = -1;
    transferfd = -1;
    // choose passive or port
    if (passive_mode)
    {
        // choose PASV
        send_sentence(sockfd, "PASV\r\n", 6);
        READ_SOCKFD_TO_SENTENCE_AND_LEN

        if (!startswith(sentence, "227"))
        {
            show_info_red(QString("Unexpected ") + sentence);
            return 1;
        }

        int nums[6];
        int i = len - 1;
        int base = 1;
        int started = 0;
        for (int j = 0; j < 6; j++)
        {
            nums[j] = 0;
            started = 0;
            base = 1;
            for (; i >= 0; i--)
            {
                if (sentence[i] >= '0' && sentence[i] <= '9')
                {
                    started = 1;
                    nums[j] += base * (sentence[i] - '0');
                    base *= 10;
                }
                else if (started)
                {
                    break;
                }
            }
        }

        if ((transferfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            show_info_red(QString("Error socket(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 0;
        }
        sprintf(targetIP, "%d.%d.%d.%d", nums[5], nums[4], nums[3], nums[2]);
        tarPort = nums[1] * 256 + nums[0];

        memset(&transfer_addr, 0, sizeof(transfer_addr));
        transfer_addr.sin_family = AF_INET;
        transfer_addr.sin_port = htons(tarPort);

        if (inet_pton(AF_INET, targetIP, &transfer_addr.sin_addr) <= 0)
        {
            show_info_red(QString("Error inet_pton(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 0;
        }
        if (::connect(transferfd, (struct sockaddr *)&transfer_addr, sizeof(transfer_addr)) < 0)
        {
            show_info_red(QString("Error connect(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 0;
        }
    }
    else
    {
        // choose PORT
        if ((transfer_listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            show_info_red(QString("Error socket(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
            return 0;
        }
        while (1)
        {
            getlocalip(sockfd, myIP, myIPstr);
            sprintf(sentence, "PORT %d,%d,%d,%d,%d,%d\r\n", myIP[0], myIP[1], myIP[2], myIP[3], port_port[0], port_port[1]);

            memset(&transfer_addr, 0, sizeof(transfer_addr));
            transfer_addr.sin_family = AF_INET;
            transfer_addr.sin_port = htons(port_port[0] * 256 + port_port[1]);
            transfer_addr.sin_addr.s_addr = htonl(INADDR_ANY);

            if (bind(transfer_listenfd, (struct sockaddr *)&transfer_addr, sizeof(transfer_addr)) < 0)
            {
                show_info_red(QString("Error bind(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
                change_port();
            }
            else {
                if (listen(transfer_listenfd, 64) < 0)
                {
                    show_info_red(QString("Error listen(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
                    change_port();
                }
                else
                {
                    break;
                }
            }
        }

        send_sentence(sockfd, sentence, strlen(sentence));

        READ_SOCKFD_TO_SENTENCE_AND_LEN

        transferfd = -1;
    }

    if (command_id & 4)
    { // STOR
        int fd;
        struct stat mstat;
        long transfer_len = 0;
        int failed = 0;
        if ((fd = open(fromname, O_RDONLY)) < 0)
        {
            failed = 1;
        }
        else
        {
            fstat(fd, &mstat);
            if (!S_ISREG(mstat.st_mode))
            {
                failed = 1;
                close(fd);
            }
            else
            {
                if (lseek(fd, offset, SEEK_SET) < 0)
                {
                    failed = 1;
                    close(fd);
                }
                else
                {
                    transfer_len = mstat.st_size - offset;
                }
            }
        }
        if (failed)
        {
            show_info_red(QString("Error! Can't read local file ") + fromname);
        }
        else
        {
            send_sentence(sockfd, buff, strlen(buff));
            READ_SOCKFD_TO_SENTENCE_AND_LEN
            if (!startswith(sentence, "150"))
            {
                show_info_red("Error! Can't store files.\n");
                close(fd);
                emit transfer_error();
            }
            else
            {
                if (transferfd < 0)
                {
                    if ((transferfd = accept(transfer_listenfd, NULL, NULL)) < 0)
                    {
                        show_info_red(QString("Error accept(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
                        close(fd);
                        failed = 3;
                    }
                }
                if (!failed)
                {
                    // send 8192 bytes a time
                    int thistimeread = 0;
                    int sent_all = offset;
                    emit started_transfer();
                    while (transfer_len > 0)
                    {
                        thistimeread = read(fd, buff, 8192);
                        qDebug() << "Thistime:" << thistimeread;
                        if (thistimeread < 0)
                        {
                            failed = 2;
                            break;
                        }
                        else if (thistimeread == 0)
                        {
                            break;
                        }
                        if (write(transferfd, buff, thistimeread) != thistimeread || canceled)
                        {
                            failed = 1;
                            if (canceled) {
                                if (transfer_listenfd >= 0)
                                {
                                    close(transfer_listenfd);
                                    transfer_listenfd = -1;
                                }
                                if (transferfd >= 0)
                                {
                                    close(transferfd);
                                    transferfd = -1;
                                }
                                failed = 4;
                            }
                            break;
                        }
                        sent_all += thistimeread;
                        offset = sent_all;
                        emit send_process(sent_all);
                        transfer_len -= thistimeread;
                    }
                }
                if (failed != 3)
                {
                    close(fd);
                    close(transferfd);
                    transferfd = -1;
                    if (failed == 1)
                    {
                        show_info_red("Error of net connection.");
                    }
                    else if (failed == 4) {
                        show_info_red("You canceled this transfer.");
                    }
                    else if (failed)
                    {
                        show_info_red("Error when reading local file.");
                    }
                    else
                    {
                        show_info("File successfully put to remote.");
                    }
                    if (transfer_listenfd >= 0)
                    {
                        close(transfer_listenfd);
                        transfer_listenfd = -1;
                    }
                    if (transferfd >= 0)
                    {
                        close(transferfd);
                        transferfd = -1;
                    }
                    READ_SOCKFD_TO_SENTENCE_AND_LEN
                    if (!startswith(sentence, "226") && !failed)
                    {
                        show_info_red("Error! Didn't receive 226.");
                    }
                    emit transfer_ok();
                    if (failed == 4) {
                        emit sended_paused();
                    }
                }
            }
        }
    }
    else
    { // RETR
        struct stat mstat;
        int fd;
        int failed = 0;
        long transfer_len;
        int mode = O_CREAT | O_WRONLY;
        if (offset == 0) {
            mode = O_CREAT | O_WRONLY | O_TRUNC;
        }
        if ((fd = open(toname, mode, 0666)) < 0)
        {
            failed = 1;
        }
        else
        {
            if (lseek(fd, offset, SEEK_SET) < 0)
            {
                failed = 1;
                close(fd);
            }
            else
            {
                fstat(fd, &mstat);
                if (!S_ISREG(mstat.st_mode))
                {
                    failed = 1;
                    close(fd);
                }
            }
        }
        if (failed)
        {
            show_info_red(QString("Error! Can't open or create local file ") + toname);
        }
        else
        {
            send_sentence(sockfd, buff, strlen(buff));
            READ_SOCKFD_TO_SENTENCE_AND_LEN
            if (!startswith(sentence, "150"))
            {
                show_info_red("Error! Can't store files.");
                close(fd);
                emit transfer_error();
            }
            else
            {
                if (transferfd < 0)
                {
                    if ((transferfd = accept(transfer_listenfd, NULL, NULL)) < 0)
                    {
                        show_info_red(QString("Error accept(): ") + strerror(errno) + "(" + QString::number(errno) + ")");
                        close(fd);
                        failed = 3;
                    }
                }
                if (!failed)
                {
                    int received_all = offset;
                    emit started_transfer();
                    while (1)
                    {
                        transfer_len = read(transferfd, buff, 8192);
                        qDebug() << "Thistime:" << transfer_len;
                        if (transfer_len < 0)
                        {
                            failed = 2;
                            break;
                        }
                        else if (transfer_len == 0)
                        {
                            break;
                        }
                        if (write(fd, buff, transfer_len) != transfer_len || canceled)
                        {
                            failed = 1;
                            if (canceled) {
                                failed = 4;
                                if (transfer_listenfd >= 0)
                                {
                                    close(transfer_listenfd);
                                    transfer_listenfd = -1;
                                }
                                if (transferfd >= 0)
                                {
                                    close(transferfd);
                                    transferfd = -1;
                                }
                            }
                            break;
                        }
                        received_all += transfer_len;
                        offset = received_all;
                        emit send_process(received_all);
                    }
                }
                if (failed != 3)
                {
                    close(fd);
                    if (failed == 2)
                    {
                        show_info_red("Error of net connection.");
                    }
                    else if (failed == 4) {
                        show_info_red("You canceled this transfer.");
                    }
                    else if (failed)
                    {
                        show_info_red("Error when writing local file.");
                    }
                    else
                    {
                        show_info("File successfully get from remote.");
                    }
                    if (transfer_listenfd >= 0)
                    {
                        close(transfer_listenfd);
                        transfer_listenfd = -1;
                    }
                    if (transferfd >= 0)
                    {
                        close(transferfd);
                        transferfd = -1;
                    }
                    READ_SOCKFD_TO_SENTENCE_AND_LEN
                    if (!startswith(sentence, "226") && !failed)
                    {
                        show_info_red("Error! Didn't receive 226.");
                    }
                    emit transfer_ok();
                    if (failed == 4) {
                        emit sended_paused();
                    }
                }
            }
        }
    }

    if (transfer_listenfd >= 0)
    {
        close(transfer_listenfd);
        transfer_listenfd = -1;
    }
    if (transferfd >= 0)
    {
        close(transferfd);
        transferfd = -1;
    }
    return 0;
}
