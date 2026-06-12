#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include "Socket_Client.h"

// ActionCode 定义
#define ACTION_PHOTO        1001    // Qt → 后端：触发拍照
#define ACTION_PHOTO_RESULT 1002    // 后端 → Qt：拍照完成通知
#define ACTION_RECORD_START 2001    // Qt → 后端：开始录像（预留）
#define ACTION_RECORD_STOP  2002    // Qt → 后端：停止录像（预留）

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPhotoClicked();
    void onToastTimeout();

private:
    void initUI();
    void initSocket();
    void sendActionCode(int code);
    void showToast(const QString &text, bool success);

    // UI 组件
    QPushButton *btnPhoto;
    QLabel *toastLabel;
    QTimer *toastTimer;

    // Socket
    SocketClient *socketClient;
};

#endif // MAINWINDOW_H
