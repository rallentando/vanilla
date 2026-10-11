#include "switch.hpp"

#include "qtlog.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

namespace {

QMutex mutex;
QFile *file = nullptr;
qint64 written = 0;
bool dropped = false;
QtMessageHandler previous = nullptr;

thread_local bool inside = false;

const char *TypeName(QtMsgType type){
    switch(type){
    case QtWarningMsg:  return "warning";
    case QtCriticalMsg: return "critical";
    case QtFatalMsg:    return "fatal";
    case QtInfoMsg:     return "info";
    default:            return "debug";
    }
}

void WriteLine(const QString &line){
    if(!file) return;
    const QByteArray bytes = line.toUtf8() + '\n';
    if(written + bytes.size() > QtLog::RunLimit){
        if(!dropped){
            dropped = true;
            file->write("--- the rest of this run is not written\n");
            file->flush();
        }
        return;
    }
    file->write(bytes);
    file->flush();
    written += bytes.size();
}

void Handler(QtMsgType type, const QMessageLogContext &context, const QString &message){
    if(QtLog::Dropped(type, message)) return;
    if(!inside && (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)){
        inside = true;
        {
            QMutexLocker lock(&mutex);
            WriteLine(QtLog::Format(type, context, message));
        }
        inside = false;
    }
    if(previous) previous(type, context, message);
}

}

namespace QtLog {

bool Dropped(QtMsgType type, const QString &message){
    return type == QtWarningMsg
        && message.startsWith(QStringLiteral("requestActivate() called for "))
        && message.contains(QStringLiteral(" EdgeHostWindow("))
        && message.contains(QStringLiteral("Qt::WindowDoesNotAcceptFocus"));
}

QString Format(QtMsgType type, const QMessageLogContext &context, const QString &message){
    QString line = QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
        + QLatin1Char(' ') + QLatin1String(TypeName(type));
    if(context.category && qstrcmp(context.category, "default") != 0)
        line += QLatin1Char(' ') + QLatin1String(context.category);
    QString body = message;
    body.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    line += QStringLiteral(": ") + body;
    if(context.file)
        line += QStringLiteral(" (%1:%2)").arg(QLatin1String(context.file)).arg(context.line);
    return line;
}

bool Install(const QString &path){
    QMutexLocker lock(&mutex);
    if(file) return true;

    if(QFileInfo(path).size() > RotateLimit){
        const QString old = path + QStringLiteral(".old");
        QFile::remove(old);
        QFile::rename(path, old);
    }
    QFile *f = new QFile(path);
    if(!f->open(QIODevice::WriteOnly | QIODevice::Append)){
        delete f;
        return false;
    }
    file = f;
    written = 0;
    dropped = false;
    WriteLine(QStringLiteral("--- %1 %2 %3 started, pid %4")
              .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
                   QCoreApplication::applicationName(),
                   QCoreApplication::applicationVersion())
              .arg(QCoreApplication::applicationPid()));
    previous = qInstallMessageHandler(Handler);
    return true;
}

void Uninstall(){
    QMutexLocker lock(&mutex);
    if(!file) return;
    qInstallMessageHandler(previous);
    previous = nullptr;
    file->close();
    delete file;
    file = nullptr;
}

}
