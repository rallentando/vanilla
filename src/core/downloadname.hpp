#ifndef DOWNLOADNAME_HPP
#define DOWNLOADNAME_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QString>
#include <QUrl>

namespace DownloadName {

QString Sanitize(const QString &name);

QString FromUrl(const QUrl &url);

QString WithSuffixForMimeType(const QString &name, const QString &mimeType);

QString FromContentDisposition(const QByteArray &header);

QString Suggest(const QString &suggested, const QString &mimeType, const QUrl &url);

QString Unique(const QString &path, int limit = 9999);

}

#endif
