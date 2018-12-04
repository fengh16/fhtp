#include "mainwindow.h"
#include "ui_mainwindow.h"

QTextBrowser* infoOutput;
QString nowPath;
bool connected = false;
bool passive_mode = true;
bool canceled = true;
int offset = 0;
bool transferring = false;
bool transfer_is_download = true;

QString nowFileName;

QString help_info = "左侧目录双击可以打开目录或者上传文件";
//QString remote_file = "";

class commandThread;

commandThread* commandT = nullptr;

// from http://blog.sina.com.cn/s/blog_9da24f3b0101jz1w.html
void stringToHtmlFilter(QString &str)
{
    str.replace("&","&amp;");
    str.replace(">","&gt;");
    str.replace("<","&lt;");
    str.replace("\"","&quot;");
    str.replace("\'","&#39;");
    str.replace(" ","&nbsp;");
    str.replace("\n","<br>");
    str.replace("\r","");
}

void stringToHtml(QString &str,QColor crl)
{
    QByteArray array;
    array.append(crl.red());
    array.append(crl.green());
    array.append(crl.blue());
    QString strC(array.toHex());
    str = QString("<span style=\" color:#%1;\">%2</span>").arg(strC).arg(str);
}

void show_info(QString info) {
    QColor clrR(0,0,0);
    stringToHtmlFilter(info);
    stringToHtml(info, clrR);
    infoOutput->insertHtml(info);
    infoOutput->append("");
    infoOutput->moveCursor(QTextCursor::End);
    infoOutput->repaint();
}

void show_info_red(QString info) {
    QColor clrR(255,0,0);
    stringToHtmlFilter(info);
    stringToHtml(info, clrR);
    infoOutput->insertHtml(info);
    infoOutput->append("");
    infoOutput->moveCursor(QTextCursor::End);
    infoOutput->repaint();
}

void show_info_gray(QString info) {
    QColor clrR(188,188,188);
    stringToHtmlFilter(info);
    stringToHtml(info, clrR);
    infoOutput->insertHtml(info);
    infoOutput->append("");
    infoOutput->moveCursor(QTextCursor::End);
    infoOutput->repaint();
}

void MainWindow::hideProcessPart() {
    ui->pauseButton->setEnabled(false);
}

void MainWindow::showProcessPart() {
    ui->pauseButton->setEnabled(true);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    folder_icon("folder.ico"),
    file_icon("file.ico"),
    menu(nullptr)
{
    ui->setupUi(this);

    ui->remoteDirectory->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->remoteDirectory, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(remoteGetMenu(const QPoint &)));
    ui->localDirectory->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->localDirectory, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(localGetMenu(const QPoint &)));

    infoOutput = ui->textBrowser;

    ui->remoteDirectory->setEnabled(false);
    ui->remotePathInput->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    ui->pushButton_3->setEnabled(false);
    ui->pushButton_5->setEnabled(false);
    hideProcessPart();

    ui->passwordInput->setEchoMode(QLineEdit::Password);
    remoteDirectoryModel = new QStandardItemModel(ui->remoteDirectory);
    ui->remoteDirectory->setModel(remoteDirectoryModel);
    ui->remoteDirectory->setEditTriggers(QAbstractItemView::NoEditTriggers);

    localDirectoryModel = new QDirModel(ui->localDirectory);
    ui->localDirectory->setModel(localDirectoryModel);

    QDir initPath = QDir::current();
    nowPath = initPath.absolutePath();
    auto targetIndex = localDirectoryModel->index(nowPath);
    ui->localDirectory->scrollTo(targetIndex);
    ui->localPathInput->setText(nowPath);
    ui->localPath->setText(nowPath);

    ui->localDirectory->resizeColumnToContents(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//void MainWindow::addSubRemoteDirectoryItem(const QString& dirName, QStandardItem* upper) {
//    QFileInfoList infolist = QDir(dirName).entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::NoSymLinks);
//    for (QFileInfo d : infolist) {
//        if (d.isDir()) {
//            QStandardItem* item = new QStandardItem(folder_icon, d.fileName());
//            addSubRemoteDirectoryItem(d.absoluteFilePath(), item);
//            upper->appendRow(item);
//        }
//    }
//}

//// Didn't use it now.
//void MainWindow::showRemoteDirectory() {
//    remoteDirectoryModel->clear();
//    remoteDirectoryModel->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Directories"));
//    QStandardItem* item = new QStandardItem(folder_icon, QString("/"));
//    addSubRemoteDirectoryItem("/", item);
//    remoteDirectoryModel->appendRow(item);
//}

void MainWindow::disconnect() {
    QString data;
    commands(data, "quit", "", "");
    connected = false;

    ui->remoteDirectory->setEnabled(false);
    ui->remotePathInput->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    ui->pushButton_3->setEnabled(false);
    ui->pushButton_5->setEnabled(false);
    ui->pushButton->setText("connect");
    remoteDirectoryModel->clear();
}

QString getfilename(QStringList& original) {
    int i = 8;
    QString ans = original[i];
    for (i += 1; i < original.length(); i++) {
        ans += " " + original[i];
    }
    if (ans[ans.length()-1] == '\r') {
        ans = ans.left(ans.length()-1);
    }
    return ans;
}

void MainWindow::checkRemoteDirectory() {
    QString data;
//    remote_file = "";
    commands(data, "pwd");
    ui->remotePathInput->setText(data);
    ui->remotePath->setText(data);
    data = "";
    commands(data, "ls");

    remoteDirectoryModel->clear();
    remoteDirectoryModel->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Files") << QStringLiteral("Size"));
    QStringList files = data.split("\n");

    QStandardItem* item = new QStandardItem(folder_icon, "..");
    remoteDirectoryModel->appendRow(item);
    for (QString file : files) {
        if (file == "") {
            continue;
        }
        QStringList fileData;
        for (QString m : file.split(" ")) {
            if (m != "") {
                fileData.append(m);
            }
        }

        QStandardItem* item;
        if (file[0] == 'd') {
            item = new QStandardItem(folder_icon, getfilename(fileData));
            remoteDirectoryModel->appendRow(item);
        }
        else if (file[0] == '-') {
            item = new QStandardItem(file_icon, getfilename(fileData));
            // Calculate size
            int file_size = fileData[4].toInt();
            QString m;
            if (file_size >= 1024 * 1024 * 1000) {
                m = QString::number((float)file_size / (1024 * 1024 * 1024), 'f', 1) + "GB";
            }
            else if (file_size >= 1024 * 1000) {
                m = QString::number((float)file_size / (1024 * 1024), 'f', 1) + "MB";
            }
            else if (file_size >= 1000) {
                m = QString::number((float)file_size / (1024), 'f', 1) + "KB";
            }
            else {
                m = QString::number(file_size) + "B";
            }
            QStandardItem* item_size = new QStandardItem(m);
            remoteDirectoryModel->appendRow(QList<QStandardItem*>() << item << item_size);
        }
    }
    ui->remoteDirectory->resizeColumnToContents(0);
}

void MainWindow::on_pushButton_clicked()
{
    if (!connected) {
        if (ui->usernameInput->text().isEmpty() || ui->passwordInput->text().isEmpty()) {
            QMessageBox::critical(nullptr, "Error", "请输入用户名和密码", QMessageBox::Yes, QMessageBox::Yes);
            return;
        }
        if (connect_now(ui->ipInput->text(), ui->usernameInput->text(), ui->passwordInput->text(), ui->portInput->text().toInt())) {
            connected = true;
            transferring = false;
            on_checkBox_stateChanged(1);
            ui->remoteDirectory->setEnabled(true);
            ui->remotePathInput->setEnabled(true);
            ui->pushButton_2->setEnabled(true);
            ui->pushButton_3->setEnabled(true);
            ui->pushButton_5->setEnabled(true);
            ui->pushButton->setText("disconnect");
            QString data;
            commands(data, "type");
            checkRemoteDirectory();
        }
        else {
            QMessageBox::critical(nullptr, "Error", "登录失败", QMessageBox::Yes, QMessageBox::Yes);
        }
    }
    else {
        disconnect();
    }
}

// index is the first of one row
QString MainWindow::getAbsLocalPath(const QModelIndex index) {
    // Got absolute path in pathStr
    QString pathStr = index.data().toString();
    QModelIndex p = index;
    while (p.parent().isValid()) {
        p = p.parent();
        if (p.data().toString()[0] == '/') {
            pathStr = p.data().toString() + pathStr;
        }
        else {
            pathStr = p.data().toString() + "/" + pathStr;
        }
    }
    return pathStr;
}

// index is the first of one row
QString MainWindow::showAbsLocalPath(const QModelIndex index) {
    // Got absolute path in pathStr
    QString pathStr = getAbsLocalPath(index);
    ui->localPathInput->setText(pathStr);
    return pathStr;
}

void MainWindow::on_localDirectory_expanded(const QModelIndex &index)
{
    QModelIndex m = index.siblingAtColumn(0);
    QString t = showAbsLocalPath(m);

}

void MainWindow::on_localDirectory_clicked(const QModelIndex &index)
{
    QModelIndex m = index.siblingAtColumn(0);
    QString t = showAbsLocalPath(m);

}

void MainWindow::on_localPathInput_returnPressed()
{
    QDir newPath = QDir(ui->localPathInput->text());
    if (!newPath.exists()) {
        qDebug() << "Error";
        QMessageBox::critical(nullptr, "Error", "输入的路径不存在", QMessageBox::Yes, QMessageBox::Yes);
        ui->localPathInput->setText(nowPath);
        return;
    }
    nowPath = newPath.absolutePath();
    auto targetIndex = localDirectoryModel->index(nowPath);
    ui->localDirectory->scrollTo(targetIndex);
    ui->localPathInput->setText(nowPath);
    ui->localDirectory->expand(targetIndex);
    ui->localPath->setText(nowPath);
    QDir::setCurrent(nowPath);
}

void MainWindow::on_checkBox_stateChanged(int)
{
    if (ui->checkBox->isChecked()) {
        passive_mode = true;
    }
    else {
        passive_mode = false;
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    on_localPathInput_returnPressed();
}

void MainWindow::on_pushButton_5_clicked()
{
    on_remotePathInput_returnPressed();
}

void MainWindow::on_passwordInput_returnPressed()
{
    on_pushButton_clicked();
}

void MainWindow::on_usernameInput_returnPressed()
{
    on_pushButton_clicked();
}

void MainWindow::on_remotePathInput_returnPressed()
{
    QString path = ui->remotePathInput->text();
    QString ans = "";
    commands(ans, "cd", path);
    checkRemoteDirectory();
}

void MainWindow::on_remoteDirectory_clicked(const QModelIndex &index)
{
    QModelIndex m = index.siblingAtColumn(0);
    QString pathStr = m.data().toString();
    ui->remotePathInput->setText(pathStr);
}

void MainWindow::on_localDirectory_doubleClicked(const QModelIndex &index)
{
    if (transferring) {
        return;
    }
    QModelIndex t = index.siblingAtColumn(0);
    QString path = getAbsLocalPath(t);
    QFileInfo m(path);
    if (m.isFile()) {
        if (connected) {
            on_pushButton_2_clicked();
        }
    }
    else if (m.isDir()) {
        on_pushButton_4_clicked();
    }
    else if (!m.exists()) {
        QMessageBox::critical(nullptr, "Error", "目录不存在，可能已经在外部进行了删除操作", QMessageBox::Yes, QMessageBox::Yes);
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    upload();
}

void MainWindow::on_remoteDirectory_doubleClicked(const QModelIndex &index)
{
    if (transferring) {
        return;
    }
    QModelIndex size_checker = index.siblingAtColumn(1);
    bool is_file = (size_checker.data().toString() != "");
    if (is_file) {
        on_pushButton_3_clicked();
    }
    else {
        on_pushButton_5_clicked();
    }
}

void MainWindow::on_pushButton_3_clicked()
{
    download();
}

void MainWindow::upload() {
    if (transferring) {
//        QMessageBox::critical(nullptr, "Error", "请先暂停当前传输", QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    startedTransfer();
    QString name = ui->localPathInput->text(), ans;
    hideProcessPart();
    ui->pauseButton->setText("Pause");
    canceled = false;
    offset = 0;
    if (commandT) {
        commandT->quit();
        delete commandT;
    }
    transfer_is_download = false;
    int size = 0;
    QFileInfo temp(name);
    if (temp.isFile()) {
        size = temp.size();
    }
    else {
        show_info_red("File doesn't exist!");
        emit stoppedTransfer();
        return;
    }
    QString remote_file_path = ui->remotePath->text();
    if (!remote_file_path.endsWith("/")) {
        remote_file_path += "/";
    }
    remote_file_path.append(temp.fileName());
    commandT = new commandThread("send", name, remote_file_path, this, size);
    commandT->start();
}

void MainWindow::download() {
    if (transferring) {
//        QMessageBox::critical(nullptr, "Error", "请先暂停当前传输", QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    startedTransfer();
    QString name = ui->remotePathInput->text(), ans;
    hideProcessPart();
    ui->pauseButton->setText("Pause");
    canceled = false;
    offset = 0;
    if (commandT) {
        commandT->quit();
        delete commandT;
    }
    transfer_is_download = true;
    QString local_file_path = ui->localPath->text();
    if (!local_file_path.endsWith("/")) {
        local_file_path += "/";
    }
    local_file_path.append(QFileInfo(name).fileName());
    commandT = new commandThread("recv", name, local_file_path, this);
    commandT->start();
}

void MainWindow::on_pauseButton_clicked()
{
    QString ans, temp;
    if (canceled) {
        canceled = false;
        hideProcessPart();
        startedTransfer();
        ui->pauseButton->setText("Pause");
        commandT->start();
    }
    else {
        canceled = true;
        hideProcessPart();
//        commands(ans, "abor");
        ::close(transferfd);
        transferfd = -1;
        ui->pauseButton->setText("Continue");
//        showProcessPart();
//        stoppedTransfer();
    }
}

void MainWindow::receive_show(int i, QString info) {
    if (i == 0) {
        show_info(info);
    }
    else if (i == 1) {
        show_info_red(info);
    }
    else {
        show_info_gray(info);
    }
}

void MainWindow::update_process(int m) {
    ui->progressBar->setValue(m);
    qDebug() << m;
}

void MainWindow::update_process_all(int m) {
    ui->progressBar->setRange(0, m);
}

void MainWindow::transfer_complete() {
    hideProcessPart();
    stoppedTransfer();
    if (!transfer_is_download) {
        checkRemoteDirectory();
    }
    else {
        checkLocalDirectory();
    }
}

void MainWindow::transfer_started() {
    showProcessPart();
}

void MainWindow::startedTransfer() {
    transferring = true;
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    ui->pushButton_3->setEnabled(false);
    ui->pushButton_4->setEnabled(false);
    ui->pushButton_5->setEnabled(false);
}

void MainWindow::stoppedTransfer() {
    transferring = false;
    ui->pushButton->setEnabled(true);
    ui->pushButton_2->setEnabled(true);
    ui->pushButton_3->setEnabled(true);
    ui->pushButton_4->setEnabled(true);
    ui->pushButton_5->setEnabled(true);
}

void MainWindow::pausedChecked() {
    showProcessPart();
    stoppedTransfer();
}

void MainWindow::remoteGetMenu(const QPoint & m) {
    if (transferring) {
        return;
    }

    QModelIndex index = ui->remoteDirectory->currentIndex();

    if (menu) {
        delete menu;
    }

    menu = new QMenu;

    on_remoteDirectory_clicked(index);

    QModelIndex size_checker = index.siblingAtColumn(1);
    bool is_file = (size_checker.data().toString() != "");
    if (is_file) {
        menu->addAction(QString("Download file"), this, SLOT(on_pushButton_3_clicked()));
        menu->addAction(QString("Remove file"), this, SLOT(remote_remove_file()));
        menu->addAction(QString("Create folder"), this, SLOT(create_folder()));
    }
    else {
        menu->addAction(QString("Open folder"), this, SLOT(on_pushButton_5_clicked()));
        menu->addAction(QString("Remove folder"), this, SLOT(remote_remove_folder()));
        menu->addAction(QString("Create folder inner this"), this, SLOT(create_folder_inner()));
    }
    menu->addAction(QString("Refresh"), this, SLOT(checkRemoteDirectory()));

    menu->exec(QCursor::pos());
}

void MainWindow::localGetMenu(const QPoint &) {
    if (transferring) {
        return;
    }

    QModelIndex index = ui->localDirectory->currentIndex();

    if (menu) {
        delete menu;
    }

    menu = new QMenu;

    on_localDirectory_clicked(index);

    QModelIndex t = index.siblingAtColumn(0);
    QString path = getAbsLocalPath(t);
    QFileInfo m(path);
    if (m.isFile()) {
        if (connected) {
            menu->addAction(QString("Download file"), this, SLOT(on_pushButton_2_clicked()));
        }
    }
    else if (m.isDir()) {
        menu->addAction(QString("Open folder"), this, SLOT(on_pushButton_4_clicked()));
    }
    menu->addAction(QString("Refresh"), this, SLOT(checkLocalDirectory()));

    menu->exec(QCursor::pos());
}

void MainWindow::remote_remove_folder() {
    QString data;
    commands(data, "rmd", ui->remotePathInput->text());
    checkRemoteDirectory();
}

void MainWindow::remote_remove_file() {
    QString data;
    commands(data, "delete", ui->remotePathInput->text());
    checkRemoteDirectory();
}

void MainWindow::checkLocalDirectory() {
    localDirectoryModel->refresh();
}

void MainWindow::create_folder_inner() {
    bool ok;
    QString text = QInputDialog::getText(this, tr("Input folder name to create inner this folder"), tr("Folder name:"), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        QString data;
        commands(data, "mkdir", ui->remotePathInput->text() + "/" + text);
        checkRemoteDirectory();
    }
}

void MainWindow::create_folder() {
    bool ok;
    QString text = QInputDialog::getText(this, tr("Input folder name to create"), tr("Folder name:"), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        QString data;
        commands(data, "mkdir", text);
        checkRemoteDirectory();
    }
}
