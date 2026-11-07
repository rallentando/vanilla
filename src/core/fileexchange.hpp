#ifndef FILEEXCHANGE_HPP
#define FILEEXCHANGE_HPP

#include <QByteArray>
#include <QString>

namespace FileExchange {

bool Replace(const QString &temporary, const QString &target);

QByteArray ReadWhole(const QString &path, bool *ok);

}

#endif
