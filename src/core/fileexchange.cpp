#include "fileexchange.hpp"

#include <QFile>

namespace FileExchange {

QByteArray ReadWhole(const QString &path, bool *ok){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)){
        if(ok) *ok = false;
        return QByteArray();
    }
    QByteArray data = file.readAll();
    file.close();
    if(ok) *ok = true;
    return data;
}

bool Replace(const QString &temporary, const QString &target){
    const QString previous = target + QStringLiteral(".prev");

    if(QFile::exists(previous) && !QFile::remove(previous)){
        QFile::remove(temporary);
        return false;
    }

    const bool hadTarget = QFile::exists(target);
    if(hadTarget && !QFile::rename(target, previous)){
        QFile::remove(temporary);
        return false;
    }

    if(!QFile::rename(temporary, target)){
        if(hadTarget) QFile::rename(previous, target);
        QFile::remove(temporary);
        return false;
    }

    if(hadTarget) QFile::remove(previous);
    return true;
}

}
