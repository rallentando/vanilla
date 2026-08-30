#ifndef TESTSUPPORT_HPP
#define TESTSUPPORT_HPP

#include <QtGlobal>
#include <QString>

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

}

#endif
