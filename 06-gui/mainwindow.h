#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Socket_Client.h"
// 前向声明
class QPushButton;
class QFrame;
class QWidget;
class SocketServer;  // 如果需要的话

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // 新增的私有方法声明
    void initSocketServer();
    void initUI();
    void initConnections();
    void createButtons();
    void createCameraFrame();
    void setupMainLayout(QWidget *centralWidget);

    // 成员变量声明
    QPushButton *btnPhoto = nullptr;
    QPushButton *btnRecord = nullptr;
    QPushButton *btnStream = nullptr;
    QFrame *camFrame = nullptr;
    QWidget *buttonContainer = nullptr;
    QWidget *frameContainer = nullptr;

    SocketClient *socketClient;

};

#endif // MAINWINDOW_H
