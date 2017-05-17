/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "gameareaimpl.h"
#include "samegameimpl.h"

#include <QDir>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickView> //Not using QQmlApplicationEngine because many examples don't have a Window{}

#include <QtTest>
#include <QQuickItem>

class SameGameBenchmark: public QObject
{
    QQuickView *view;
    Q_OBJECT
public:
    SameGameBenchmark(QQuickView *view)
        : view(view)
    { }

private slots:
    static void countChildren(QObject *obj, const QString &rootName, unsigned &numObjects, unsigned &numItems, bool count = false)
    {
        if (obj->objectName() == rootName)
            count = true;
        if (count) {
            ++numObjects;
            if (qobject_cast<QQuickItem*>(obj))
                ++numItems;
        }
        foreach (auto *child, obj->children())
            countChildren(child, rootName, numObjects, numItems, count);
    }

    void initTestCase()
    {
        view->rootObject()->setProperty("state", "in-game");

        QMetaObject::invokeMethod(view->rootObject(), "startGameBench");
        unsigned numObjects = 0, numItems = 0;
        countChildren(view->rootObject(), "gameCanvas", numObjects, numItems);
        qDebug() << "GameArea has" << numObjects << "QObjects of which" << numItems << "are QQuickItems.";

    }
    void cleanupTestCase()
    {
        QMetaObject::invokeMethod(view->rootObject(), "cleanUp");
    }

    void startGameBenchmark()
    {
        QElapsedTimer t;
        quint64 totalTime = 0;
        const int numIterations = 100;
        for (int i = 0; i < numIterations; ++i)
        {
            QMetaObject::invokeMethod(view->rootObject(), "cleanUp");
            // QML object destroy() is posted to the event loop and we must process them now to avoid accumulating objects.
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);

            t.restart();
            QMetaObject::invokeMethod(view->rootObject(), "startGameBench");
            totalTime += t.nsecsElapsed();
        }
        QTest::setBenchmarkResult(double(totalTime) / numIterations, QTest::WalltimeNanoseconds);
    }
    void handleClickBenchmark()
    {
        QElapsedTimer t;
        quint64 totalTime = 0;
        const int numIterations = 100;
        for (int i = 0; i < numIterations; ++i)
        {
            QMetaObject::invokeMethod(view->rootObject(), "startGameBench");
            // QML object destroy() is posted to the event loop and we must process them now to avoid accumulating objects.
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);

            t.restart();
            QMetaObject::invokeMethod(view->rootObject(), "handleClickBench");
            totalTime += t.nsecsElapsed();
        }
        QTest::setBenchmarkResult(double(totalTime) / numIterations, QTest::WalltimeNanoseconds);
    }
};


int main(int argc, char* argv[])
{
    qmlRegisterType<SameGameImpl>("main", 1, 0, "SameGameImpl");
    qmlRegisterType<GameAreaImpl>("main", 1, 0, "GameAreaImpl");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc,argv);
    app.setOrganizationName("QtProject");
    app.setOrganizationDomain("qt-project.org");
    app.setApplicationName(QFileInfo(app.applicationFilePath()).baseName());
    QQuickView view;
    if (qgetenv("QT_QUICK_CORE_PROFILE").toInt()) {
        QSurfaceFormat f = view.format();
        f.setProfile(QSurfaceFormat::CoreProfile);
        f.setVersion(4, 4);
        view.setFormat(f);
    }
    view.connect(view.engine(), &QQmlEngine::quit, &app, &QCoreApplication::quit);
    view.setSource(QUrl("qrc:///demos/samegame/samegame.qml")); 
    if (view.status() == QQuickView::Error)
        return -1;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    if (QGuiApplication::platformName() == QLatin1String("qnx") || 
          QGuiApplication::platformName() == QLatin1String("eglfs")) {
        view.showFullScreen();
    } else {
        view.show();
    }

    // Create the benchmark object manually and execute tests before starting the application.
    SameGameBenchmark bench(&view);
    QTest::qExec(&bench, argc, argv);

    return app.exec();
}


#include "main.moc"
