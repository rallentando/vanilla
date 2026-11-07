#ifndef SEARCHSUGGEST_HPP
#define SEARCHSUGGEST_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace SearchSuggest {
    QUrl Url(const QString &engine, const QString &words);

    QStringList Parse(const QByteArray &answer);
}

#endif
