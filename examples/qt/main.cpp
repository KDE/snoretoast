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

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QLocalServer *server = new QLocalServer();
    QObject::connect(server, &QLocalServer::newConnection, server, [server]() {
        auto sock = server->nextPendingConnection();
        sock->waitForReadyRead();
        const QByteArray rawData = sock->readAll();
        const QString data =
                QString::fromWCharArray(reinterpret_cast<const wchar_t *>(rawData.constData()),
                                        rawData.size() / sizeof(wchar_t));
        QMap<QString, QString> map;
        for (const auto &str : data.split(";")) {
            const auto index = str.indexOf("=");
            map[str.mid(0, index)] = str.mid(index + 1);
        }
        const auto action = map["action"];
        const auto snoreAction = SnoreToastActions::getAction(action.toStdWString());

        std::wcout << qPrintable(data) << std::endl;
        std::wcout << "Action: " << qPrintable(action) << " " << static_cast<int>(snoreAction)
                   << std::endl;

        // TODO: parse data
    });
    server->listen("foo");
    std::wcout << qPrintable(server->fullServerName()) << std::endl;

    const QString appId = "SnoreToast.Qt.Example";
    QProcess proc(&a);
    proc.start("SnoreToast.exe",
               { "-install", "SnoreToastTestQt", a.applicationFilePath(), appId });
    proc.waitForFinished();
    std::wcout << proc.exitCode() << std::endl;
    std::wcout << qPrintable(proc.readAll()) << std::endl;

    QTimer *timer = new QTimer(&a);
    a.connect(timer, &QTimer::timeout, timer, [&] {
        static int id = 0;
        if (id >= 10) {
            timer->stop();
        }
        auto proc = new QProcess(&a);
        proc->start("SnoreToast.exe",
                    { "-t", "test", "-m", "message", "-pipename", server->fullServerName(), "-w",
                      "-id", QString::number(id++), "-appId", appId, "-application",
                      a.applicationFilePath() });
        proc->connect(proc, QOverload<int>::of(&QProcess::finished), proc, [proc] {
            std::wcout << qPrintable(proc->readAll()) << std::endl;
            std::wcout << proc->exitCode() << std::endl;
        });
    });
    timer->start(1000);
    return a.exec();
}
