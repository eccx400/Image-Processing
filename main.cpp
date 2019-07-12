#include <QGuiApplication>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQuick/QQuickView>
#include <QtQml>
#include <QQmlContext>
#include "imageprocessor.h"
#include <QQuickItem>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qmlRegisterType<ImageProcessor>("an.qt.ImageProcessor", 1, 0, "ImageProcessor");

    QQuickView viewer;
    viewer.setResizeMode(QQuickView::SizeRootObjectToView);
    viewer.setSource(QUrl("qrc:///main.qml"));
    viewer.show();

    return app.exec();
}
