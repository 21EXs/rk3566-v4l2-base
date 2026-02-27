#include "mainwindow.h"
#include "Socket_Client.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QPushButton>
#include <QSpacerItem>
#include <QDebug>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    initUI();

    // 初始化按钮信号连接
    initConnections();

    socketClient = new SocketClient();
    int fd = socketClient->Socket_Client_Init();

    if (fd > 0)
    {
        std::cout << "Socket连接成功，文件描述符:" << fd << std::endl;
    }
    else
    {
        std::cout << "Socket连接失败" << std::endl;
    }

}


void MainWindow::initUI()
{
    setWindowTitle("摄像头取景-带按钮");
    setFixedSize(720, 1280);

    // 中心部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建按钮
    createButtons();

    // 创建取景框
    createCameraFrame();

    // 设置主布局
    setupMainLayout(centralWidget);
}

void MainWindow::createButtons()
{
    int buttonSize = 160;  // 按钮大小
    int buttonSpacing = 60;  // 按钮间距

    btnPhoto = new QPushButton("拍照");
    btnRecord = new QPushButton("录像");
    btnStream = new QPushButton("推流");

    // 设置按钮为正方形
    btnPhoto->setFixedSize(buttonSize, buttonSize);
    btnRecord->setFixedSize(buttonSize, buttonSize);
    btnStream->setFixedSize(buttonSize, buttonSize);

    // 设置按钮样式
    QString buttonStyle =
        "QPushButton {"
        "  border: 2px solid #333;"
        "  border-radius: 10px;"
        "  background-color: #4CAF50;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  color: white;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #3d8b40;"
        "}";

    btnPhoto->setStyleSheet(buttonStyle);
    btnRecord->setStyleSheet(buttonStyle);
    btnStream->setStyleSheet(buttonStyle);

    // 按钮水平布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(buttonSpacing);
    buttonLayout->setContentsMargins(0, 30, 0, 30);

    buttonLayout->addStretch();
    buttonLayout->addWidget(btnPhoto);
    buttonLayout->addWidget(btnRecord);
    buttonLayout->addWidget(btnStream);
    buttonLayout->addStretch();

    buttonContainer = new QWidget();
    buttonContainer->setLayout(buttonLayout);
    buttonContainer->setFixedHeight(buttonSize + 60);
}

void MainWindow::createCameraFrame()
{
    camFrame = new QFrame();
    camFrame->setFixedSize(640, 480);
    camFrame->setFrameShape(QFrame::Box);
    camFrame->setLineWidth(3);
    camFrame->setStyleSheet(
        "border: 3px solid #2196F3;"
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "stop:0 #E3F2FD, stop:1 #BBDEFB);"
        );
    camFrame->setToolTip("摄像头取景区域 640x480");

    // 取景框容器（用于居中）
    QHBoxLayout *frameContainerLayout = new QHBoxLayout();
    frameContainerLayout->setContentsMargins(0, 0, 0, 0);
    frameContainerLayout->addStretch();
    frameContainerLayout->addWidget(camFrame);
    frameContainerLayout->addStretch();

    frameContainer = new QWidget();
    frameContainer->setLayout(frameContainerLayout);
}

void MainWindow::setupMainLayout(QWidget *centralWidget)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(buttonContainer);
    mainLayout->addWidget(frameContainer, 1);
}

void MainWindow::initConnections()
{
    connect(btnPhoto, &QPushButton::clicked, this, []()
    {
        printf("拍照按钮被点击\n");
    });

    connect(btnRecord, &QPushButton::clicked, this, []()
    {
        static bool recording = false;
        recording = !recording;
        qDebug() << (recording ? "开始录像" : " 停止录像");
    });

    connect(btnStream, &QPushButton::clicked, this, []()
    {
        qDebug() << "📡 推流按钮被点击";
    });
}

MainWindow::~MainWindow() {}
