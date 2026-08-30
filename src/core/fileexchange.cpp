#include "fileexchange.hpp"

#include <QFile>

namespace FileExchange {

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
