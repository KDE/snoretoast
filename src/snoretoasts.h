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

#include "stringreferencewrapper.h"

#include <sdkddkver.h>

// Windows Header Files:
#include <windows.h>
#include <sal.h>
#include <psapi.h>
#include <strsafe.h>
#include <objbase.h>
#include <shobjidl.h>
#include <functiondiscoverykeys.h>
#include <guiddef.h>
#include <shlguid.h>

#include <wrl/client.h>
#include <wrl/implements.h>
#include <windows.ui.notifications.h>

#include <filesystem>
#include <string>
#include <vector>

using namespace Microsoft::WRL;
using namespace ABI::Windows::Data::Xml::Dom;

class ToastEventHandler;


class SnoreToastActions {
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

    static SnoreToastActions::Actions getAction(const std::wstring &s);

private:
    static constexpr std::wstring_view ActionStrings[] =
	{
		L"clicked",
        L"hidden",
		L"dismissed",
		L"timedout",
		L"buttonClicked",
		L"textEntered",
	};
};

class SnoreToasts
{
public:
    static std::wstring version();

    SnoreToasts(const std::wstring &appID);
    ~SnoreToasts();

    void displayToast(const std::wstring &title, const std::wstring &body, const std::filesystem::path &image, bool wait);
	SnoreToastActions::Actions userAction();
    bool closeNotification();

    void setSound(const std::wstring &soundFile);
    void setSilent(bool silent);
    void setId(const std::wstring &id);
	std::wstring id() const;

    void setButtons(const std::wstring &buttons);
    void setTextBoxEnabled(bool textBoxEnabled);

	std::filesystem::path pipeName() const;
    void setPipeName(const std::filesystem::path &pipeName);

	std::wstring formatAction(const SnoreToastActions::Actions &action, const std::vector<std::pair<std::wstring, std::wstring> > &extraData = {}) const;

    private:
    HRESULT createToast();
    HRESULT setImage();
    HRESULT setSound();
    HRESULT setTextValues();
    HRESULT setButtons(ComPtr<IXmlNode> root);
    HRESULT setTextBox(ComPtr<IXmlNode> root);
    HRESULT setEventHandler(Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> toast);
    HRESULT setNodeValueString(const HSTRING &onputString, ABI::Windows::Data::Xml::Dom::IXmlNode *node);
    HRESULT addAttribute(const std::wstring &name, ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap *attributeMap);
    HRESULT addAttribute(const std::wstring &name, ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap *attributeMap, const std::wstring &value);
    HRESULT createNewActionButton(ComPtr<IXmlNode> actionsNode, const std::wstring &value);

    void printXML();

    std::wstring m_appID;
	std::filesystem::path m_pipeName;

    std::wstring m_title;
    std::wstring m_body;
	std::filesystem::path m_image;
    std::wstring m_sound = L"Notification.Default";
    std::wstring m_id;
    std::wstring m_buttons;
    bool m_silent;
    bool m_wait;
    bool m_textbox;

	SnoreToastActions::Actions m_action = SnoreToastActions::Actions::Clicked;

    Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> m_toastXml;
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotificationManagerStatics> m_toastManager;
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotifier> m_notifier;
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> m_notification;

    Microsoft::WRL::ComPtr<ToastEventHandler> m_eventHanlder;

};
