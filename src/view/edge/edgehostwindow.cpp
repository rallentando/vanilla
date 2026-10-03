#include "switch.hpp"
#include "edgehostwindow.hpp"

#include <QWidget>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

EdgeHostWindow::EdgeHostWindow(QWindow *parent)
    : QWindow(parent)
{
    setFlag(Qt::WindowDoesNotAcceptFocus, true);
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

bool EdgeHostWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result){
#ifdef Q_OS_WIN
    if(eventType == "windows_generic_MSG" && message){
        const MSG *msg = static_cast<const MSG*>(message);
        if(AnswerMouseActivate(msg->message, result)) return true;
    }
#endif
    return QWindow::nativeEvent(eventType, message, result);
}
