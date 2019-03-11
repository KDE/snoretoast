#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDebug>
#include <QProcess>
#include <QTimer>

#include <iostream>

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  QLocalServer *server = new QLocalServer();
  QObject::connect(server, &QLocalServer::newConnection, server, [server](){
    auto sock = server->nextPendingConnection();
    sock->waitForReadyRead();
    qDebug() << sock->bytesAvailable();
    const QByteArray rawData = sock->readAll();
    const QString data = QString::fromWCharArray(reinterpret_cast<const wchar_t*>(rawData.constData()), rawData.size() / sizeof(wchar_t));
    std::wcout << qPrintable(data) << std::endl;

    // TODO: parse data
  });
  server->listen("foo");
  std::wcout << qPrintable(server->fullServerName()) << std::endl;

  QTimer *timer = new QTimer(&a);
  a.connect(timer, &QTimer::timeout, timer, [&]{
    static int id = 0;
    if (id >= 10)
    {
      timer->stop();
    }
    auto proc = new QProcess(&a);
    proc->startDetached("SnoreToast.exe", {
                          "-t", "test",
                          "-m", "message",
                          "-pipename", server->fullServerName(),
                          "-w",
                          "-id", QString::number(id++)
                        });
    proc->connect(proc, QOverload<int>::of(&QProcess::finished), proc, [proc]{
      std::wcout << qPrintable(proc->errorString()) << std::endl;
      std::wcout << qPrintable(proc->readAll()  ) << std::endl;
      std::wcout << proc->exitCode() << std::endl;
    });
  });
  timer->start(1000);
  return a.exec();
}
