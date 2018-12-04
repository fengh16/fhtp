#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QDir>
#include "client.h"
#include <QThread>
#include <QDirModel>
#include <QMessageBox>
#include <QFileInfo>
#include <QCoreApplication>
#include <QObject>
#include <QInputDialog>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void on_localDirectory_expanded(const QModelIndex &index);
    void on_localDirectory_clicked(const QModelIndex &index);
    void on_localPathInput_returnPressed();
    void on_checkBox_stateChanged(int arg1);
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_passwordInput_returnPressed();
    void on_usernameInput_returnPressed();
    void on_remotePathInput_returnPressed();
    void on_remoteDirectory_clicked(const QModelIndex &index);
    void on_localDirectory_doubleClicked(const QModelIndex &index);
    void on_pushButton_2_clicked();
    void on_remoteDirectory_doubleClicked(const QModelIndex &index);
    void on_pushButton_3_clicked();
    void on_pauseButton_clicked();

private:
    Ui::MainWindow *ui;
    QIcon folder_icon, file_icon;
    QDirModel *localDirectoryModel;
    QStandardItemModel *remoteDirectoryModel;
    QMenu *menu;

//    void addSubRemoteDirectoryItem(const QString& dirName, QStandardItem* upper);
    void disconnect();
    QString getAbsLocalPath(const QModelIndex index);
    QString showAbsLocalPath(const QModelIndex index);
    void download();
    void upload();
    void startedTransfer();
    void hideProcessPart();
    void showProcessPart();

public slots:
    void receive_show(int, QString);
    void update_process(int);
    void update_process_all(int);
    void transfer_complete();
    void transfer_started();
    void stoppedTransfer();
    void pausedChecked();
    void remoteGetMenu(const QPoint &);
    void localGetMenu(const QPoint &);
    void remote_remove_file();
    void remote_remove_folder();
    void checkRemoteDirectory();
    void checkLocalDirectory();
    void create_folder_inner();
    void create_folder();
};

#endif // MAINWINDOW_H
