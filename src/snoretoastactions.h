/*
    SnoreToast is capable to invoke Windows 8 toast notifications.
    Copyright (C) 2013-2019  Hannah von Reth <vonreth@kde.org>

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
#pragma once

#include <string>
#include <map>
#include <vector>

class SnoreToastActions
{
public:
    enum class Actions {
        Clicked,
        Hidden,
        Dismissed,
        Timedout,
        ButtonClicked,
        TextEntered,

        Error = -1
    };

    static const inline std::wstring &getActionString(const Actions &a)
    {
        return actionMap().at(a);
    }

    template<typename T>
    static inline SnoreToastActions::Actions getAction(const T &s)
    {
        for (const auto &a : actionMap()) {
            if (a.second.compare(s) == 0) {
                return a.first;
            }
        }
        return SnoreToastActions::Actions::Error;
    }

private:
    static const std::map<Actions, std::wstring> &actionMap()
    {
        static const std::map<Actions, std::wstring> _ActionStrings = {
            { Actions::Clicked, L"clicked" },
            { Actions::Hidden, L"hidden" },
            { Actions::Dismissed, L"dismissed" },
            { Actions::Timedout, L"timedout" },
            { Actions::ButtonClicked, L"buttonClicked" },
            { Actions::TextEntered, L"textEntered" }
        };
        return _ActionStrings;
    }
};
