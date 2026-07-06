#include "mainwindow.h"

#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QFont>
#include <QPainter>
#include <QCursor>
#include <QScreen>
#include <QTimer>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 创建启动画面
    QPixmap splashPixmap(800, 450);
    splashPixmap.fill(QColor(30, 30, 50));  // 深蓝色背景
    
    // 绘制标题和文字
    {
        QPainter painter(&splashPixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        
        // 绘制标题
        QFont titleFont("Microsoft YaHei", 32, QFont::Bold);
        painter.setFont(titleFont);
        painter.setPen(Qt::white);
        painter.drawText(splashPixmap.rect(), Qt::AlignCenter, "混合范式脑电解码可视化平台");
        
        // 绘制副标题
        QFont subtitleFont("Microsoft YaHei", 14);
        painter.setFont(subtitleFont);
        painter.setPen(QColor(150, 150, 200));
        
        QRect subtitleRect = splashPixmap.rect();
        subtitleRect.setTop(subtitleRect.bottom() - 60);  // 距离底部60像素
        painter.drawText(subtitleRect, Qt::AlignHCenter | Qt::AlignTop, "Hybrid Paradigm EEG Decoding Visualization Platform");
    }  // QPainter 在这里自动销毁
    
    QSplashScreen splash(splashPixmap);
    
    // 设置splash窗口始终保持在最顶层
    splash.setWindowFlag(Qt::WindowStaysOnTopHint, true);
    
    splash.show();
    
    // 将splash窗口居中显示到当前屏幕的可用区域
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen) {
        const QRect availableGeometry = screen->availableGeometry();
        splash.move(availableGeometry.center() - splash.rect().center());
    }
    
    a.processEvents();  // 处理事件，确保splash显示并更新位置
    
    // 创建主窗口（此时splash保持显示）
    MainWindow w;
    
    // 使用定时器延迟显示主窗口，让用户看清splash
    QTimer::singleShot(1000, [&]() {
        // 自适应屏幕：占可用区域85%，兼顾标题栏/任务栏
        QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }
        if (screen) {
            const QSize available = screen->availableGeometry().size();
            w.resize(available.width() * 85 / 100, available.height() * 85 / 100);
            w.move(screen->availableGeometry().center() - w.rect().center());
        }
        w.show();
        splash.finish(&w);
    });

    return a.exec();
}