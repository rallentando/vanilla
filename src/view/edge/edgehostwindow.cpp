#include "switch.hpp"
#include "edgehostwindow.hpp"

#include <QWidget>
#include <QGuiApplication>
#include <QTimer>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

EdgeHostWindow::EdgeHostWindow(QWindow *parent)
    : QWindow(parent)
{
    setFlag(Qt::WindowDoesNotAcceptFocus, true);

    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, [this](QWindow *window){
        if(!window || !m_Container || window != m_Container->window()->windowHandle()) return;
        QPointer<QWidget> container = m_Container;
        QTimer::singleShot(0, this, [this, container](){
            if(container && container == m_Container && container->hasFocus() &&
               QGuiApplication::focusWindow() == container->window()->windowHandle())
                emit ReactivatedWithFocus();
        });
    });
}

bool EdgeHostWindow::AnswerMouseActivate(unsigned int message, qintptr *result){
#ifdef Q_OS_WIN
    if(message != WM_MOUSEACTIVATE || !result) return false;
    *result = MA_ACTIVATE;
    return true;
#else
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
#endif
}

void EdgeHostWindow::GiveQtFocusOnPress(QWidget *widget){
    m_FocusOnPress = widget;
}

bool EdgeHostWindow::TakeQtFocus(){
    if(!m_FocusOnPress || m_FocusOnPress->hasFocus()) return false;
    m_FocusOnPress->setFocus(Qt::MouseFocusReason);
    return true;
}

void EdgeHostWindow::QuietContainerFocusIn(QWidget *container){
    if(m_Container) m_Container->removeEventFilter(this);
    m_Container = container;
    if(m_Container) m_Container->installEventFilter(this);
}

bool EdgeHostWindow::eventFilter(QObject *watched, QEvent *ev){
    if(watched == m_Container && ev->type() == QEvent::FocusIn) return true;
    return QWindow::eventFilter(watched, ev);
}

bool EdgeHostWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result){
#ifdef Q_OS_WIN
    if(eventType == "windows_generic_MSG" && message){
        const MSG *msg = static_cast<const MSG*>(message);
        if(AnswerMouseActivate(msg->message, result)) return true;
    }
#endif
    return QWindow::nativeEvent(eventType, message, result);
}
