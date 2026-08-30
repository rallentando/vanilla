#include "switch.hpp"

#include "mediatype.hpp"

#include <QImageReader>
#include <QMimeDatabase>
#include <QMimeType>
#include <QSet>

namespace MediaType {

    namespace {

        bool IsMediaContainerName(const QString &mimeName){
            return mimeName == QStringLiteral("application/ogg")
                || mimeName == QStringLiteral("application/x-ogg")
                || mimeName == QStringLiteral("application/mxf")
                || mimeName == QStringLiteral("application/x-matroska")
                || mimeName == QStringLiteral("application/vnd.rn-realmedia")
                || mimeName == QStringLiteral("application/vnd.ms-asf");
        }
    }

    Kind KindOfMimeName(const QString &mimeName){
        if(mimeName.startsWith(QStringLiteral("image/"))) return Image;
        if(mimeName.startsWith(QStringLiteral("audio/"))) return Audio;
        if(mimeName.startsWith(QStringLiteral("video/"))) return Video;
        if(IsMediaContainerName(mimeName)) return Video;
        return NotMedia;
    }

    QString MimeNameOfPath(const QString &path){
        static QMimeDatabase db;
        return db.mimeTypeForFile(path, QMimeDatabase::MatchExtension).name();
    }

    bool IsDecodableImage(const QString &path){
        if(path.endsWith(QStringLiteral("#"))) return false;

        static const QSet<QByteArray> formats = [](){
            QSet<QByteArray> set;
            foreach(const QByteArray &format, QImageReader::supportedImageFormats())
                set.insert(format);
            return set;
        }();

        QImageReader reader(path);
        return formats.contains(reader.format());
    }

    Kind KindOfPath(const QString &path){
        if(IsDecodableImage(path)) return Image;

        Kind kind = KindOfMimeName(MimeNameOfPath(path));

        if(kind == Image) return NotMedia;

        return kind;
    }

    bool IsImage(const QString &path){ return KindOfPath(path) == Image;}
    bool IsAudio(const QString &path){ return KindOfPath(path) == Audio;}
    bool IsVideo(const QString &path){ return KindOfPath(path) == Video;}

    bool IsMedia(const QString &path){ return KindOfPath(path) != NotMedia;}
}
