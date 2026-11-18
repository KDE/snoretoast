#include "snoretoasts.h"
#include "toasteventhandler.h"
#include "utils.h"

#include "testing.h"

#include <wrl.h>

#include <sstream>

using namespace Testing;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Notifications;

class SnoreToastsTest
{
public:
    static HRESULT buildXml(SnoreToasts &toast, const std::wstring &title, const std::wstring &body,
                            const std::filesystem::path &image = {})
    {
        return toast.createToastXml(title, body, image);
    }

    static ComPtr<IXmlDocument> xml(const SnoreToasts &toast) { return toast.toastXml(); }
};

namespace {

class FakeActivatedArgs : public RuntimeClass<IToastActivatedEventArgs>
{
    InspectableClass(L"SnoreToast.Tests.FakeActivatedArgs", BaseTrust)

public:
    explicit FakeActivatedArgs(const std::wstring &args) : m_args(args) {}

    IFACEMETHODIMP get_Arguments(HSTRING *value) override
    {
        return WindowsCreateString(m_args.c_str(), static_cast<UINT32>(m_args.size()), value);
    }

private:
    std::wstring m_args;
};

class FakeDismissedArgs : public RuntimeClass<IToastDismissedEventArgs>
{
    InspectableClass(L"SnoreToast.Tests.FakeDismissedArgs", BaseTrust)

public:
    explicit FakeDismissedArgs(ToastDismissalReason reason) : m_reason(reason) {}

    IFACEMETHODIMP get_Reason(ToastDismissalReason *value) override
    {
        *value = m_reason;
        return S_OK;
    }

private:
    ToastDismissalReason m_reason;
};

class FakeFailedArgs : public RuntimeClass<IToastFailedEventArgs>
{
    InspectableClass(L"SnoreToast.Tests.FakeFailedArgs", BaseTrust)

public:
    IFACEMETHODIMP get_ErrorCode(HRESULT *value) override
    {
        *value = E_FAIL;
        return S_OK;
    }
};

class CaptureStream
{
public:
    explicit CaptureStream(std::wostream &stream) : m_stream(stream)
    {
        m_old = m_stream.rdbuf(m_buffer.rdbuf());
    }
    ~CaptureStream() { m_stream.rdbuf(m_old); }
    std::wstring str() const { return m_buffer.str(); }

private:
    std::wostream &m_stream;
    std::wstringstream m_buffer;
    std::wstreambuf *m_old;
};

struct Fixture
{
    explicit Fixture(const std::wstring &pipe = {}) : toast(L"SnoreToast.Tests.Unregistered")
    {
        toast.setId(uniqueName(L"test_"));
        toast.setPipeName(pipe);
        handler.Attach(new ToastEventHandler(toast));
    }

    bool signaled() const { return WaitForSingleObject(handler->event(), 0) == WAIT_OBJECT_0; }

    void activate(const std::wstring &args)
    {
        ComPtr<IInspectable> inspectable;
        Make<FakeActivatedArgs>(args).As(&inspectable);
        CHECK(handler->Invoke(nullptr, inspectable.Get()) == S_OK);
    }

    void dismiss(ToastDismissalReason reason)
    {
        auto args = Make<FakeDismissedArgs>(reason);
        CHECK(handler->Invoke(nullptr, static_cast<IToastDismissedEventArgs *>(args.Get())) == S_OK);
    }

    void fail()
    {
        CaptureStream err(std::wcerr);
        auto args = Make<FakeFailedArgs>();
        CHECK(handler->Invoke(nullptr, static_cast<IToastFailedEventArgs *>(args.Get())) == S_OK);
    }

    SnoreToasts toast;
    ComPtr<ToastEventHandler> handler;
};

std::wstring markerOnly(PipeServer &server)
{
    Utils::writePipe(server.name(), L"marker");
    return server.received();
}

std::vector<ComPtr<IXmlElement>> elements(const ComPtr<IXmlDocument> &doc, const wchar_t *tag)
{
    std::vector<ComPtr<IXmlElement>> out;
    ComPtr<IXmlNodeList> list;
    if (!doc || FAILED(doc->GetElementsByTagName(HStringReference(tag).Get(), &list))) {
        return out;
    }
    UINT32 length = 0;
    list->get_Length(&length);
    for (UINT32 i = 0; i < length; ++i) {
        ComPtr<IXmlNode> node;
        ComPtr<IXmlElement> element;
        list->Item(i, &node);
        node.As(&element);
        out.push_back(element);
    }
    return out;
}

std::wstring attribute(const ComPtr<IXmlElement> &element, const wchar_t *name)
{
    HString value;
    element->GetAttribute(HStringReference(name).Get(), value.GetAddressOf());
    return value.GetRawBuffer(nullptr);
}

std::wstring text(const ComPtr<IXmlElement> &element)
{
    ComPtr<IXmlNodeSerializer> serializer;
    element.As(&serializer);
    HString value;
    serializer->get_InnerText(value.GetAddressOf());
    return value.GetRawBuffer(nullptr);
}

std::wstring parsed(const std::wstring &data, const wchar_t *key)
{
    const auto map = Utils::splitData(data);
    const auto it = map.find(key);
    return it == map.end() ? L"<missing>" : std::wstring(it->second);
}

ComPtr<IXmlDocument> buildXml(SnoreToasts &toast, const std::filesystem::path &image = {})
{
    CHECK(SUCCEEDED(SnoreToastsTest::buildXml(toast, L"Title", L"Body", image)));
    return SnoreToastsTest::xml(toast);
}

}

TEST(handlerStartsUnsignaledAsHidden)
{
    Fixture f;
    CHECK(!f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Hidden);
}

TEST(handlerClick)
{
    PipeServer server;
    Fixture f(server.name());
    f.activate(f.toast.formatAction(Action::Clicked));
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Clicked);
    CHECK_EQ(server.received(), f.toast.formatAction(Action::Clicked));
}

TEST(handlerButtonClick)
{
    PipeServer server;
    Fixture f(server.name());
    CaptureStream out(std::wcout);
    f.activate(f.toast.formatAction(Action::ButtonClicked, { { L"button", L"Snooze" } }));
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::ButtonClicked);
    CHECK_EQ(out.str(), std::wstring(L"Snooze\n"));
    CHECK_EQ(server.received(), f.toast.formatAction(Action::ButtonClicked));
}

TEST(handlerTextEntered)
{
    PipeServer server;
    Fixture f(server.name());
    f.activate(f.toast.formatAction(Action::TextEntered));
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::TextEntered);
    CHECK_EQ(server.received(), f.toast.formatAction(Action::TextEntered));
}

TEST(handlerActivationWithoutToastArgs)
{
    Fixture f;
    CaptureStream err(std::wcerr);
    auto args = Make<FakeDismissedArgs>(ToastDismissalReason_UserCanceled);
    ComPtr<IInspectable> inspectable;
    args.As(&inspectable);
    CHECK(f.handler->Invoke(nullptr, inspectable.Get()) == S_OK);
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Hidden);
    CHECK(err.str().find(L"IToastActivatedEventArgs") != std::wstring::npos);
}

TEST(handlerActivationWithoutPipe)
{
    Fixture f;
    f.activate(f.toast.formatAction(Action::Clicked));
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Clicked);
}

TEST(handlerDismissedByUser)
{
    PipeServer server;
    Fixture f(server.name());
    f.dismiss(ToastDismissalReason_UserCanceled);
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Dismissed);
    CHECK_EQ(server.received(), f.toast.formatAction(Action::Dismissed));
}

TEST(handlerTimedOut)
{
    PipeServer server;
    Fixture f(server.name());
    f.dismiss(ToastDismissalReason_TimedOut);
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Timedout);
    CHECK_EQ(parsed(server.received(), L"action"), std::wstring(L"timedout"));
}

TEST(handlerHiddenByApplication)
{
    PipeServer server;
    Fixture f(server.name());
    f.dismiss(ToastDismissalReason_ApplicationHidden);
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Hidden);
    CHECK_EQ(parsed(server.received(), L"action"), std::wstring(L"hidden"));
}

TEST(handlerDismissWithoutPipe)
{
    Fixture f;
    f.dismiss(ToastDismissalReason_TimedOut);
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Timedout);
}

TEST(handlerFailedDoesNotWritePipe)
{
    PipeServer server;
    Fixture f(server.name());
    f.fail();
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Error);
    CHECK_EQ(markerOnly(server), std::wstring(L"marker"));
}

TEST(handlerFirstEventWins)
{
    Fixture f;
    f.dismiss(ToastDismissalReason_TimedOut);
    f.activate(f.toast.formatAction(Action::Clicked));
    f.dismiss(ToastDismissalReason_UserCanceled);
    f.fail();
    CHECK_EQ(f.handler->userAction(), Action::Timedout);
}

TEST(handlerLateEventDoesNotWritePipe)
{
    Fixture f;
    f.dismiss(ToastDismissalReason_UserCanceled);
    PipeServer server;
    f.toast.setPipeName(server.name());
    f.dismiss(ToastDismissalReason_TimedOut);
    f.activate(f.toast.formatAction(Action::Clicked));
    CHECK_EQ(markerOnly(server), std::wstring(L"marker"));
}

TEST(closeNotificationSignalsRunningToast)
{
    Fixture f;
    SnoreToasts closer(L"SnoreToast.Tests.Unregistered");
    closer.setId(f.toast.id());
    CHECK(closer.closeNotification());
    CHECK(f.signaled());
    CHECK_EQ(f.handler->userAction(), Action::Hidden);
}

TEST(closeNotificationOnlyAffectsMatchingId)
{
    Fixture f;
    SnoreToasts closer(L"SnoreToast.Tests.Unregistered");
    closer.setId(uniqueName(L"other_"));
    closer.closeNotification();
    CHECK(!f.signaled());
}

TEST(xmlDefaults)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setId(L"42");
    const auto doc = buildXml(toast);
    const auto root = elements(doc, L"toast");
    CHECK_EQ(root.size(), size_t(1));
    CHECK_EQ(attribute(root[0], L"launch"), toast.formatAction(Action::Clicked));
    CHECK_EQ(attribute(root[0], L"duration"), std::wstring(L"short"));
    CHECK(elements(doc, L"actions").empty());
    const auto audio = elements(doc, L"audio");
    CHECK_EQ(audio.size(), size_t(1));
    CHECK_EQ(attribute(audio[0], L"src"), std::wstring(L"ms-winsoundevent:Notification.Default"));
    CHECK_EQ(attribute(audio[0], L"silent"), std::wstring(L"false"));
}

TEST(xmlTitleAndBody)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    CHECK(SUCCEEDED(SnoreToastsTest::buildXml(toast, L"<b>Tom & \"Jerry\"</b>", L"line1\nline2")));
    const auto texts = elements(SnoreToastsTest::xml(toast), L"text");
    CHECK_EQ(texts.size(), size_t(2));
    CHECK_EQ(text(texts[0]), std::wstring(L"<b>Tom & \"Jerry\"</b>"));
    CHECK_EQ(text(texts[1]), std::wstring(L"line1\nline2"));
}

TEST(xmlLongDuration)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setDuration(Duration::Long);
    CHECK_EQ(attribute(elements(buildXml(toast), L"toast")[0], L"duration"), std::wstring(L"long"));
}

TEST(xmlSilentAndCustomSound)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setSilent(true);
    toast.setSound(L"Notification.Mail");
    const auto audio = elements(buildXml(toast), L"audio");
    CHECK_EQ(attribute(audio[0], L"src"), std::wstring(L"ms-winsoundevent:Notification.Mail"));
    CHECK_EQ(attribute(audio[0], L"silent"), std::wstring(L"true"));
}

TEST(xmlSoundUriIsNotPrefixedTwice)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setSound(L"ms-winsoundevent:Notification.Looping.Alarm");
    CHECK_EQ(attribute(elements(buildXml(toast), L"audio")[0], L"src"),
             std::wstring(L"ms-winsoundevent:Notification.Looping.Alarm"));
}

TEST(xmlWithoutImage)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    const auto doc = buildXml(toast);
    CHECK(elements(doc, L"image").empty());
    CHECK_EQ(attribute(elements(doc, L"binding")[0], L"template"), std::wstring(L"ToastText02"));
}

TEST(xmlImageIsAbsolute)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    const auto doc = buildXml(toast, L"logo.png");
    CHECK_EQ(attribute(elements(doc, L"binding")[0], L"template"),
             std::wstring(L"ToastImageAndText02"));
    const auto images = elements(doc, L"image");
    CHECK_EQ(images.size(), size_t(1));
    CHECK_EQ(attribute(images[0], L"src"), std::filesystem::absolute(L"logo.png").wstring());
}

TEST(xmlButtons)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setId(L"42");
    toast.setPipeName(L"\\\\.\\pipe\\foo");
    toast.setButtons(std::wstring(L"Yes;No;Maybe later"));
    const auto doc = buildXml(toast);
    CHECK_EQ(elements(doc, L"actions").size(), size_t(1));
    CHECK(elements(doc, L"input").empty());
    const auto actions = elements(doc, L"action");
    const std::vector<std::wstring> expected = { L"Yes", L"No", L"Maybe later" };
    CHECK_EQ(actions.size(), expected.size());
    for (size_t i = 0; i < std::min(actions.size(), expected.size()); ++i) {
        CHECK_EQ(attribute(actions[i], L"content"), expected[i]);
        CHECK_EQ(attribute(actions[i], L"activationType"), std::wstring(L"foreground"));
        const auto args = attribute(actions[i], L"arguments");
        CHECK_EQ(args, toast.formatAction(Action::ButtonClicked, { { L"button", expected[i] } }));
        CHECK_EQ(parsed(args, L"button"), expected[i]);
        CHECK_EQ(parsed(args, L"pipe"), std::wstring(L"\\\\.\\pipe\\foo"));
    }
}

TEST(xmlTextBox)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setId(L"42");
    toast.setTextBoxEnabled(true);
    const auto doc = buildXml(toast);
    const auto inputs = elements(doc, L"input");
    CHECK_EQ(inputs.size(), size_t(1));
    CHECK_EQ(attribute(inputs[0], L"id"), std::wstring(L"textBox"));
    CHECK_EQ(attribute(inputs[0], L"type"), std::wstring(L"text"));
    const auto actions = elements(doc, L"action");
    CHECK_EQ(actions.size(), size_t(1));
    CHECK_EQ(attribute(actions[0], L"content"), std::wstring(L"Send"));
    CHECK_EQ(attribute(actions[0], L"hint-inputId"), std::wstring(L"textBox"));
    CHECK_EQ(attribute(actions[0], L"arguments"), toast.formatAction(Action::TextEntered));
}

TEST(xmlButtonsTakePrecedenceOverTextBox)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setButtons(std::wstring(L"Ok"));
    toast.setTextBoxEnabled(true);
    const auto doc = buildXml(toast);
    CHECK(elements(doc, L"input").empty());
    const auto actions = elements(doc, L"action");
    CHECK_EQ(actions.size(), size_t(1));
    CHECK_EQ(attribute(actions[0], L"content"), std::wstring(L"Ok"));
}

TEST(xmlArgumentsRoundTripThroughHandler)
{
    PipeServer server;
    Fixture f(server.name());
    f.toast.setButtons(std::wstring(L"Reply"));
    const auto actions = elements(buildXml(f.toast), L"action");
    CHECK_EQ(actions.size(), size_t(1));
    CaptureStream out(std::wcout);
    f.activate(attribute(actions[0], L"arguments"));
    CHECK_EQ(f.handler->userAction(), Action::ButtonClicked);
    CHECK_EQ(out.str(), std::wstring(L"Reply\n"));
    server.received();
}
