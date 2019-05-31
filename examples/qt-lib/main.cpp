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

#include <QGuiApplication>
#include <QDebug>
#include <QTimer>

#include <iostream>

#include "../../src/snoretoasts.h"
#include "../../src/snoretoastactions.h"
#include "../../src/linkhelper.h"

namespace {
const auto appId =
        QStringLiteral("Snore.DesktopToasts%1.QtLib").arg(SnoreToasts::version()).toStdWString();
}

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    qDebug() << SUCCEEDED(LinkHelper::tryCreateShortcut(
            std::filesystem::path(L"SnoreToast") / SnoreToasts::version() / L"SnoreToastQtLib",
            appId));
    QTimer::singleShot(0, &a, [&] {
        SnoreToasts toast(appId);
        //  app.setPipeName(pipe);
        //  app.setApplication(app.applicationDirPath().);
        toast.displayToast(L"Test", L"Message", L"", true);
        std::wcout << "Result" << SnoreToastActions::getActionString(toast.userAction())
                   << std::endl;
        a.quit();
    });
    return a.exec();
}

