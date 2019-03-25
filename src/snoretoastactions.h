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

#include <string_view>
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

    static constexpr std::wstring_view getActionString(const Actions &a)
    {
        return ActionStrings[static_cast<int>(a)];
    }

    static SnoreToastActions::Actions getAction(const std::wstring_view &s)
    {
        int i = 0;
        for (const auto &sv : ActionStrings) {
            if (sv.compare(s) == 0) {
                return static_cast<SnoreToastActions::Actions>(i);
            }
            ++i;
        }
        return SnoreToastActions::Actions::Error;
    }

private:
    static constexpr std::wstring_view ActionStrings[] = {
        L"clicked", L"hidden", L"dismissed", L"timedout", L"buttonClicked", L"textEntered",
    };
};
