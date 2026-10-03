#ifndef TESTSUPPORT_HPP
#define TESTSUPPORT_HPP

#include <QtGlobal>
#include <QString>
#include <QApplication>
#include <QProxyStyle>

namespace TestSupport {

inline QtMessageHandler &PreviousHandler(){
    static QtMessageHandler handler = nullptr;
    return handler;
}

inline void DropDebugMessages(QtMsgType type, const QMessageLogContext &context, const QString &message){
    if(type == QtDebugMsg) return;
    if(PreviousHandler()) PreviousHandler()(type, context, message);
}

inline void SilenceDebugOutput(){
    PreviousHandler() = qInstallMessageHandler(DropDebugMessages);
}

class NoWidgetAnimationStyle : public QProxyStyle {
public:
    explicit NoWidgetAnimationStyle(QStyle *base) : QProxyStyle(base) {}
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *ret = nullptr) const override {
        if(hint == QStyle::SH_Widget_Animation_Duration) return 0;
        return QProxyStyle::styleHint(hint, option, widget, ret);
    }
};

inline void DisableWidgetAnimation(){
    QApplication::setStyle(new NoWidgetAnimationStyle(QApplication::style()));
}

}

#endif
