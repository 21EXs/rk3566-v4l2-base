#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QApplication>
#include <QDebug>
#include <iostream>
#include <string>
#include <cstring>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , btnPhoto(nullptr)
    , toastLabel(nullptr)
    , toastTimer(nullptr)
    , socketClient(nullptr)
{
    initUI();
    initSocket();
}

MainWindow::~MainWindow() {}

void MainWindow::initUI()
{
    setWindowTitle("Camera");
    setFixedSize(720, 1280);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: black;");
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 中间取景区域（占满剩余空间，纯黑）
    QWidget *viewfinder = new QWidget();
    viewfinder->setStyleSheet("background-color: black;");
    mainLayout->addWidget(viewfinder, 1);

    // 底部按钮区域
    QWidget *bottomWidget = new QWidget();
    bottomWidget->setFixedHeight(120);
    bottomWidget->setStyleSheet("background-color: black;");

    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setAlignment(Qt::AlignCenter);

    // 圆形拍照按钮
    btnPhoto = new QPushButton();
    btnPhoto->setFixedSize(80, 80);
    btnPhoto->setCursor(Qt::PointingHandCursor);
    btnPhoto->setStyleSheet(
        "QPushButton {"
        "  border: 4px solid white;"
        "  border-radius: 40px;"
        "  background-color: white;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #CCCCCC;"
        "  border-color: #CCCCCC;"
        "}"
    );

    bottomLayout->addWidget(btnPhoto);
    mainLayout->addWidget(bottomWidget);

    // Toast 标签（初始隐藏）
    toastLabel = new QLabel(this);
    toastLabel->setAlignment(Qt::AlignCenter);
    toastLabel->setFixedHeight(50);
    toastLabel->setMinimumWidth(200);
    toastLabel->setStyleSheet(
        "QLabel {"
        "  color: white;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  background-color: rgba(0, 0, 0, 180);"
        "  border-radius: 10px;"
        "  padding: 8px 20px;"
        "}"
    );
    toastLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    toastLabel->hide();

    // Toast 定时器
    toastTimer = new QTimer(this);
    toastTimer->setSingleShot(true);
    connect(toastTimer, &QTimer::timeout, this, &MainWindow::onToastTimeout);

    // 拍照按钮点击
    connect(btnPhoto, &QPushButton::clicked, this, &MainWindow::onPhotoClicked);
}

void MainWindow::initSocket()
{
    socketClient = new SocketClient();
    int fd = socketClient->Socket_Client_Init();
    if (fd > 0) {
        std::cout << "Socket连接成功，文件描述符:" << fd << std::endl;
    } else {
        std::cout << "Socket连接失败" << std::endl;
    }
}

void MainWindow::sendActionCode(int code)
{
    if (!socketClient) return;

    char data[16];
    snprintf(data, sizeof(data), "%d", code);
    socketClient->SendData(data);

    // 如果是拍照命令，等待后端响应
    if (code == ACTION_PHOTO) {
        char recvBuf[64] = {0};
        ssize_t ret = socketClient->RecvData(recvBuf, sizeof(recvBuf));
        if (ret > 0) {
            int resultCode = atoi(recvBuf);
            if (resultCode == ACTION_PHOTO_RESULT) {
                // 响应格式: "1002:1" 成功, "1002:0" 失败
                char *colon = strchr(recvBuf, ':');
                if (colon) {
                    int status = atoi(colon + 1);
                    showToast(status == 1 ? QStringLiteral("拍照成功") : QStringLiteral("拍照失败"), status == 1);
                } else {
                    showToast(QStringLiteral("拍照成功"), true);
                }
            }
        } else {
            showToast(QStringLiteral("拍照失败"), false);
        }
    }
}

void MainWindow::onPhotoClicked()
{
    // 按钮缩放动画
    QPropertyAnimation *anim = new QPropertyAnimation(btnPhoto, "geometry");
    anim->setDuration(150);

    QRect originalRect = btnPhoto->geometry();
    QRect scaledRect = originalRect.adjusted(10, 10, -10, -10);

    anim->setKeyValueAt(0, originalRect);
    anim->setKeyValueAt(0.5, scaledRect);
    anim->setKeyValueAt(1, originalRect);

    anim->start(QAbstractAnimation::DeleteWhenStopped);

    // 发送拍照命令
    sendActionCode(ACTION_PHOTO);
}

void MainWindow::showToast(const QString &text, bool success)
{
    // 将 Toast 显示在屏幕中央偏下位置
    int toastX = (width() - toastLabel->width()) / 2;
    int toastY = height() / 2 + 100;
    toastLabel->setGeometry(toastX, toastY, toastLabel->width(), toastLabel->height());

    QString color = success ? "rgba(76, 175, 80, 200)" : "rgba(244, 67, 54, 200)";
    toastLabel->setStyleSheet(
        QString(
            "QLabel {"
            "  color: white;"
            "  font-size: 18px;"
            "  font-weight: bold;"
            "  background-color: %1;"
            "  border-radius: 10px;"
            "  padding: 8px 20px;"
            "}"
        ).arg(color)
    );

    toastLabel->setText(text);
    toastLabel->adjustSize();
    toastLabel->show();
    toastLabel->raise();

    // 2秒后自动隐藏
    toastTimer->start(2000);
}

void MainWindow::onToastTimeout()
{
    toastLabel->hide();
}
