/*
    SnoreToast is capable to invoke Windows 8 toast notifications.
    Copyright (C) 2019  Hannah von Reth <vonreth@kde.org>

    SnoreToast is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SnoreToast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SnoreToast.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDebug>
#include <QProcess>
#include <QTimer>

#include <iostream>

#include <snoretoastactions.h>

namespace {
constexpr int NOTIFICATION_COUNT = 10;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QLocalServer *server = new QLocalServer();
    QObject::connect(server, &QLocalServer::newConnection, server, [server]() {
        auto sock = server->nextPendingConnection();
        sock->waitForReadyRead();
        const QByteArray rawData = sock->readAll();
        sock->deleteLater();

        const QString data =
                QString::fromWCharArray(reinterpret_cast<const wchar_t *>(rawData.constData()),
                                        rawData.size() / static_cast<int>(sizeof(wchar_t)));
        qDebug() << data;
        QMap<QString, QString> map;
        for (const auto &str : data.split(QLatin1Char(';'))) {
            const auto index = str.indexOf(QLatin1Char('='));
            if (index > 0) {
                map[str.mid(0, index)] = str.mid(index + 1);
            }
        }
        const QString action = map["action"];

        const auto snoreAction = SnoreToastActions::getAction(action.toStdWString());

        std::wcout << qPrintable(data) << std::endl;
        std::wcout << "Action: " << qPrintable(action) << " " << static_cast<int>(snoreAction)
                   << std::endl;

        switch (snoreAction) {
        case SnoreToastActions::Actions::Clicked:
            break;
        case SnoreToastActions::Actions::Hidden:
            break;
        case SnoreToastActions::Actions::Dismissed:
            break;
        case SnoreToastActions::Actions::Timedout:
            break;
        case SnoreToastActions::Actions::ButtonClicked:
            break;
        case SnoreToastActions::Actions::TextEntered:
            break;
        case SnoreToastActions::Actions::Error:
            break;
        }
    });
    server->listen("foo");
    std::wcout << qPrintable(server->fullServerName()) << std::endl;

    const QString appId = "SnoreToast.Qt.Example";
    QProcess proc(&app);
    proc.start("SnoreToast.exe",
               { "-install", "SnoreToastTestQt", app.applicationFilePath(), appId });
    proc.waitForFinished();
    std::wcout << proc.exitCode() << std::endl;
    std::wcout << qPrintable(proc.readAllStandardOutput()) << std::endl;
    std::wcout << qPrintable(proc.readAllStandardError()) << std::endl;

    QTimer *timer = new QTimer(&app);
    app.connect(timer, &QTimer::timeout, timer, [&] {
        static int id = 0;
        if (id >= NOTIFICATION_COUNT) {
            timer->stop();
        }
        auto proc = new QProcess(&app);
        proc->start("SnoreToast.exe",
                    { "-t", "test", "-m", "message", "-pipename", server->fullServerName(), "-id",
                      QString::number(id++), "-appId", appId, "-application",
                      app.applicationFilePath() });

        int currentId = id;
        proc->connect(proc, QOverload<int>::of(&QProcess::finished), proc, [proc, currentId, &app] {
            std::wcout << qPrintable(proc->readAllStandardOutput()) << std::endl;
            std::wcout << qPrintable(proc->readAllStandardError()) << std::endl;
            if (proc->error() != QProcess::UnknownError) {
                std::wcout << qPrintable(proc->errorString()) << std::endl;
            }
            std::wcout << qPrintable(proc->program()) << L" Notification: " << currentId
                       << L" exited with: " << proc->exitCode() << std::endl;
            if (currentId >= NOTIFICATION_COUNT) {
                app.quit();
            }
            proc->deleteLater();
        });
    });
    timer->start(1000);

    return app.exec();
}
