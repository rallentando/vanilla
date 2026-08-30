#ifndef MEDIATYPE_HPP
#define MEDIATYPE_HPP

#include "switch.hpp"

#include <QString>

namespace MediaType {

    enum Kind {
        NotMedia,
        Image,
        Audio,
        Video,
    };

    Kind KindOfMimeName(const QString &mimeName);

    QString MimeNameOfPath(const QString &path);

    bool IsDecodableImage(const QString &path);

    Kind KindOfPath(const QString &path);

    bool IsImage(const QString &path);
    bool IsAudio(const QString &path);
    bool IsVideo(const QString &path);
    bool IsMedia(const QString &path);
}

#endif
