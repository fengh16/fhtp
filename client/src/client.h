#ifndef CLIENT_H
#define CLIENT_H

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

#include <QString>
#include <string.h>
#include <QDebug>
#include <QTextBrowser>
#include <QThread>
#include <QFileInfo>

#define DEBUG_ON

int getlocalip(int sockfd, int *myIP, char*myIPstr);
int read_into_sentence(int sockfd, char *sentence, int *len);
int read_data_into_sentence(int sockfd, char *sentence, int *len);
int read_data_into_sentence(int sockfd, QString& target, int *len);
int startswith(char *str, const char *starter);
int send_sentence(int connfd, char *sentence, int len);
void qstring2char(QString& a, char* tar, int len=20);

bool connect_now(QString IP, QString username, QString password, int port);

void show_info(QString info);
void show_info_red(QString info);
void show_info_gray(QString info);

extern QTextBrowser* infoOutput;
extern QString nowPath, nowFileName;
extern bool connected;
extern bool passive_mode;
extern int commands(QString& ans, QString m, QString param1 = "", QString param2 = "");
extern bool canceled;
extern int transferfd;
extern bool transferring;

class commandThread : public QThread {
    Q_OBJECT
signals:
    void send_show(int color, QString info);
    void send_process(int);
    void send_process_all(int);
    void transfer_ok();
    void transfer_error();
    void started_transfer();
    void sended_paused();

private:
    QString ans, a, b, c;
    bool started;
    int size;
    int offset;
    void show_info(QString info) {
        emit send_show(0, info);
    }
    void show_info_red(QString info) {
        emit send_show(1, info);
    }
    void show_info_gray(QString info) {
        emit send_show(2, info);
    }

public:
    commandThread(QString m, QString n, QString p, QObject* parent, int msize = -1): a(m), b(n), c(p), started(false), offset(0), size(msize) {
        connect(this, SIGNAL(send_show(int, QString)), parent, SLOT(receive_show(int, QString)));
        connect(this, SIGNAL(send_process(int)), parent, SLOT(update_process(int)));
        connect(this, SIGNAL(send_process_all(int)), parent, SLOT(update_process_all(int)));
        connect(this, SIGNAL(transfer_ok()), parent, SLOT(transfer_complete()));
        connect(this, SIGNAL(started_transfer()), parent, SLOT(transfer_started()));
        connect(this, SIGNAL(transfer_error()), parent, SLOT(stoppedTransfer()));
        connect(this, SIGNAL(sended_paused()), parent, SLOT(pausedChecked()));
    }
    int commands_files(QString& ans, QString m, QString param1 = "", QString param2 = "");
    int send_sentence(int connfd, char *sentence, int len);

protected:
    void run() override {
        if (!started) {
            if (size < 0) {
                commands_files(ans, "size", b);
                if (size < 0) {
                    show_info_red("File size read error!");
                    emit transfer_error();
                    return;
                }
            }
            emit send_process_all(size);
            emit send_process(0);
            commands_files(ans, a, b, c);
        }
        else {
            commands_files(ans, a, b, c);
        }
    }
};

#endif
