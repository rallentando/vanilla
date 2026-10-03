#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include "edgewebview_p.hpp"
#include "edgehostwindow.hpp"

#include <QAction>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QCursor>
#include <QClipboard>
#include <QGuiApplication>
#include <QStyleHints>
#include <QBackingStore>
#include <QPainter>
#include <QPlatformSurfaceEvent>
#include <cstdio>

#include "page.hpp"

bool EdgeInspectorTraceOn();
#include "webelement.hpp"
#include "treebank.hpp"
#include "application.hpp"
#include "theme.hpp"
#include "devicescale.hpp"
#include "actionmapper.hpp"
#include "extensionhost.hpp"

bool EdgeWebView::CanCompleteAction(const QString &action){
    static QSet<QString> completable;
    if(completable.isEmpty()){
#define VANILLA_EDGE_COMPLETABLE(ACTION) completable.insert(QStringLiteral(#ACTION));
        FOR_EACH_APPLICATION_EVENTS(VANILLA_EDGE_COMPLETABLE)
        FOR_EACH_VIEW_EVENTS(VANILLA_EDGE_COMPLETABLE)
#undef VANILLA_EDGE_COMPLETABLE

        completable
            << QStringLiteral("Back")
            << QStringLiteral("Forward")
            << QStringLiteral("UpDirectory")
            << QStringLiteral("Load")
            << QStringLiteral("Reload")
            << QStringLiteral("Stop")
            << QStringLiteral("ZoomIn")
            << QStringLiteral("ZoomOut")
            << QStringLiteral("Copy")
            << QStringLiteral("Cut")
            << QStringLiteral("Paste")
            << QStringLiteral("Undo")
            << QStringLiteral("Redo")
            << QStringLiteral("SelectAll")
            << QStringLiteral("Unselect")
            << QStringLiteral("StopAndUnselect")
            << QStringLiteral("Save")
            << QStringLiteral("ToggleMediaControls")
            << QStringLiteral("ToggleMediaLoop")
            << QStringLiteral("ToggleMediaPlayPause")
            << QStringLiteral("ToggleMediaMute")
            << QStringLiteral("Print")
            << QStringLiteral("InspectElement")
            << QStringLiteral("ExitFullScreen")
            << QStringLiteral("ReloadAndBypassCache")
            << QStringLiteral("ViewSource");

#define VANILLA_EDGE_EDIT_COMPLETABLE(ACTION)                           \
        completable.insert(QStringLiteral(#ACTION));
        FOR_EACH_EDIT_EVENTS(VANILLA_EDGE_EDIT_COMPLETABLE)
        FOR_EACH_KEYBOARD_EVENTS(VANILLA_EDGE_EDIT_COMPLETABLE)
#undef VANILLA_EDGE_EDIT_COMPLETABLE
    }
    return completable.contains(action);
}

static bool IsMovementAction(const QString &action){
    static QSet<QString> movement;
    if(movement.isEmpty()){
#define VANILLA_EDGE_MOVEMENT(ACTION) movement.insert(QStringLiteral(#ACTION));
        FOR_EACH_KEYBOARD_EVENTS(VANILLA_EDGE_MOVEMENT)
#undef VANILLA_EDGE_MOVEMENT
    }
    return movement.contains(action);
}

void EdgeWebView::RegisterInputBridge(){
    if(!m_Impl->m_WebView){
        if(m_Impl->m_State.ScriptFailed() == EdgeControllerState::Effect::ApplyPending)
            ApplyPendingState();
        return;
    }

    QList<int> plain;
    QList<int> shifted;
    for(int code = 0x30; code <= 0x5A; code++){
        if(code > 0x39 && code < 0x41) continue;

        const int key = Application::JsKeyToQtKey(code);
        QKeyEvent plainEvent(QEvent::KeyPress, key, Qt::NoModifier);
        QKeyEvent shiftEvent(QEvent::KeyPress, key, Qt::ShiftModifier);

        const QString plainAction = KeyAction(Application::MakeKeySequence(&plainEvent));
        const QString shiftAction = KeyAction(Application::MakeKeySequence(&shiftEvent));

        if(!plainAction.isEmpty() && CanCompleteAction(plainAction)) plain << code;
        if(!shiftAction.isEmpty() && CanCompleteAction(shiftAction)) shifted << code;
    }
    {
        QKeyEvent escapeEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        const QString escapeAction = KeyAction(Application::MakeKeySequence(&escapeEvent));
        if(!escapeAction.isEmpty() && CanCompleteAction(escapeAction)) plain << 0x1B;
    }

    const QString script =
        EdgeInputBridgeJsCode(plain, shifted) + EdgeScrollReportJsCode()
        + EdgeHideWebViewJsCode();

    const HRESULT hr = m_Impl->m_WebView->AddScriptToExecuteOnDocumentCreated
        (reinterpret_cast<PCWSTR>(script.utf16()),
         Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>
         ([this](HRESULT result, PCWSTR) -> HRESULT {
             const EdgeControllerState::Effect effect = SUCCEEDED(result)
                 ? m_Impl->m_State.ScriptRegistered()
                 : m_Impl->m_State.ScriptFailed();

             if(effect != EdgeControllerState::Effect::ApplyPending) return S_OK;

             if(FAILED(result))
                 emit statusBarMessage
                     (tr("The page cannot report keys to Vanilla in this view."));

             ApplyPendingState();
             return S_OK;
         }).Get());

    if(FAILED(hr)){
        QTimer::singleShot(0, this, [this](){
            if(m_Impl->m_State.ScriptFailed() != EdgeControllerState::Effect::ApplyPending)
                return;
            emit statusBarMessage
                (tr("The page cannot report keys to Vanilla in this view."));
            ApplyPendingState();
        });
    }
}

void EdgeWebView::HandleWebMessage(const QString &json, const QString &source){
    if(!m_Impl->m_Document.IsDocumentActive()) return;

    if(!EdgeMessage::IsFromDocument(source, m_Impl->m_Url) &&
       !m_Impl->m_StringDocument.IsOwn(m_Impl->m_Url, QUrl(source)))
        return;

    const EdgeMessage message = EdgeMessage::Parse(json, Application::EventKey());

    if(message.GetKind() == EdgeMessage::Kind::Scroll){
        m_Impl->m_PageScroll = message.GetScrollPosition();
        m_Impl->m_PageContents = message.GetContentsSize();
        m_Impl->m_PageViewport = message.GetViewportSize();
        emit PageGeometryChanged();
        m_Impl->m_Scroll = EdgeScrollRatio(m_Impl->m_PageScroll, m_Impl->m_PageContents, m_Impl->m_PageViewport);
        emit ScrollChanged(m_Impl->m_Scroll);
        return;
    }

    if(!m_Impl->m_HasFocus || !visible()) return;
    if(!m_TreeBank || m_TreeBank->GetCurrentView().get() != this) return;

    switch(message.GetKind()){
    case EdgeMessage::Kind::Key: {
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        if(message.GetShift()) modifiers |= Qt::ShiftModifier;

        QKeyEvent event(QEvent::KeyPress,
                        Application::JsKeyToQtKey(message.GetCode()),
                        modifiers);

        const QKeySequence sequence = Application::MakeKeySequence(&event);
        if(sequence.isEmpty()) return;

        const QString action = m_KeyMap.value(sequence);
        if(action.isEmpty() || !CanCompleteAction(action)) return;

        View::TriggerAction(action);
        break;
    }
    case EdgeMessage::Kind::Print:
        QTimer::singleShot(0, this, [this](){ if(!IsGoing()) HandlePagePrintRequest();});
        break;
    case EdgeMessage::Kind::PreventScrollRestoration:
        break;
    case EdgeMessage::Kind::Invalid:
        break;
    }
}

bool EdgeWebView::HandleAcceleratorKey(int virtualKey, bool down, bool repeat){

    if(virtualKey == VK_PROCESSKEY || virtualKey == VK_PACKET) return false;

    if(!down){
        return m_Impl->m_HandledKeys.remove(virtualKey);
    }

    if(repeat) return m_Impl->m_HandledKeys.contains(virtualKey);

    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    if(GetKeyState(VK_SHIFT)   & 0x8000) modifiers |= Qt::ShiftModifier;
    if(GetKeyState(VK_CONTROL) & 0x8000) modifiers |= Qt::ControlModifier;
    if(GetKeyState(VK_MENU)    & 0x8000) modifiers |= Qt::AltModifier;
    if((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
        modifiers |= Qt::MetaModifier;

    if(virtualKey == VK_ESCAPE && modifiers == Qt::NoModifier) return false;

    QKeyEvent event(QEvent::KeyPress,
                    Application::JsKeyToQtKey(virtualKey),
                    modifiers);

    const QKeySequence sequence = Application::MakeKeySequence(&event);
    if(sequence.isEmpty()) return false;

    const QString action = KeyAction(sequence);
    if(action.isEmpty() || !CanCompleteAction(action)) return false;

    if(IsMovementAction(action)) return false;

    if(!View::TriggerAction(action)) return false;

    m_Impl->m_HandledKeys.insert(virtualKey);
    return true;
}

void EdgeWebView::DisplayContextMenuFor(const ContextTarget &target){
    const int generation = target.m_Generation;
    if(target.m_Superseded) return;
    if(target.m_Sequence != m_Impl->m_MenuSequence) return;
    if(generation && !(m_Impl->m_ContextMenu.HasOutstanding() &&
                       m_Impl->m_ContextMenu.Generation() == generation))
        return;
    if(m_Impl->m_State.IsRetired()) return;
    if(m_Impl->m_ContextMenu.IsFinishing() || m_Impl->m_MenuHandlerDepth > 0){
        m_Impl->m_MenuRetry = target;
        if(!m_Impl->m_MenuRetryArmed){
            m_Impl->m_MenuRetryArmed = true;
            QPointer<EdgeWebView> later(this);
            QTimer::singleShot(16, this, [later](){
                if(!later) return;
                later->m_Impl->m_MenuRetryArmed = false;
                const ContextTarget again = later->m_Impl->m_MenuRetry;
                later->m_Impl->m_MenuRetry = ContextTarget();
                later->DisplayContextMenuFor(again);
            });
        }
        return;
    }
    if(!page() || !m_TreeBank){
        if(generation) CompleteContextMenu(generation, -1);
        return;
    }

    std::shared_ptr<JsWebElement> element = std::make_shared<JsWebElement>();
    *element = JsWebElement(this, target.m_Position, target.m_LinkUrl,
                            target.m_SourceUrl, target.m_Editable);

    m_SelectedText = target.m_SelectedText;

    std::shared_ptr<int> chosen = std::make_shared<int>(-1);
    std::function<void(QMenu*)> extra;
    QPointer<EdgeWebView> alive(this);
    if(!target.m_ExtensionItems.isEmpty()){
        const QList<EdgeMenuItem> items = target.m_ExtensionItems;
        extra = [items, chosen, alive](QMenu *menu){
            if(alive) alive->m_Impl->m_ContextMenuWidget = menu;
            AddEdgeMenuItems(menu, items, [chosen](int id){ *chosen = id;});
        };
    }

    page()->DisplayContextMenu(m_TreeBank, element,
                               target.m_Position,
                               mapToGlobal(target.m_Position),
                               static_cast<Page::MediaType>(target.m_MediaType),
                               extra);
    if(!alive) return;

    if(generation){
        const int id = *chosen;
        QTimer::singleShot(0, alive.data(), [alive, generation, id](){
            if(alive) alive->CompleteContextMenu(generation, id);
        });
    }
}

void EdgeWebView::CompleteContextMenu(int generation, int commandId){
    if(EdgeInspectorTraceOn()){
        fprintf(stderr, "edge-menu: complete generation=%d command=%d held=%d outstanding=%d\n",
                generation, commandId, m_Impl->m_ContextMenu.Generation(),
                m_Impl->m_ContextMenu.HasOutstanding() ? 1 : 0);
        fflush(stderr);
    }
    if(!m_Impl->m_ContextMenu.Complete(generation)) return;
    FinishContextMenu(commandId);
}

void EdgeWebView::FinishContextMenu(int commandId){

    ComPtr<ICoreWebView2ContextMenuRequestedEventArgs> args = m_Impl->m_ContextMenuArgs;
    ComPtr<ICoreWebView2Deferral> deferral = m_Impl->m_ContextMenuDeferral;
    m_Impl->m_ContextMenuArgs.Reset();
    m_Impl->m_ContextMenuDeferral.Reset();
    CountedComplete(args.Get(), deferral.Get(), commandId);
}

void EdgeWebView::CountedComplete(ICoreWebView2ContextMenuRequestedEventArgs *args,
                                  ICoreWebView2Deferral *deferral, int commandId){
    QPointer<EdgeWebView> alive(this);
    m_Impl->m_ContextMenu.BeginFinish();
    if(args && commandId >= 0){
        ViewNode *vn = GetViewNode();
        const qint64 tab = vn ? static_cast<qint64>(vn->GetSerial()) : 0;
        const QPointer<ExtensionController> extensions = m_Impl->m_Extensions;
        if(SUCCEEDED(args->put_SelectedCommandId(commandId))){
            QString page;
            ComPtr<ICoreWebView2ContextMenuTarget> target;
            LPWSTR uri = nullptr;
            if(SUCCEEDED(args->get_ContextMenuTarget(&target)) && target &&
               SUCCEEDED(target->get_PageUri(&uri)) && uri){
                page = QString::fromWCharArray(uri);
                CoTaskMemFree(uri);
            }
            if(extensions) ExtensionHost::MenuPicked(extensions.data(), tab, page);
        }
    }
    if(deferral) deferral->Complete();
    if(!alive) return;
    if(m_Impl->m_ContextMenu.EndFinish()){
        Retire();
    }
}

void EdgeWebView::setFocus(Qt::FocusReason reason){
    QWidget::setFocus(reason);
    if(m_Impl->m_Controller && !m_Impl->m_State.IsRetired())
        m_Impl->m_Controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
}

void EdgeWebView::TakeKeyboardBack(){
    const bool inspector = InspectorHoldsKeyboard();
    if(EdgeInspectorTraceOn()){
        fprintf(stderr, "edge-keyboard: TakeKeyboardBack backend=%d inspector=%d\n",
                m_Impl->m_HasFocus ? 1 : 0, inspector ? 1 : 0);
        fflush(stderr);
    }
    if(!m_Impl->m_HasFocus && !inspector) return;
    if(QWidget *top = window())
        ::SetFocus(reinterpret_cast<HWND>(top->winId()));
}

bool EdgeWebView::InspectorHoldsKeyboard() const {
    const HWND hwnd = reinterpret_cast<HWND>(m_Impl->m_InspectorWinId);
    if(!hwnd || !::IsWindow(hwnd)) return false;
    const HWND focus = ::GetFocus();
    return focus && (focus == hwnd || ::IsChild(hwnd, focus));
}

bool EdgeWebView::eventFilter(QObject *watched, QEvent *ev){

    if(watched == m_Impl->m_HostWindow && ev && ev->type() == QEvent::PlatformSurface && EdgeInspectorTraceOn()){
        const auto kind = static_cast<QPlatformSurfaceEvent*>(ev)->surfaceEventType();
        fprintf(stderr, "edge-inspector: host %s hwnd=%p\n",
                kind == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed ? "SurfaceAboutToBeDestroyed" : "SurfaceCreated",
                reinterpret_cast<void*>(m_Impl->m_HostWindow->winId()));
        fflush(stderr);
    }
    if(watched == m_Impl->m_HostWindow && ev && ev->type() == QEvent::Expose)
        PaintHostWindow();
    if(watched == m_Impl->m_HostWindow && ev && ev->type() == QEvent::MouseButtonPress)
        if(EdgeHostWindow *host = qobject_cast<EdgeHostWindow*>(m_Impl->m_HostWindow))
            host->TakeQtFocus();
    if(m_Impl && m_Impl->m_Composition && ev && ForwardMouseEvent(ev)) return true;
    return QWidget::eventFilter(watched, ev);
}

void EdgeWebView::PaintHostWindow(){
    if(!m_Impl->m_HostWindow || m_Impl->m_Controller ||
       !m_Impl->m_HostWindow->isExposed() || m_Impl->m_HostWindow->size().isEmpty()) return;

    const QRect rect(QPoint(), m_Impl->m_HostWindow->size());
    const QColor color = m_Impl->m_BaseBackgroundColor.isValid()
        ? m_Impl->m_BaseBackgroundColor : Theme::Color(Theme::PageBackground);
    QBackingStore store(m_Impl->m_HostWindow);
    store.resize(rect.size());
    store.beginPaint(rect);
    {
        QPainter painter(store.paintDevice());
        painter.fillRect(rect, color);
        if(!m_Impl->m_FailureText.isEmpty()){
            painter.setPen(color.lightness() < 128 ? Qt::white : Qt::black);
            const int margin = DeviceScale::FromDpi(16, logicalDpiY());
            const QRect box = rect.width() > margin * 2 ? rect.adjusted(margin, 0, -margin, 0) : rect;
            const bool fits = painter.boundingRect(box, Qt::AlignCenter | Qt::TextWordWrap,
                                                   m_Impl->m_FailureText).height() <= box.height();
            painter.drawText(box, (fits ? Qt::AlignCenter : Qt::AlignHCenter | Qt::AlignTop) | Qt::TextWordWrap,
                             m_Impl->m_FailureText);
        }
    }
    store.endPaint();
    store.flush(rect);
}

static EdgeInputLedger::Button LedgerButtonOf(Qt::MouseButton button){
    switch(button){
    case Qt::LeftButton:   return EdgeInputLedger::LeftButton;
    case Qt::RightButton:  return EdgeInputLedger::RightButton;
    case Qt::MiddleButton: return EdgeInputLedger::MiddleButton;
    case Qt::XButton1:     return EdgeInputLedger::XButton1;
    case Qt::XButton2:     return EdgeInputLedger::XButton2;
    default:               return EdgeInputLedger::NoButton;
    }
}

static int DownKindOf(Qt::MouseButton button){
    switch(button){
    case Qt::LeftButton:   return COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_DOWN;
    case Qt::RightButton:  return COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_DOWN;
    case Qt::MiddleButton: return COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_DOWN;
    case Qt::XButton1:
    case Qt::XButton2:     return COREWEBVIEW2_MOUSE_EVENT_KIND_X_BUTTON_DOWN;
    default:               return -1;
    }
}

static int DoubleKindOf(Qt::MouseButton button){
    switch(button){
    case Qt::LeftButton:   return COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_DOUBLE_CLICK;
    case Qt::RightButton:  return COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_DOUBLE_CLICK;
    case Qt::MiddleButton: return COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_DOUBLE_CLICK;
    case Qt::XButton1:
    case Qt::XButton2:     return COREWEBVIEW2_MOUSE_EVENT_KIND_X_BUTTON_DOUBLE_CLICK;
    default:               return -1;
    }
}

static Qt::MouseButtons QtButtonsOf(int ledgerButtons){
    Qt::MouseButtons buttons = Qt::NoButton;
    if(ledgerButtons & EdgeInputLedger::LeftButton)   buttons |= Qt::LeftButton;
    if(ledgerButtons & EdgeInputLedger::RightButton)  buttons |= Qt::RightButton;
    if(ledgerButtons & EdgeInputLedger::MiddleButton) buttons |= Qt::MiddleButton;
    if(ledgerButtons & EdgeInputLedger::XButton1)     buttons |= Qt::XButton1;
    if(ledgerButtons & EdgeInputLedger::XButton2)     buttons |= Qt::XButton2;
    return buttons;
}

static int UpKindOf(Qt::MouseButton button){
    switch(button){
    case Qt::LeftButton:   return COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_UP;
    case Qt::RightButton:  return COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_UP;
    case Qt::MiddleButton: return COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_UP;
    case Qt::XButton1:
    case Qt::XButton2:     return COREWEBVIEW2_MOUSE_EVENT_KIND_X_BUTTON_UP;
    default:               return -1;
    }
}

void EdgeWebView::SendMouse(int kind, unsigned int data, const QPointF &pos,
                            Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons){
    if(!m_Impl->m_Composition) return;

    const qreal ratio = m_Impl->m_HostWindow ? m_Impl->m_HostWindow->devicePixelRatio() : 1.0;
    POINT point = {};
    point.x = static_cast<LONG>(qRound(pos.x() * ratio));
    point.y = static_cast<LONG>(qRound(pos.y() * ratio));

    buttons &= ~QtButtonsOf(m_Impl->m_Input.Kept());

    unsigned int keys = COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_NONE;
    if(modifiers & Qt::ControlModifier) keys |= COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_CONTROL;
    if(modifiers & Qt::ShiftModifier)   keys |= COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_SHIFT;
    if(buttons & Qt::LeftButton)        keys |= COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_LEFT_BUTTON;
    if(buttons & Qt::RightButton)       keys |= COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_RIGHT_BUTTON;
    if(buttons & Qt::MiddleButton)      keys |= COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_MIDDLE_BUTTON;

    m_Impl->m_Composition->SendMouseInput
        (static_cast<COREWEBVIEW2_MOUSE_EVENT_KIND>(kind),
         static_cast<COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS>(keys),
         data, point);
}

void EdgeWebView::ApplyCursor(unsigned int systemCursorId){
    const Qt::CursorShape shape = EdgeCursorShape(systemCursorId);
    if(m_Impl->m_HostWindow) m_Impl->m_HostWindow->setCursor(QCursor(shape));
    if(m_Impl->m_Container) m_Impl->m_Container->setCursor(QCursor(shape));
}

void EdgeWebView::AbandonMouse(){
    const int owed = m_Impl->m_Input.Abandon();

    const QPointF pos = m_Impl->m_LastMousePos;
    if(owed){
        if(owed & EdgeInputLedger::LeftButton)
            SendMouse(COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_UP, 0, pos, Qt::NoModifier, Qt::NoButton);
        if(owed & EdgeInputLedger::RightButton)
            SendMouse(COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_UP, 0, pos, Qt::NoModifier, Qt::NoButton);
        if(owed & EdgeInputLedger::MiddleButton)
            SendMouse(COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_UP, 0, pos, Qt::NoModifier, Qt::NoButton);
        if(owed & (EdgeInputLedger::XButton1 | EdgeInputLedger::XButton2))
            SendMouse(COREWEBVIEW2_MOUSE_EVENT_KIND_X_BUTTON_UP, 0, pos, Qt::NoModifier, Qt::NoButton);

        SendMouse(COREWEBVIEW2_MOUSE_EVENT_KIND_LEAVE, 0, pos, Qt::NoModifier, Qt::NoButton);
    }

    GestureAborted();
}

void EdgeWebView::ShowGestureInProgress(){
    const QString shape = GestureToString(m_Gesture);
    if(!m_RightGestureMap.contains(shape)){
        emit statusBarMessage(shape + QStringLiteral(" (") + tr("NoAction") + QStringLiteral(")"));
        return;
    }
    const QString name = m_RightGestureMap.value(shape);
    QAction *action = Page::IsValidAction(name)
        ? Action(Page::StringToAction(name)) : nullptr;
    emit statusBarMessage(shape + QStringLiteral(" (") +
                          (action ? action->text() : name) +
                          QStringLiteral(")"));
}

void EdgeWebView::WheelEvent(QWheelEvent *ev){
    if(m_Impl && m_Impl->m_Composition) ForwardMouseEvent(ev);
    ev->setAccepted(true);
}

bool EdgeWebView::ForwardMouseEvent(QEvent *ev){
    m_Impl->m_Input.SetReady(m_Impl->m_Composition && visible());

    switch(ev->type()){
    case QEvent::MouseButtonDblClick:
        return true;
    case QEvent::MouseButtonPress: {
        QMouseEvent *me = static_cast<QMouseEvent*>(ev);
        m_Impl->m_LastMousePos = me->position();
        const EdgeInputLedger::Button button = LedgerButtonOf(me->button());

        QString name;
        Application::AddModifiersToString(name, me->modifiers());
        Application::AddMouseButtonsToString(name, me->buttons() & ~me->button());
        Application::AddMouseButtonToString(name, me->button());
        const QString action = m_MouseMap.value(name);
        const bool hostWants = !action.isEmpty() && CanCompleteAction(action);

        if(!hostWants && m_EnableRightGestureLocal && me->button() == Qt::RightButton &&
           m_Impl->m_Input.Hold(button)){
            m_Impl->m_Clicks.Reset();
            GestureStarted(me->position().toPoint());
            return true;
        }

        const EdgeInputLedger::Send send = m_Impl->m_Input.Press(button, hostWants);

        const bool doubled = m_Impl->m_Clicks.Press(
            me->button(), me->timestamp(), me->position(),
            QGuiApplication::styleHints()->mouseDoubleClickInterval(),
            QGuiApplication::styleHints()->mouseDoubleClickDistance(),
            send == EdgeInputLedger::Send::ToBackend);

        if(send == EdgeInputLedger::Send::ToBackend){
            const int kind = doubled ? DoubleKindOf(me->button())
                                     : DownKindOf(me->button());
            if(kind >= 0)
                SendMouse(kind, 0, me->position(), me->modifiers(), me->buttons());
            return true;
        }
        if(hostWants){
            if(const int kept = m_Impl->m_Input.Kept()){
                m_Impl->m_Input.Drop(kept);
                GestureAborted();
            }
            View::TriggerAction(action);
            return true;
        }
        return false;
    }
    case QEvent::MouseButtonRelease: {
        QMouseEvent *me = static_cast<QMouseEvent*>(ev);
        m_Impl->m_LastMousePos = me->position();
        const EdgeInputLedger::Button button = LedgerButtonOf(me->button());

        if(m_Impl->m_Input.Kept() & button) emit statusBarMessage(QString());

        if((m_Impl->m_Input.Kept() & button) && !m_Gesture.isEmpty()){
            m_Impl->m_Input.Drop(button);
            GestureFinished(me->position().toPoint(), me->button());
            return true;
        }

        switch(m_Impl->m_Input.Release(button)){
        case EdgeInputLedger::Send::DownThenUp: {
            const int down = DownKindOf(me->button());
            const int up   = UpKindOf(me->button());
            if(down >= 0)
                SendMouse(down, 0, me->position(), me->modifiers(),
                          me->buttons() | me->button());
            if(up >= 0)
                SendMouse(up, 0, me->position(), me->modifiers(), me->buttons());
            GestureAborted();
            break;
        }
        case EdgeInputLedger::Send::ToBackend: {
            const int kind = UpKindOf(me->button());
            if(kind >= 0)
                SendMouse(kind, 0, me->position(), me->modifiers(), me->buttons());
            break;
        }
        case EdgeInputLedger::Send::Nothing:
            break;
        }
        return true;
    }
    case QEvent::MouseMove: {
        QMouseEvent *me = static_cast<QMouseEvent*>(ev);
        m_Impl->m_LastMousePos = me->position();

        if(m_Impl->m_Input.Kept()){
            GestureMoved(me->position().toPoint());
            ShowGestureInProgress();
        }

        if(m_Impl->m_Input.Move() == EdgeInputLedger::Send::ToBackend)
            SendMouse(COREWEBVIEW2_MOUSE_EVENT_KIND_MOVE, 0,
                      me->position(), me->modifiers(), me->buttons());
        return true;
    }
    case QEvent::Wheel: {
        QWheelEvent *we = static_cast<QWheelEvent*>(ev);

        QString name;
        Application::AddModifiersToString(name, we->modifiers());
        Application::AddMouseButtonsToString(name, we->buttons());
        Application::AddWheelDirectionToString(name, Application::WheelWentUp(we));
        const QString action = m_MouseMap.value(name);
        if(!action.isEmpty() && CanCompleteAction(action)){
            if(const int kept = m_Impl->m_Input.Kept()){
                m_Impl->m_Input.Drop(kept);
                GestureAborted();
            }
            View::TriggerAction(action);
            return true;
        }

        if(m_Impl->m_Input.Wheel() != EdgeInputLedger::Send::ToBackend) return true;

        const QPoint angle = we->angleDelta();
        if(angle.y() != 0)
            SendMouse(COREWEBVIEW2_MOUSE_EVENT_KIND_WHEEL,
                      static_cast<unsigned int>(angle.y()),
                      we->position(), we->modifiers(), we->buttons());
        if(angle.x() != 0)
            SendMouse(COREWEBVIEW2_MOUSE_EVENT_KIND_HORIZONTAL_WHEEL,
                      static_cast<unsigned int>(angle.x()),
                      we->position(), we->modifiers(), we->buttons());
        return true;
    }
    case QEvent::Enter:
        m_Impl->m_Input.Enter();
        return false;
    case QEvent::Leave:
        if(m_Impl->m_Input.Leave() == EdgeInputLedger::Send::ToBackend)
            SendMouse(COREWEBVIEW2_MOUSE_EVENT_KIND_LEAVE, 0, m_Impl->m_LastMousePos,
                      Qt::NoModifier, Qt::NoButton);
        return false;
    case QEvent::UngrabMouse:
        AbandonMouse();
        return false;
    case QEvent::WindowDeactivate:
        if(!isActiveWindow()) AbandonMouse();
        return false;
    default:
        return false;
    }
}

#endif
