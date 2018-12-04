#include "server.h"

int listenfd, connfd;
char USER[] = "anonymous";
const char basic_dir[256] = "/"; // if target dir isn't start with basic_dir, stop it.
char now_dir[256] = {0};

struct sockaddr_in addr, transfer_addr;
char buff[8192 + 6]; // Used for send back "%d %s\r\n"
char sentence[8192]; // Used for read&write into connfd
char myIP[100];
int port_port[2] = {8156 / 256, 8156 % 256};
int pasv_port_transferfd = -1, pasv_listenfd = -1;

// int listen_port = 6668;
int listen_port = 21;

// Shouldn't be longer than 8192 bits
int reply_text(int code, char *sentence)
{
    sprintf(buff, "%d %s\r\n", code, sentence);
    return send_sentence(connfd, buff, strlen(buff));
}

int reply_text_part(int code, char *sentence)
{
    sprintf(buff, "%d-%s\r\n", code, sentence);
    return send_sentence(connfd, buff, strlen(buff));
}

int reply_to_transfer_port(char *sentence)
{
    return send_sentence(pasv_port_transferfd, sentence, strlen(sentence));
}

// accept char* or string const.
#define INDEALING_SEND(code, str)       \
    len = strlen(str);                  \
    if (1 == reply_text((code), (str))) \
    {                                   \
        close(connfd);                  \
        return;                         \
    }

// accept char* or string const.
#define INDEALING_SEND_PART(code, str)          \
    len = strlen(str);                          \
    if (1 == reply_text_part((code), (str)))    \
    {                                           \
        close(connfd);                          \
        return;                                 \
    }

#define CHECK_LOGIN                                              \
    if (!logged_in)                                              \
    {                                                            \
        INDEALING_SEND(530, "Please login with USER and PASS."); \
        continue;                                                \
    }

#define PRE_FOR_CHANGE_PASV_PORT     \
    if (pasv_port_transferfd >= 0)   \
    {                                \
        close(pasv_port_transferfd); \
        pasv_port_transferfd = -1;   \
    }                                \
    if (pasv_listenfd >= 0)          \
    {                                \
        close(pasv_listenfd);        \
        pasv_listenfd = -1;          \
    }                                \
    transfer_way_chosen = 0;

#define CHECK_PASV_PORT                                             \
    if (transfer_way_chosen != 1 && transfer_way_chosen != 2)       \
    {                                                               \
        INDEALING_SEND(425, "Use PORT or PASV first.");             \
        continue;                                                   \
    }                                                               \
    if (pasv_listenfd >= 0 && pasv_port_transferfd == -32) {        \
        pasv_port_transferfd = accept(pasv_listenfd, NULL, NULL);   \
    }

void deal_with_commands()
{
    int logged_in = 0;
    int inputed_username = 0;
    int transfer_way_chosen = 0; // 0 for not set, 1 for port, 2 for pasv
    char username[33];
    int len;
    int commandlen;
    int ascii = 0;
    char RN_FROM[256] = {0};
    long offset = 0;
    int not_forked = 1;
    int child_pid = -1;

    INDEALING_SEND(220, "Anonymous FTP server ready.");

    while (1)
    {
        int read_ok = read_into_sentence(connfd, sentence, &len);
        if (read_ok == 1) {
            printf("Connection error!\n");
            exit(0);
        }
        else if (read_ok == 2) {
            printf("Read nothing\n");
        }
        if (!not_forked) {
            int status;
            if ( 0 == (waitpid( child_pid, &status, WNOHANG ))) {
                // Child process isn't dead
                if (sentence_is_command(sentence, "ABOR", &commandlen)) {
                    printf("Received ABOR and trying to stop...");
                    kill(child_pid, SIGKILL);
                    INDEALING_SEND(426, "Connection closed; transfer aborted.");
                    PRE_FOR_CHANGE_PASV_PORT
                }
                else {
                    printf("Received %s but don't deal with it.\n", sentence);
                }
                continue;
            }
            else {
                not_forked = 1;
                child_pid = -1;
            }
        }
        if (sentence_is_command(sentence, "USER", &commandlen))
        {
            if (logged_in)
            {
                INDEALING_SEND(530, "Can't change to another user.");
            }
            inputed_username = 1;
            strncpy(username, sentence + commandlen, 32);
            username[32] = 0;
            INDEALING_SEND(331, "Guest login ok, send your complete e-mail address as password.");
        }
        else if (sentence_is_command(sentence, "PASS", &commandlen))
        {
            if (logged_in)
            {
                INDEALING_SEND(230, "Already logged in.");
            }
            else if (!inputed_username)
            {
                INDEALING_SEND(503, "Login with USER first.");
            }
            else if (len >= commandlen && !strcmp(USER, username))
            {
                INDEALING_SEND(230, "Login successful.");
                logged_in = 1;
            }
            else
            {
                INDEALING_SEND(530, "Login incorrect.");
            }
            inputed_username = 0;
        }
        else if (sentence_is_command(sentence, "RETR", &commandlen))
        {
            CHECK_LOGIN
            CHECK_PASV_PORT
            struct stat mstat;
            int fd;
            int failed = 0;
            long transfer_len;
            if (contains(sentence + commandlen, "../") || (fd = open(sentence + commandlen, O_RDONLY)) < 0)
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
            offset = 0;
            child_pid = -1;

            if (failed)
            {
                INDEALING_SEND(451, "Failed to open file");
            }
            else
            {
                child_pid = fork();
                if (child_pid < 0) {
                    INDEALING_SEND(550, "Internal error.")
                }
                else if (child_pid > 0) {
                    not_forked = 0;
                    PRE_FOR_CHANGE_PASV_PORT
                    continue;
                }
                else {
                    // close(connfd);
                    if (!ascii)
                    {
                        sprintf(buff, "Opening BINARY mode data connection for %s (%ld bytes).", sentence + commandlen, transfer_len);
                    }
                    else
                    {
                        sprintf(buff, "Havn't done ascii mode, still use binary to write.");
                        // Havn't done.
                    }
                    strncpy(sentence, buff, 8192);
                    INDEALING_SEND(150, sentence);
                    // send 8192 bytes a time
                    int thistimeread = 0;
                    while (transfer_len > 0)
                    {
                        thistimeread = read(fd, buff, 8192);
                        if (thistimeread < 0)
                        {
                            failed = 2;
                            break;
                        }
                        else if (thistimeread == 0)
                        {
                            break;
                        }
                        unsigned int templen;
                        struct sockaddr tempm;
                        if (getpeername(pasv_port_transferfd, &tempm, &templen) < 0 && errno == ENOTCONN) {
                            failed = 1;
                            break;
                        }
                        if (write(pasv_port_transferfd, buff, thistimeread) != thistimeread)
                        {
                            failed = 1;
                            break;
                        }
                        transfer_len -= thistimeread;
                    }
                    close(fd);
                    if (failed == 2)
                    {
                        INDEALING_SEND(451, "Requested action aborted. Local file reading error.");
                    }
                    else if (failed)
                    {
                        INDEALING_SEND(426, "Connection closed; transfer aborted.");
                    }
                    else
                    {
                        INDEALING_SEND(226, "Transfer complete.");
                    }
                    PRE_FOR_CHANGE_PASV_PORT
                    exit(0);
                }
            }
            PRE_FOR_CHANGE_PASV_PORT
        }
        else if (sentence_is_command(sentence, "STOR", &commandlen) || sentence_is_command(sentence, "APPE", &commandlen))
        {
            CHECK_LOGIN
            CHECK_PASV_PORT
            struct stat mstat;
            int fd;
            int failed = 0;
            long transfer_len;
            int mode = O_CREAT | O_WRONLY | O_TRUNC;
            if (offset > 0) {
                mode = O_CREAT | O_WRONLY;
            }
            int is_appe = 0;
            if (sentence[0] == 'a' || sentence[0] == 'A') {
                is_appe = 1;
                mode = O_WRONLY | O_APPEND;
            }
            if (contains(sentence + commandlen, "../") || (fd = open(sentence + commandlen, mode)) < 0)
            {
                failed = 1;
            }
            else
            {
                if (is_appe) {
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
                else {
                    fstat(fd, &mstat);
                    if (!S_ISREG(mstat.st_mode))
                    {
                        failed = 1;
                        close(fd);
                    }
                }
            }
            offset = 0;
            child_pid = -1;

            if (failed)
            {
                INDEALING_SEND(553, "Can't create file.")
            }
            else
            {
                child_pid = fork();
                if (child_pid < 0) {
                    INDEALING_SEND(550, "Internal error.")
                }
                else if (child_pid > 0) {
                    not_forked = 0;
                    PRE_FOR_CHANGE_PASV_PORT
                    continue;
                }
                else {
                    // close(connfd);
                    if (!ascii)
                    {
                        sprintf(buff, "Opening BINARY mode data connection for %s.", sentence + commandlen);
                    }
                    else
                    {
                        sprintf(buff, "Havn't done ascii mode, still use binary to write.");
                        // Havn't done.
                    }
                    strncpy(sentence, buff, 8192);
                    INDEALING_SEND(150, sentence);

                    while (1)
                    {
                        transfer_len = read(pasv_port_transferfd, buff, 8192);
                        if (transfer_len < 0)
                        {
                            failed = 2;
                            break;
                        }
                        else if (transfer_len == 0)
                        {
                            break;
                        }
                        if (write(fd, buff, transfer_len) != transfer_len)
                        {
                            failed = 1;
                            break;
                        }
                    }

                    close(fd);
                    if (failed == 1)
                    {
                        INDEALING_SEND(451, "Requested action aborted. Local file writing error.");
                    }
                    else if (failed)
                    {
                        INDEALING_SEND(426, "Connection closed; transfer aborted.");
                    }
                    else
                    {
                        INDEALING_SEND(226, "Transfer complete.");
                    }
                    PRE_FOR_CHANGE_PASV_PORT
                    exit(0);
                }
            }
            PRE_FOR_CHANGE_PASV_PORT
        }
        else if (sentence_is_command(sentence, "QUIT", &commandlen))
        {
            INDEALING_SEND(221, "Goodbye.");
            PRE_FOR_CHANGE_PASV_PORT
            return;
        }
        else if (sentence_is_command(sentence, "ABOR", &commandlen))
        {
            CHECK_LOGIN
            INDEALING_SEND(225, "No transfer task to abort.");
            PRE_FOR_CHANGE_PASV_PORT
        }
        else if (sentence_is_command(sentence, "SYST", &commandlen))
        {
            CHECK_LOGIN
            INDEALING_SEND(215, "UNIX Type: L8");
        }
        else if (sentence_is_command(sentence, "TYPE", &commandlen))
        {
            CHECK_LOGIN
            if (!strcmp(sentence + commandlen, "I") || !strcmp(sentence + commandlen, "i"))
            {
                INDEALING_SEND(200, "Type set to I.");
                ascii = 0;
            }
            else
            {
                INDEALING_SEND(500, "Unrecognised TYPE command.");
            }
        }
        else if (sentence_is_command(sentence, "PASV", &commandlen))
        {
            CHECK_LOGIN
            PRE_FOR_CHANGE_PASV_PORT
            getlocalip(connfd, myIP);
            char pasv_str[] = "Entering Passive Mode (";
            strcpy(sentence, pasv_str);
            len = strlen(pasv_str);
            int i = 0;
            for (i = 0; myIP[i]; i++)
            {
                sentence[len + i] = myIP[i] == '.' ? ',' : myIP[i];
            }
            sentence[len + i] = 0;
            transfer_way_chosen = 2;

            if ((pasv_listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
            {
                printf("Error socket(): %s(%d)\n", strerror(errno), errno);
                INDEALING_SEND(425, "Socket error for PASV mode.");
                continue;
            }
            while (1)
            {
                sprintf(sentence + len + i, ",%d,%d).", port_port[0], port_port[1]);

                memset(&transfer_addr, 0, sizeof(transfer_addr));
                transfer_addr.sin_family = AF_INET;
                transfer_addr.sin_port = htons(port_port[0] * 256 + port_port[1]);
                transfer_addr.sin_addr.s_addr = htonl(INADDR_ANY);

                if (bind(pasv_listenfd, (struct sockaddr *)&transfer_addr, sizeof(transfer_addr)) < 0)
                {
                    printf("Error bind(): %s(%d), will use another port\n", strerror(errno), errno);
                    change_port(port_port);
                    continue;
                }

                if (listen(pasv_listenfd, 64) < 0)
                {
                    printf("Error listen(): %s(%d), will use another port\n", strerror(errno), errno);
                    change_port(port_port);
                }
                else
                {
                    break;
                }
            }
            INDEALING_SEND(227, sentence);
            pasv_port_transferfd = -32;
        }
        else if (sentence_is_command(sentence, "PORT", &commandlen))
        {
            CHECK_LOGIN
            PRE_FOR_CHANGE_PASV_PORT
            int nums[6] = {0};
            int i = len - 1;
            int base = 1;
            int started = 0;
            char targetIP[20] = "127.0.0.1";
            int tarPort;
            int failed = 0;
            for (int j = 0; j < 6; j++)
            {
                if (i < commandlen)
                {
                    failed = 1;
                    break;
                }
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
            for (int j = 0; j < 6; j++)
            {
                if (nums[j] > 255)
                {
                    failed = 1;
                    break;
                }
            }

            if (!failed)
            {
                if ((pasv_port_transferfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                {
                    printf("Error socket(): %s(%d)\n", strerror(errno), errno);
                    INDEALING_SEND(425, "Socket error for PORT mode.");
                    continue;
                }
                sprintf(targetIP, "%d.%d.%d.%d", nums[5], nums[4], nums[3], nums[2]);
                tarPort = nums[1] * 256 + nums[0];

                memset(&transfer_addr, 0, sizeof(transfer_addr));
                transfer_addr.sin_family = AF_INET;
                transfer_addr.sin_port = htons(tarPort);

                if (inet_pton(AF_INET, targetIP, &addr.sin_addr) <= 0)
                {
                    printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
                    INDEALING_SEND(500, "Illegal PORT command.");
                    continue;
                }
            }
            if (failed)
            {
                INDEALING_SEND(500, "Illegal PORT command.");
                continue;
            }
            else
            {
                transfer_way_chosen = 1;
                INDEALING_SEND(200, "Port command successful.");
            }
            if (connect(pasv_port_transferfd, (struct sockaddr *)&transfer_addr, sizeof(transfer_addr)) < 0)
            {
                printf("Error connect(): %s(%d)\n", strerror(errno), errno);
                INDEALING_SEND(500, "Illegal PORT command.");
                continue;
            }
        }
        else if (sentence_is_command(sentence, "MKD", &commandlen))
        {
            CHECK_LOGIN
            int failed = 0;
            if (mkdir(sentence + commandlen, 0777) < 0)
            {
                failed = 1;
            }
            else
            {
                if (*(sentence + commandlen) == '/')
                {
                    // It's absolute path
                    // return now path + command path even though there is . or ..
                    // just like software ftp do
                    sprintf(buff, "\"%s\" created", sentence + commandlen);
                }
                else
                {
                    getcwd(now_dir, 256);
                    sprintf(buff, "\"%s/%s\" created", now_dir, sentence + commandlen);
                }
            }
            if (failed)
            {
                INDEALING_SEND(550, "Create directory operation failed.");
            }
            else
            {
                strncpy(sentence, buff, 8192);
                INDEALING_SEND(257, sentence);
            }
        }
        else if (sentence_is_command(sentence, "CWD", &commandlen))
        {
            CHECK_LOGIN
            int failed = 0;
            getcwd(now_dir, 256);
            if (chdir(sentence + commandlen) < 0)
            {
                failed = 1;
            }
            else
            {
                getcwd(buff, 256);
                if (!startswith(buff, basic_dir))
                {
                    chdir(now_dir);
                    failed = 1;
                }
            }
            if (failed)
            {
                INDEALING_SEND(550, "Failed to change directory.");
            }
            else
            {
                INDEALING_SEND(250, "Directory successfully changed.");
            }
        }
        else if (sentence_is_command(sentence, "PWD", &commandlen))
        {
            CHECK_LOGIN
            getcwd(now_dir, 256);
            sprintf(sentence, "\"%s\" is the current directory", now_dir);
            INDEALING_SEND(257, sentence);
        }
        else if (sentence_is_command(sentence, "REST", &commandlen))
        {
            CHECK_LOGIN
            offset = 0;
            sscanf(sentence + 4, " %ld", &offset);
            sprintf(sentence, "Restart position accepted (%ld).", offset);
            INDEALING_SEND(350, sentence);
        }
        else if (sentence_is_command(sentence, "LIST", &commandlen))
        {
            CHECK_LOGIN
            CHECK_PASV_PORT
            INDEALING_SEND(150, "Here comes the directory listing.");

            DIR *dir = opendir(".");
            if (!dir)
            {
                INDEALING_SEND(450, "File system not available.");
                continue;
            }

            struct dirent *dirp;
            struct stat mstat;
            while ((dirp = readdir(dir)))
            {
                if (lstat(dirp->d_name, &mstat) < 0)
                {
                    continue;
                }
                if (dirp->d_name[0] == '.')
                {
                    continue;
                }

                sprintf(buff, "%s %s %s %s %s\r\n", statbuf_get_perms(&mstat), statbuf_get_user_info(&mstat),
                        statbuf_get_size(&mstat), statbuf_get_date(&mstat), statbuf_get_filename(&mstat, dirp->d_name));

                reply_to_transfer_port(buff);
            }
            closedir(dir);
            INDEALING_SEND(226, "Directory send OK.");
            PRE_FOR_CHANGE_PASV_PORT
        }
        else if (sentence_is_command(sentence, "STAT", &commandlen))
        {
            CHECK_LOGIN
            INDEALING_SEND_PART(213, "Status follows:");

            struct stat mstat;
            if (lstat(sentence + 5, &mstat) < 0)
            {
                continue;
            }

            sprintf(buff, "%s %s %s %s %s\r\n", statbuf_get_perms(&mstat), statbuf_get_user_info(&mstat),
                    statbuf_get_size(&mstat), statbuf_get_date(&mstat), sentence + 5);
            if (1 == send_sentence(connfd, buff, strlen(buff)))
            {
                close(connfd);
                return;
            }

            INDEALING_SEND(213, "End of status.");
        }
        else if (sentence_is_command(sentence, "RMD", &commandlen))
        {
            CHECK_LOGIN
            int failed = 0;
            if (rmdir(sentence + commandlen) < 0)
            {
                failed = 1;
            }
            if (failed)
            {
                INDEALING_SEND(550, "Remove directory operation failed.");
            }
            else
            {
                INDEALING_SEND(250, "Remove directory operation successful.");
            }
        }
        else if (sentence_is_command(sentence, "RNFR", &commandlen))
        {
            CHECK_LOGIN
            if (*(sentence + commandlen) && access(sentence + commandlen, F_OK) == 0)
            {
                strncpy(RN_FROM, sentence + commandlen, 256);
                INDEALING_SEND(350, "Ready for RNTO.");
            }
            else
            {
                INDEALING_SEND(550, "RNFR command failed.");
            }
        }
        else if (sentence_is_command(sentence, "RNTO", &commandlen))
        {
            CHECK_LOGIN
            if (RN_FROM[0])
            {
                if (rename(RN_FROM, sentence + commandlen) == 0)
                {
                    INDEALING_SEND(250, "Rename successful.");
                }
                else
                {
                    INDEALING_SEND(550, "Rename failed.");
                }
            }
            else
            {
                INDEALING_SEND(503, "RNFR required first.");
            }
            RN_FROM[0] = 0;
        }
        else
        {
            CHECK_LOGIN
            INDEALING_SEND(500, "Unknown command.");
        }
    }
}

int main(int argc, char **argv)
{
    int changed_dir = 0;
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp("-port", argv[i]))
        {
            sscanf(argv[i + 1], "%d", &listen_port);
        }
        if (!strcmp("-root", argv[i]))
        {
            chdir(argv[i + 1]);
            changed_dir = 1;
        }
    }
    if (!changed_dir)
    {
        chdir("/tmp");
    }
    // start listen to socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(listen_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    if (listen(listenfd, 10) < 0)
    {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    // end of listen to socket

    while (1)
    {
        printf("Main process start to wait for connection!\n");
        if ((connfd = accept(listenfd, NULL, NULL)) < 0)
        {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            continue;
        }

        int pid = fork();
        if (pid < 0)
        {
            printf("Error can't fork\n");
            continue;
        }
        if (pid > 0)
        {
            continue;
        }

        deal_with_commands();

        close(connfd);
        close(listenfd);
        printf("Child process exited~\n");
        return 0;
    }

    close(listenfd);
    return 0;
}
